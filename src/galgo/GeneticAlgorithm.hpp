//=================================================================================================
//                    Copyright (C) 2017 Olivier Mallet - All Rights Reserved                      
//=================================================================================================

#ifndef GENETICALGORITHM_HPP
#define GENETICALGORITHM_HPP

namespace galgo {

//=================================================================================================

template <typename T>
class GeneticAlgorithm
{
   static_assert(std::is_same<std::string,T>::value, "variable type can only be string, please amend.");

   template <typename K>
   friend class Population;
   template <typename K>
   friend class Chromosome;

   template <typename K>
   using Func = std::vector<std::string> (*)(const std::vector<K>&);

private:
   Population<T> pop;             // population of chromosomes
   std::vector<PAR<T>> param;     // parameter(s) 
   std::vector<T> lowerBound;     // parameter(s) lower bound
   std::vector<T> upperBound;     // parameter(s) upper bound
   std::vector<T> initialSet;     // initial set of parameter(s)
   std::vector<int> idx;          // indexes for chromosome breakdown

public: 
   // objective function pointer
   Func<T> Objective; 
   // selection method initialized to roulette wheel selection                                   
   void (*Selection)(Population<T>&) = TNT;  
   // cross-over method initialized to 1-point cross-over                                
   void (*CrossOver)(const Population<T>&, CHR<T>&, CHR<T>&) = P1XO;
   // mutation method initialized to single-point mutation 
   void (*Mutation)(CHR<T>&) = SPM;  
   // adaptation to constraint(s) method                                        
   void (*Adaptation)(Population<T>&) = nullptr; 
   // constraint(s)                               
   std::vector<double> (*Constraint)(const std::vector<T>&) = nullptr; 

   double covrate = .90;   // cross-over rate
   double mutrate = .01;   // mutation rate   
   double SP = 1.0;        // selective pressure for RSP selection method 
   double tolerance = 0.0; // terminal condition (inactive if equal to zero)

   double chrmutrate = 1.0; 
                 
   int elitpop = 0;   // elit population size
   int matsize;       // mating pool size, set to popsize by default
   int tntsize = 3;  // tournament size
   int genstep = 1;  // generation step for outputting results
   int precision = 5; // precision for outputting results

   int earlystop = 5;

   int pattid;
   int graphid;

   int nfit = 1;

   std::string tmplt = "";

   // constructor
   template <int...N>
   GeneticAlgorithm(Func<T> objective, std::string tmplt, int popsize, int nbgen, bool output, int nbit, const Parameter<T,N>&...args);
   // run genetic algorithm
   std::vector<T> run();
   // return best chromosome 
   const CHR<T>& result() const;

private:
   int nbbit;     // total number of bits per chromosome
   int nbgen;     // number of generations
   int nogen = 0; // numero of generation
   int nbparam;   // number of parameters to be estimated
   int popsize;   // population size
   bool output;   // control if results must be outputted

   // end of recursion for initializing parameter(s) data
   template <int I = 0, int...N>
   typename std::enable_if<I == sizeof...(N), void>::type init(const TUP<T,N...>&); 
   // recursion for initializing parameter(s) data
   template <int I = 0, int...N>
   typename std::enable_if<I < sizeof...(N), void>::type init(const TUP<T,N...>&);

   // check inputs validity
   void check() const ;
   // print results for each new generation
   void print() const;
   void print(int bestNogen, std::vector<T> bestParam, std::vector<double> bestResult, std::vector<std::string> codes) const;
};

/*-------------------------------------------------------------------------------------------------*/
   
// constructor
template <typename T> template <int...N>
GeneticAlgorithm<T>::GeneticAlgorithm(Func<T> objective, std::string tmplt,  int popsize, int nbgen, bool output, int nbit, const Parameter<T,N>&...args)
{
   this->Objective = objective;
   // getting total number of bits per chromosome
   // this->nbbit = sum(N...);
   // std::cout<<"this->nbbit="<<this->nbbit<<"\n";
   this->nbbit = nbit; 
   this->nbgen = nbgen;
   // getting number of parameters in the pack
   this->nbparam = sizeof...(N);
   this->popsize = popsize;
   this->matsize = popsize;
   this->output = output;
   this->tmplt = tmplt;
   // unpacking parameter pack in tuple
   TUP<T,N...> tp(args...);
   // initializing parameter(s) data
   this->init(tp);
}

/*-------------------------------------------------------------------------------------------------*/

// end of recursion for initializing parameter(s) data
template <typename T> template <int I, int...N>
inline typename std::enable_if<I == sizeof...(N), void>::type 
GeneticAlgorithm<T>::init(const TUP<T,N...>& tp) {}

// recursion for initializing parameter(s) data
template <typename T> template <int I, int...N>
inline typename std::enable_if<I < sizeof...(N), void>::type 
GeneticAlgorithm<T>::init(const TUP<T,N...>& tp) 
{
   // getting Ith parameter in tuple
   auto par = std::get<I>(tp);
   // getting Ith parameter initial data
   const std::vector<T>& data = par.getData();
   // copying parameter data
   param.emplace_back(new decltype(par)(par));
   lowerBound.push_back(data[0]);
   upperBound.push_back(data[1]);
   // if parameter has initial value
   if (data.size() > 2) {
      initialSet.push_back(data[2]);
   }
   // setting indexes for chromosome breakdown
   if (I == 0) {
      idx.push_back(0);
   } else {
      idx.push_back(idx[I - 1] + par.size());
   }
   // recursing
   init<I + 1>(tp);
}

/*-------------------------------------------------------------------------------------------------*/

// check inputs validity
template <typename T>
void GeneticAlgorithm<T>::check() const
{
#if 0
   if (!initialSet.empty()) {
      for (int i = 0; i < nbparam; ++i) {
         if (initialSet[i] < lowerBound[i] || initialSet[i] > upperBound[i]) {
            throw std::invalid_argument("Error: in class galgo::Parameter<T,N>, initial parameter value cannot be outside the parameter boundaries, please choose a value between its lower and upper bounds.");
         }
      }
      if (initialSet.size() != (unsigned)nbparam) {
         throw std::invalid_argument("Error: in class galgo::GeneticAlgorithm<T>, initial set of parameters does not have the same dimension than the number of parameters, please adjust.");
      }
   }
#endif
   if (SP < 1.0 || SP > 2.0) {
      throw std::invalid_argument("Error: in class galgo::GeneticAlgorithm<T>, selective pressure (SP) cannot be outside [1.0,2.0], please choose a real value within this interval.");
   }
   if (elitpop > popsize || elitpop < 0) {
      throw std::invalid_argument("Error: in class galgo::GeneticAlgorithm<T>, elit population (elitpop) cannot outside [0,popsize], please choose an integral value within this interval.");
   }
   if (covrate < 0.0 || covrate > 1.0) {
      throw std::invalid_argument("Error: in class galgo::GeneticAlgorithm<T>, cross-over rate (covrate) cannot outside [0.0,1.0], please choose a real value within this interval.");
   }
   if (genstep <= 0) {
      throw std::invalid_argument("Error: in class galgo::GeneticAlgorithm<T>, generation step (genstep) cannot be <= 0, please choose an integral value > 0.");
   }
}

/*-------------------------------------------------------------------------------------------------*/
   
// run genetic algorithm
template <typename T>
std::vector<T> GeneticAlgorithm<T>::run()
{
   // checking inputs validity
   this->check();

   // setting adaptation method to default if needed
   if (Constraint != nullptr && Adaptation == nullptr) {
      Adaptation = DAC;
   }

   // initializing population
   pop = Population<T>(*this);

   if (output) {
      std::cout << "\n Running Genetic Algorithm...\n";
      std::cout << " ----------------------------\n";
   }

   // creating population
   pop.creation();
   // initializing best result and previous best result
   double bestResult = pop(0)->getTotal();
   double globalBestResult = bestResult;

   std::vector<T> popBestParam = pop(0)->getParam();
   std::vector<double> popBestResult = pop(0)->getResult();
   std::vector<std::string> codes;
   std::vector<std::string> gids;

   codes = pop(0)->getOthercodes();
   gids = pop(0)->getGids();

   int bestNogen = 0;

   // outputting results 
   if (output) print();

   // early stop counter
   int escounter = 0;
    
   // starting population evolution
   for (nogen = 1; nogen <= nbgen; ++nogen) {
      if (tolerance != 0.0 && escounter >= earlystop) {
         std::cout << "\n Early Stop... at " << nogen << "\n";
         break;
      }
      // evolving population
      pop.evolution();
      // getting best current result
      bestResult = pop(0)->getTotal();
      // outputting results
      if (output) print();
      // checking convergence
      if (tolerance != 0.0) {
         if (fabs(bestResult - globalBestResult) < fabs(tolerance) || bestResult < globalBestResult)
         {
            escounter++;
         }
         else
         {
            escounter = 0;
         }
      }

      if (bestResult >= globalBestResult)
      {
         popBestParam = pop(0)->getParam();
         popBestResult = pop(0)->getResult();
         codes = pop(0)->getOthercodes();
         gids = pop(0)->getGids();
         globalBestResult = bestResult;
         bestNogen = nogen;
      }
   }
   print(bestNogen, popBestParam, popBestResult, codes);

   // outputting contraint value
   if (Constraint != nullptr) {
      // getting best parameter(s) constraint value(s)
      std::vector<double> cst = pop(0)->getConstraint(); 
      if (output) {
         std::cout << "\n Constraint(s)\n";
         std::cout << " -------------\n";
         for (unsigned i = 0; i < cst.size(); ++i) {
            std::cout << " C"; 
            if (nbparam > 1) {
               std::cout << std::to_string(i + 1);
            }
            std::cout << "(x) = " << std::setw(6) << std::fixed << std::setprecision(precision) << cst[i] << "\n"; 
         }
         std::cout << "\n"; 
      }
   }

   int codeLen = codes.size();
   codes.insert(codes.begin(), std::to_string(codeLen));
   codes.insert(codes.begin(), std::to_string(popBestResult[0]));
   for(auto gid : gids)
   {
      codes.push_back(gid);
   }
   return codes; 
}

/*-------------------------------------------------------------------------------------------------*/

// return best chromosome
template <typename T>
inline const CHR<T>& GeneticAlgorithm<T>::result() const
{
   return pop(0);
}

/*-------------------------------------------------------------------------------------------------*/

template <typename T>
void GeneticAlgorithm<T>::print(int bestNogen, std::vector<T> bestParam, std::vector<double> bestResult, std::vector<std::string> codes) const
{
   // getting best parameter(s) from best chromosome
   std::cout << " Best Generation = "  << bestNogen << " |";
   for (int i = 0; i < nbparam; ++i) {
	   std::cout << " X";
      if (nbparam > 1) {
         std::cout << std::to_string(i + 1);
      }
      std::cout << " = "  << bestParam[i] << " |";
	}
   for (unsigned i = 0; i < bestResult.size(); ++i) {
	   std::cout << " F";
      if (bestResult.size() > 1) {
         std::cout << std::to_string(i + 1);
      }
      std::cout << "(x) = " << std::setw(12) << std::fixed << std::setprecision(precision) << bestResult[i];
      // std::cout << " |";
      // if (i < bestResult.size() - 1) {
      //    std::cout << " |";
      // } else {
      //    std::cout << "\n";
      // }
   }
   // std::cout << " pattID = " << pattid << " | "
   //           << " graphID = " << graphid << "\n";
   
   // for(auto code : codes)
   // {
   //    std::cout<<" | "<<code;
   // }
   std::cout<<"\n";
}
   
// print results for each new generation
template <typename T>
void GeneticAlgorithm<T>::print() const
{
   // getting best parameter(s) from best chromosome
   std::vector<T> bestParam = pop(0)->getParam();
   std::vector<double> bestResult = pop(0)->getResult();

   if (nogen % genstep == 0) {
      std::cout << " Generation = " << std::setw(std::to_string(nbgen).size()) << nogen << " |";
      for (int i = 0; i < nbparam; ++i) {
	      std::cout << " X";
         if (nbparam > 1) {
            std::cout << std::to_string(i + 1);
         }
         // std::cout << " = "  << std::setw(9) << std::fixed << std::setprecision(precision) << bestParam[i] << " |";
         std::cout << " = "  << bestParam[i] << " |";
	   }
      for (unsigned i = 0; i < bestResult.size(); ++i) {
	      std::cout << " F";
         if (bestResult.size() > 1) {
            std::cout << std::to_string(i + 1);
         }
         std::cout << "(x) = " << std::setw(12) << std::fixed << std::setprecision(precision) << bestResult[i];
         if (i < bestResult.size() - 1) {
            std::cout << " |";
         } else {
            std::cout << "\n";
         }
	   }
 
   }
}
   
//=================================================================================================

}

#endif

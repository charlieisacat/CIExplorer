//=================================================================================================
//                    Copyright (C) 2017 Olivier Mallet - All Rights Reserved                      
//=================================================================================================

#ifndef CHROMOSOME_HPP
#define CHROMOSOME_HPP

namespace galgo {

//=================================================================================================

template <typename T>
class Chromosome
{
   static_assert(std::is_same<std::string,T>::value, "variable type can only be string, please amend.");

public:
   // constructor
   Chromosome(const GeneticAlgorithm<T>& ga);
   // copy constructor
   Chromosome(const Chromosome<T>& rhs);
   // create new chromosome 
   void create();
   // initialize chromosome
   void initialize();
   // evaluate chromosome 
   void evaluate(); 
   // reset chromosome
   void reset();
   // set or replace kth gene by a new one
   void setGene(int k);
   // initialize or replace kth gene by a know value
   void initGene(int k, T value);
   // add bit to chromosome
   void addBit(char bit);
   // initialize or replace an existing chromosome bit   
   void setBit(char bit, int pos);
   // flip an existing chromosome bit
   void flipBit(int pos);
   // get chromosome bit
   char getBit(int pos) const;
   // initialize or replace a portion of bits with a portion of another chromosome
   void setPortion(const Chromosome<T>& x, int start, int end);
   // initialize or replace a portion of bits with a portion of another chromosome
   void setPortion(const Chromosome<T>& x, int start);
   // get parameter value(s) from chromosome
   const std::vector<T>& getParam() const;
   // get objective function result
   const std::vector<double>& getResult() const;
   // get the total sum of all objective function(s) result
   double getTotal() const;
   // get constraint value(s)
   const std::vector<double> getConstraint() const;
   // return chromosome size in number of bits
   int size() const;
   // return number of chromosome bits to mutate
   double mutrate() const;
   double chrmutrate() const;
   double indpb() const;
   // return number of genes in chromosome
   int nbgene() const;
   // return numero of generation this chromosome belongs to
   int nogen() const;
   // return lower bound(s)
   const std::vector<T>& lowerBound() const;
   // return upper bound(s)
   const std::vector<T>& upperBound() const;
   const GeneticAlgorithm<T>* ptr = nullptr; // pointer to genetic algorithm

   // return other patts codes 
   const std::vector<std::string>& getOthercodes() const;
   const std::vector<std::string>& getGids() const;

private:
   std::vector<T> param;                     // estimated parameter(s)
   std::vector<double> result;                    // chromosome objective function(s) result
   std::string chr;                          // string of bits representing chromosome
public:
   double fitness;                                // chromosome fitness, objective function(s) result that can be modified (adapted to constraint(s), set to positive values, etc...)
   std::vector<std::string> otherpatts;
   std::vector<std::string> gids;
   int nfit;
private:
   double total;                                  // total sum of objective function(s) result
   int chrsize;                              // chromosome size (in number of bits)
   int numgen;                               // numero of generation
};

/*-------------------------------------------------------------------------------------------------*/

// constructor
template <typename T>
Chromosome<T>::Chromosome(const GeneticAlgorithm<T>& ga) 
{
   param.resize(ga.nbparam);
   ptr = &ga;
   chrsize = ga.nbbit;
   numgen = ga.nogen;
   nfit = ga.nfit;
}

/*-------------------------------------------------------------------------------------------------*/

// copy constructor
template <typename T>
Chromosome<T>::Chromosome(const Chromosome<T>& rhs) 
{
   param = rhs.param;
   result = rhs.result;
   chr = rhs.chr;
   ptr = rhs.ptr;
   // re-initializing fitness to its original value
   fitness = rhs.total;
   total = rhs.total;
   chrsize = rhs.chrsize;
   numgen = rhs.numgen;
   nfit = rhs.nfit;
}

/*-------------------------------------------------------------------------------------------------*/

// create new chromosome
template <typename T>
inline void Chromosome<T>::create()
{
   chr.clear();

   for (const auto& x : ptr->param) {
      // encoding parameter random value
      std::string str = x->encode();
      chr.append(str);
   }  
}

/*-------------------------------------------------------------------------------------------------*/

// initialize chromosome (from known parameter values)
template <typename T>
inline void Chromosome<T>::initialize()
{
   chr.clear();

   int i(0);
   for (const auto& x : ptr->param) {
      // encoding parameter initial value
      std::string str = x->encode(ptr->initialSet[i++]);
      chr.append(str);
   }      
}

/*-------------------------------------------------------------------------------------------------*/

// evaluate chromosome fitness
template <typename T>
inline void Chromosome<T>::evaluate() 
{
   int i(0);
   for (const auto& x : ptr->param) {
      // decoding chromosome: converting chromosome string into a real value
      param[i] = x->decode(chr.substr(ptr->idx[i], x->size()));
      i++;
   } 
   // computing objective result(s) 
   auto tmpResult = ptr->Objective(param);

   gids.clear();
   otherpatts.clear();
   result.clear();

   // std::cout<<"???size="<<tmpResult.size()<<"\n";

   result.push_back(std::stod(tmpResult[0]));
   int codeLen = std::stoi(tmpResult[1]);

   if (codeLen != 0)
   {
      for (int j = 2; j < 2 + codeLen; j++)
      {
         otherpatts.push_back(tmpResult[j]);
      }

      for (int j = 2 + codeLen; j < tmpResult.size(); j++)
      {
         gids.push_back(tmpResult[j]);
      }
   }

   // computing sum of all results (in case there is not only one objective functions)
   total = std::accumulate(result.begin(), result.end(), 0.0);
   // initializing fitness to this total
   fitness = total;
}

/*-------------------------------------------------------------------------------------------------*/

// reset chromosome
template <typename T>
inline void Chromosome<T>::reset()
{
   chr.clear();
   result = 0.0;
   total = 0.0;
   fitness = 0.0;
}

/*-------------------------------------------------------------------------------------------------*/

// set or replace kth gene by a new one
template <typename T>
inline void Chromosome<T>::setGene(int k)
{
   #ifndef NDEBUG
   if (k < 0 || k >= ptr->nbparam) {
      std::cout<<"Error: in galgo::Chromosome<T>::setGene(int), argument cannot be outside interval [0,nbparam-1], please amend."<<"\n";
      throw std::invalid_argument("Error: in galgo::Chromosome<T>::setGene(int), argument cannot be outside interval [0,nbparam-1], please amend.");
   }
   #endif

   // generating a new gene
   std::string s = ptr->param[k]->encode();
   // adding or replacing gene in chromosome
   chr.replace(ptr->idx[k], s.size(), s, 0, s.size());
}

/*-------------------------------------------------------------------------------------------------*/

// initialize or replace kth gene by a know value
template <typename T>
inline void Chromosome<T>::initGene(int k, T x)
{
   #ifndef NDEBUG
   if (k < 0 || k >= ptr->nbparam) {
      std::cout<<"Error: in galgo::Chromosome<T>::initGene(int), first argument cannot be outside interval [0,nbparam-1], please amend."<<"\n";
      throw std::invalid_argument("Error: in galgo::Chromosome<T>::initGene(int), first argument cannot be outside interval [0,nbparam-1], please amend.");
   }
   #endif

   // encoding gene
   std::string s = ptr->param[k]->encode(x);
   // adding or replacing gene in chromosome
   chr.replace(ptr->idx[k], s.size(), s, 0, s.size());
}

/*-------------------------------------------------------------------------------------------------*/

// add chromosome bit to chromosome (when constructing a new one)
template <typename T>
inline void Chromosome<T>::addBit(char bit)
{
   chr.push_back(bit);

   #ifndef NDEBUG
   if (chr.size() > chrsize) {
      std::cout<<"Error: in galgo::Chromosome<T>::setBit(char), exceeding chromosome size."<<"\n";
      throw std::out_of_range("Error: in galgo::Chromosome<T>::setBit(char), exceeding chromosome size.");
   }
   #endif
}

/*-------------------------------------------------------------------------------------------------*/

// initialize or replace an existing chromosome bit   
template <typename T>
inline void Chromosome<T>::setBit(char bit, int pos)
{  
   #ifndef NDEBUG
   if (pos >= chrsize) {
      std::cout<<"Error: in galgo::Chromosome<T>::replaceBit(char, int), second argument cannot be equal or greater than chromosome size."<<"\n";
      throw std::out_of_range("Error: in galgo::Chromosome<T>::replaceBit(char, int), second argument cannot be equal or greater than chromosome size.");
   }
   #endif

   std::stringstream ss;
   std::string str;
   ss << bit;
   ss >> str;
   chr.replace(pos, 1, str);
   std::cout << chr << "\n";
}

/*-------------------------------------------------------------------------------------------------*/
      
// flip an existing chromosome bit
template <typename T>
inline void Chromosome<T>::flipBit(int pos)
{
   #ifndef NDEBUG
   if (pos >= chrsize) {
      std::cout<<"Error: in galgo::Chromosome<T>::flipBit(int), argument cannot be equal or greater than chromosome size."<<"\n";
      throw std::out_of_range("Error: in galgo::Chromosome<T>::flipBit(int), argument cannot be equal or greater than chromosome size.");
   }
   #endif

   if (chr[pos] == '0') {
      chr.replace(pos, 1, "1");
   } else {
      chr.replace(pos, 1, "0");
   }
}

/*-------------------------------------------------------------------------------------------------*/

// get a chromosome bit
template <typename T>
inline char Chromosome<T>::getBit(int pos) const
{
   #ifndef NDEBUG
   if (pos >= chrsize) {
      std::cout<<"Error: in galgo::Chromosome<T>::getBit(int), argument cannot be equal or greater than chromosome size."<<"\n";
      throw std::out_of_range("Error: in galgo::Chromosome<T>::getBit(int), argument cannot be equal or greater than chromosome size.");
   }
   #endif

   return chr[pos];
}

/*-------------------------------------------------------------------------------------------------*/

// initialize or replace a portion of bits with a portion of another chromosome (from position start to position end included)
template <typename T>
inline void Chromosome<T>::setPortion(const Chromosome<T>& x, int start, int end)
{
   #ifndef NDEBUG
   if (start > chrsize) {
      std::cout<<"Error: in galgo::Chromosome<T>::setPortion(const Chromosome<T>&, int, int), second argument cannot be greater than chromosome size. func 2"<<"\n";
      throw std::out_of_range("Error: in galgo::Chromosome<T>::setPortion(const Chromosome<T>&, int, int), second argument cannot be greater than chromosome size.");
   }
   #endif

   chr.replace(start, end - start + 1, x.chr, start, end - start + 1);
}

/*-------------------------------------------------------------------------------------------------*/

// initialize or replace a portion of bits with a portion of another chromosome (from position start to the end of he chromosome)
template <typename T>
inline void Chromosome<T>::setPortion(const Chromosome<T>& x, int start)
{
   #ifndef NDEBUG
   if (start > chrsize) {
      std::cout<<"Error: in galgo::Chromosome<T>::setPortion(const Chromosome<T>&, int), second argument cannot be greater than chromosome size. func 2"<<"\n";
      throw std::out_of_range("Error: in galgo::Chromosome<T>::setPortion(const Chromosome<T>&, int), second argument cannot be greater than chromosome size.");
   }
   #endif

   chr.replace(start, chrsize, x.chr, start, x.chrsize);
}

/*-------------------------------------------------------------------------------------------------*/

// get parameter value(s) from chromosome
template <typename T>
inline const std::vector<T>& Chromosome<T>::getParam() const
{
   return param;
}

template <typename T>
inline const std::vector<std::string>& Chromosome<T>::getOthercodes() const
{
   return otherpatts;
}

template <typename T>
inline const std::vector<std::string>& Chromosome<T>::getGids() const
{
   return gids;
}

/*-------------------------------------------------------------------------------------------------*/

// get objective function result
template <typename T>
inline const std::vector<double>& Chromosome<T>::getResult() const
{
   return result;
}

/*-------------------------------------------------------------------------------------------------*/

// get the total sum of all objective function(s) result
template <typename T>
inline double Chromosome<T>::getTotal() const
{
   return total;
}

/*-------------------------------------------------------------------------------------------------*/

// get constraint value(s) for this chromosome
template <typename T>
inline const std::vector<double> Chromosome<T>::getConstraint() const
{
   return ptr->Constraint(param);
}

/*-------------------------------------------------------------------------------------------------*/

// return chromosome size in number of bits
template <typename T>
inline int Chromosome<T>::size() const
{
   return chrsize;
}

/*-------------------------------------------------------------------------------------------------*/

// return mutation rate 
template <typename T>
inline double Chromosome<T>::mutrate() const
{
   return ptr->mutrate;
}

// return mutation rate per chr
template <typename T>
inline double Chromosome<T>::chrmutrate() const
{
   return ptr->chrmutrate;
}

template <typename T>
inline double Chromosome<T>::indpb() const
{
   return ptr->indpb;
}

/*-------------------------------------------------------------------------------------------------*/

// return number of genes in chromosome
template <typename T>
inline int Chromosome<T>::nbgene() const
{
   return ptr->nbparam;
}

/*-------------------------------------------------------------------------------------------------*/

// return numero of generation this chromosome belongs to
template <typename T>
inline int Chromosome<T>::nogen() const
{
   return numgen;
}

/*-------------------------------------------------------------------------------------------------*/

// return lower bound(s)
template <typename T>
inline const std::vector<T>& Chromosome<T>::lowerBound() const
{
   return ptr->lowerBound;
}

/*-------------------------------------------------------------------------------------------------*/

// return upper bound(s)
template <typename T>
inline const std::vector<T>& Chromosome<T>::upperBound() const
{
   return ptr->upperBound;
}

//=================================================================================================

}

#endif

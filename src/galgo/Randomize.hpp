//=================================================================================================
//                    Copyright (C) 2017 Olivier Mallet - All Rights Reserved
//=================================================================================================

#ifndef RANDOMIZE_H
#define RANDOMIZE_H

namespace galgo
{

    //=================================================================================================

    // template metaprogramming for getting maximum unsigned integral value from N bits
    template <unsigned int N>
    struct MAXVALUE
    {
        enum : uint64_t
        {
            value = 2 * MAXVALUE<N - 1>::value
        };
    };

    // template specialization for initial case N = 0
    template <>
    struct MAXVALUE<0>
    {
        enum
        {
            value = 1
        };
    };

    /*-------------------------------------------------------------------------------------------------*/

    // Mersenne Twister 19937 pseudo-random number generator
    // std::random_device rand_dev;
    // std::mt19937_64 rng(rand_dev());
    std::mt19937_64 rng(21011013);

    // generate uniform random probability in range [0,1)
    std::uniform_real_distribution<> proba(0, 1);

    /*-------------------------------------------------------------------------------------------------*/

    // generate a uniform random number within the interval [min,max)
    template <typename T>
    inline T uniform(T min, T max)
    {
#if 0
#ifndef NDEBUG
        if (min >= max)
        {
            throw std::invalid_argument("Error: in galgo::uniform(T, T), first argument must be < to second argument.");
        }
#endif
#endif

        return min + proba(rng) * (max - min);
    }

    /*-------------------------------------------------------------------------------------------------*/

    // static class for generating random unsigned integral numbers
    template <int N>
    class Randomize
    {

    public:
        // computation only done once for each different N
        static constexpr uint64_t MAXVAL = MAXVALUE<N>::value - 1;

        // generating random unsigned long long integer on [0,MAXVAL]
        static uint64_t generate()
        {
            // class constructor only called once for each different N
            static std::uniform_int_distribution<uint64_t> udistrib(0, MAXVAL);
            return udistrib(rng);
        }

        static std::string generateStr()
        {
            static std::uniform_int_distribution<uint64_t> udistrib(0, 1);
            std::string s = "";
            for (int i = 0; i < N; i++)
            {
                s += std::to_string(udistrib(rng));
            }
            return s;
        }

        static std::string generateStr(int x)
        {
            static std::uniform_int_distribution<uint64_t> udistrib(0, 1);
            std::string s = "";
            for (int i = 0; i < x; i++)
            {
                s += std::to_string(udistrib(rng));
            }
            return s;
        }

        static std::string generateStr(int x, std::string tmplt)
        {
            if(tmplt == "")
            {
                std::cout<<"tmplt is empty..\n";
                return generateStr(x);

            }

            static std::uniform_int_distribution<uint64_t> udistrib(0, 1);
            std::string s = "";
            for (int i = 0; i < x; i++)
            {
                if (tmplt[i] == '1')
                    s += "1";
                else
                    s += std::to_string(udistrib(rng));
            }
            return s;
        }
    };

    //=================================================================================================

}

#endif

#ifndef ARGUMENT_HPP
#define ARGUMENT_HPP
#include <vector>

template <typename T>
class Argument
{
public:
    T start, end, stride;
    std::vector<T> values;
    const double EPS = 1e-6;
    std::string argName = "";

    void buildValues()
    {
        static_assert(!std::is_same<std::string, T>::value);
        T tmp = start;
        while ((tmp - end) <= EPS)
        {
            values.push_back(tmp);
            tmp += stride;
        }
    }

    Argument(std::string _argName, std::vector<T> _values)
    {
        this->argName = _argName;
        this->values = _values;
    }

    Argument(std::string _argName, T _start, T _end, T _stride)
    {
        this->argName = _argName;
        this->start = _start;
        this->end = _end;
        this->stride = _stride;
        this->buildValues();
    }
};
#endif
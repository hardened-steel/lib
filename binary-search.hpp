#include <iterator>


template<class Begin, class End, class Value>
auto find(Begin begin, End end, const Value& value)
{
    auto distance = std::distance(begin, end);

    while (distance > 0) {
        auto step = distance / 2;
        auto it = begin + step;
 
        if (*it < value) {
            begin = ++it;
            distance -= step + 1;
        } else {
            distance = step;
        }
    }
    if (begin != end && *begin == value) {
        return begin;
    }
    return end;
}

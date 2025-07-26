#pragma once

#include <type_traits>

template<typename T1, typename T2>
std::common_type_t<T1, T2> max(T1 a, T2 b)
{
    return b < a ? a : b;
}

template<typename T1, typename T2>
auto max2(T1 a, T2 b) -> typename std::decay_t<decltype(true?a:b)>
{
    return b < a ? a : b;
}
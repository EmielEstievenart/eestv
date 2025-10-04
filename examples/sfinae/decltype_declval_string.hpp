#pragma once

#include <iostream>
#include <string>
#include <type_traits>

namespace eestv
{
struct A
{

    int value {};

    A(int input) : value {input} {};

    std::string operator+(A& rhs) { return std::to_string(this->value + rhs.value); }
};

template <typename T> auto add(T& first, T& second) -> decltype(first + second)
{
    return first + second;
}

template <typename T> struct IsStringType : public std::false_type
{
};

template <> struct IsStringType<std::string> : public std::true_type
{
};

void do_magic_addition()
{
    A first {3};
    A second {5};

    std::cout << "The result is: " << add(first, second) << "\n";
    static_assert(IsStringType<decltype(add(first, second))>::value, "This isn't a string");
}

}
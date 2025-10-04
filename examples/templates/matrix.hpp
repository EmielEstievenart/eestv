#pragma once
#include "vector.hpp"

namespace eestv
{
template <typename T> class Matrix
{
    Vector<T> v[4];

    public:

    friend Vector<T> operator*<>(const Matrix<T>& matrix, const Vector<T>& vector)
    {
        return vec

    }

};
}

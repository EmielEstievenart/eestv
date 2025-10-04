#pragma once

namespace eestv
{
template <typename T> class Matrix;

template <typename T> class Vector
{
    T v[4];

public:
    friend Vector operator* <>(const Matrix<T>& matrix, const Vector<T>& vector) { }
};
} // namespace eestv

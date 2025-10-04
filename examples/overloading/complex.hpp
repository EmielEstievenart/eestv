#pragma once

namespace eestv
{
class Complex
{
public:
    Complex(float real, float imaginary);

    Complex operator+(Complex& b) const { return Complex {this->_real + b._real, this->_imaginary + b._imaginary}; }

private:
    float _real {};
    float _imaginary {};
};
}
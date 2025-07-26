#pragma once

#include <iostream>
#include <string>

/**
 * @brief Second templated executable with different behavior
 * 
 * This class demonstrates another approach to templated executables,
 * with different execution logic and functionality.
 */
template<typename T>
class TemplatedExecutable2 {
private:
    T value;
    T multiplier;
    std::string identifier;

public:
    /**
     * @brief Construct a new Templated Executable 2 object
     * Creates an executable that multiplies a value by a multiplier
     * 
     * @param initial_value The initial value to store
     * @param multiplication_factor The factor to multiply by during execution  
     * @param name The identifier for this executable
     * 
     * Note: Parameters are intentionally of the same type T to demonstrate templates.
     * The parameter names clearly indicate their purpose to avoid confusion.
     */
    // NOLINTNEXTLINE(readability-identifier-length,bugprone-easily-swappable-parameters)
    TemplatedExecutable2(T initial_value, T multiplication_factor, const std::string& name = "Executable2") 
        : value(initial_value), multiplier(multiplication_factor), identifier(name) {}

    /**
     * @brief Execute the operation - multiplies value by multiplier
     * 
     * @return T The result of value * multiplier
     */
    T execute() {
        std::cout << identifier << " executing: " << value << " * " << multiplier;
        T result = value * multiplier;
        std::cout << " = " << result << '\n';
        return result;
    }

    /**
     * @brief Execute with an additional parameter
     * 
     * @param additional Additional value to add to the result
     * @return T The result of (value * multiplier) + additional
     */
    T executeWithAddition(T additional) {
        std::cout << identifier << " executing with addition: (" << value 
                  << " * " << multiplier << ") + " << additional;
        T result = (value * multiplier) + additional;
        std::cout << " = " << result << '\n';
        return result;
    }

    /**
     * @brief Get the current value
     * 
     * @return T The current value
     */
    T getValue() const {
        return value;
    }

    /**
     * @brief Get the multiplier
     * 
     * @return T The multiplier
     */
    T getMultiplier() const {
        return multiplier;
    }

    /**
     * @brief Set a new value
     * 
     * @param newValue The new value to set
     */
    void setValue(const T& newValue) {
        value = newValue;
    }

    /**
     * @brief Set a new multiplier
     * 
     * @param newMultiplier The new multiplier to set
     */
    void setMultiplier(const T& newMultiplier) {
        multiplier = newMultiplier;
    }

    /**
     * @brief Get the identifier
     * 
     * @return std::string The identifier
     */
    std::string getIdentifier() const {
        return identifier;
    }
};

// Convenience type aliases for common uses
using IntExecutable2 = TemplatedExecutable2<int>;
using DoubleExecutable2 = TemplatedExecutable2<double>;
using FloatExecutable2 = TemplatedExecutable2<float>;

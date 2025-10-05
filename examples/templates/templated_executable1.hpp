#pragma once

#include <iostream>
#include <string>

/**
 * @brief First templated executable that demonstrates template usage
 * 
 * This class can work with different types and provides an execute method
 * that returns a result based on the template parameter.
 */
template <typename T>
class TemplatedExecutable1
{
private:
    T data;
    std::string name;

public:
    /**
     * @brief Construct a new Templated Executable 1 object
     * 
     * @param value The value to store
     * @param itemName The name of this executable
     */
    TemplatedExecutable1(T value, const std::string& itemName = "Executable1") : data(value), name(itemName) { }

    /**
     * @brief Execute the operation and return a result
     * 
     * @return T The processed data
     */
    T execute()
    {
        std::cout << name << " executing with data: " << data << '\n';
        // Perform some operation - in this case, just return the data
        // but could be modified for different behaviors
        return data;
    }

    /**
     * @brief Get the stored data
     * 
     * @return T The data
     */
    T getData() const { return data; }

    /**
     * @brief Get the name of this executable
     * 
     * @return std::string The name
     */
    std::string getName() const { return name; }

    /**
     * @brief Set new data
     * 
     * @param newData The new data to store
     */
    void setData(const T& newData) { data = newData; }
};

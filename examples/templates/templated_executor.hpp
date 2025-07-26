#pragma once

#include <iostream>

/**
 * @brief A templated executor that can process different types of executable items
 * 
 * This class demonstrates template usage by providing a generic execution framework
 * that can work with various types that implement an execute() method.
 */
class TemplatedExecutor {
public:
    /**
     * @brief Execute a templated item and return its result
     * 
     * @tparam T The type of the executable item
     * @param item The item to execute
     * @return auto The result of the execution
     */
    template<typename T>
    auto execute(T&& item) -> decltype(std::forward<T>(item).execute()) {
        std::cout << "Executing item of type: " << typeid(T).name() << '\n';
        return std::forward<T>(item).execute();
    }

    /**
     * @brief Execute multiple items in sequence
     * 
     * @tparam T First item type
     * @tparam U Second item type
     * @param item1 First item to execute
     * @param item2 Second item to execute
     */
    template<typename T, typename U>
    void executeSequence(T&& item1, U&& item2) {
        std::cout << "=== Executing Sequence ===\n";
        auto result1 = execute(std::forward<T>(item1));
        auto result2 = execute(std::forward<U>(item2));
        
        std::cout << "First result: " << result1 << '\n';
        std::cout << "Second result: " << result2 << '\n';
        std::cout << "=========================\n";
    }

    /**
     * @brief Process an item with a custom operation
     * 
     * @tparam T The type of the executable item
     * @tparam Func The type of the function to apply
     * @param item The item to process
     * @param func The function to apply to the result
     * @return auto The result after applying the function
     */
    template<typename T, typename Func>
    auto processWithFunction(T&& item, Func&& func) -> decltype(std::forward<Func>(func)(std::forward<T>(item).execute())) {
        std::cout << "Processing item with custom function...\n";
        auto result = std::forward<T>(item).execute();
        return std::forward<Func>(func)(result);
    }
};

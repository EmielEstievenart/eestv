#include <iostream>
#include <string>

#include "templated_executor.hpp"
#include "templated_executable1.hpp"
#include "templated_executable2.hpp"
#include "max.hpp"

// Constants to avoid magic numbers
constexpr int SAMPLE_INT_VALUE           = 42;
constexpr int EXEC2_VALUE                = 5;
constexpr int EXEC2_MULTIPLIER           = 3;
constexpr double SAMPLE_DOUBLE_VALUE     = 2.5;
constexpr double DOUBLE_MULTIPLIER       = 4.0;
constexpr int ALIAS_INT_VALUE            = 100;
constexpr double PI_VALUE                = 3.14;
constexpr double ALIAS_DOUBLE_MULTIPLIER = 2.0;
constexpr int COMPLEX_VALUE              = 10;
constexpr int COMPLEX_MULTIPLIER         = 5;
constexpr int ADDITION_VALUE             = 15;

int main()
{

    std::cout << "=== Template Example ===\n";

    double a1 = 3.14;
    double b1 = 2.71;

    auto maxValue = max(a1, b1);

    auto maxValue2 = max2(a1, b1);

    std::cout << "=== Template Demonstration ===\n\n";

    // Create an executor
    TemplatedExecutor executor;

    // Create some templated executables with different types
    TemplatedExecutable1<int> intExec1(SAMPLE_INT_VALUE, "IntegerExec1");
    TemplatedExecutable1<std::string> stringExec1("Hello", "StringExec1");

    TemplatedExecutable2<int> intExec2(EXEC2_VALUE, EXEC2_MULTIPLIER, "IntegerExec2");
    TemplatedExecutable2<double> doubleExec2(SAMPLE_DOUBLE_VALUE, DOUBLE_MULTIPLIER, "DoubleExec2");

    std::cout << "1. Individual Executions:\n";
    std::cout << "-------------------------\n";

    // Execute individual items
    auto result1 = executor.execute(intExec1);
    std::cout << "Result from IntExec1: " << result1 << "\n\n";

    auto result2 = executor.execute(stringExec1);
    std::cout << "Result from StringExec1: " << result2 << "\n\n";

    auto result3 = executor.execute(intExec2);
    std::cout << "Result from IntExec2: " << result3 << "\n\n";

    auto result4 = executor.execute(doubleExec2);
    std::cout << "Result from DoubleExec2: " << result4 << "\n\n";

    std::cout << "2. Sequence Execution:\n";
    std::cout << "----------------------\n";

    // Execute items in sequence (this demonstrates the template system)
    executor.executeSequence(intExec1, intExec2);
    std::cout << '\n';

    executor.executeSequence(stringExec1, doubleExec2);
    std::cout << '\n';

    std::cout << "3. Custom Function Processing:\n";
    std::cout << "------------------------------\n";

    // Demonstrate processing with custom functions
    auto doubleFunction = [](int value)
    {
        std::cout << "Doubling the result: " << value << " -> ";
        return value * 2;
    };

    auto processedResult = executor.processWithFunction(intExec1, doubleFunction);
    std::cout << processedResult << "\n\n";

    auto stringProcessor = [](const std::string& str)
    {
        std::cout << "Processing string: '" << str << "' -> ";
        return str + " World!";
    };

    auto processedString = executor.processWithFunction(stringExec1, stringProcessor);
    std::cout << "'" << processedString << "'\n\n";

    std::cout << "4. Using Type Aliases:\n";
    std::cout << "----------------------\n";

    // Demonstrate using the convenience type aliases
    TemplatedExecutable1<int> aliasedIntExec(ALIAS_INT_VALUE, "AliasedInt");
    TemplatedExecutable2<double> aliasedDoubleExec(PI_VALUE, ALIAS_DOUBLE_MULTIPLIER, "AliasedDouble");

    executor.executeSequence(aliasedIntExec, aliasedDoubleExec);
    std::cout << '\n';

    std::cout << "5. Advanced Usage:\n";
    std::cout << "------------------\n";

    // Demonstrate more complex scenarios
    TemplatedExecutable2<int> complexExec(COMPLEX_VALUE, COMPLEX_MULTIPLIER, "ComplexExec");

    // Show normal execution
    std::cout << "Normal execution: ";
    auto normalResult = complexExec.execute();

    // Show execution with addition (demonstrating method overloading)
    std::cout << "Execution with addition: ";
    auto additionResult = complexExec.executeWithAddition(ADDITION_VALUE);

    std::cout << "Normal result: " << normalResult << ", Addition result: " << additionResult << "\n\n";

    std::cout << "=== Demo Complete ===\n";

    return 0;
}

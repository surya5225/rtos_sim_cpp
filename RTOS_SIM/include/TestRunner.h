// TestRunner.h — Built-in unit tests (no external framework required).
#pragma once

#include <string>
#include <vector>

namespace rtos {

class TestRunner {
public:
    struct Result { std::string name; bool passed; std::string message; };

    void runAll();
    void printReport() const;
    int passedCount() const;
    int failedCount() const;

private:
    void addResult(const std::string& name, bool passed,
                   const std::string& message = "");

    std::vector<Result> results_;
};

} // namespace rtos
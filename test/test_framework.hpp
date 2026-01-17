#ifndef TEST_FRAMEWORK_HPP
#define TEST_FRAMEWORK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cassert>

// Простой тестовый фреймворк
class TestFramework {
private:
    struct TestCase {
        std::string name;
        std::function<void()> test_func;
    };
    
    std::vector<TestCase> tests;
    int passed = 0;
    int failed = 0;
    
public:
    void addTest(const std::string& name, std::function<void()> test_func) {
        tests.push_back({name, test_func});
    }
    
    void runAll() {
        std::cout << "Running " << tests.size() << " test(s)...\n" << std::endl;
        
        for (const auto& test : tests) {
            std::cout << "[TEST] " << test.name << " ... ";
            try {
                test.test_func();
                std::cout << "PASSED" << std::endl;
                passed++;
            } catch (const std::exception& e) {
                std::cout << "FAILED: " << e.what() << std::endl;
                failed++;
            } catch (...) {
                std::cout << "FAILED: Unknown exception" << std::endl;
                failed++;
            }
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Tests passed: " << passed << std::endl;
        std::cout << "Tests failed: " << failed << std::endl;
        std::cout << "Total tests: " << (passed + failed) << std::endl;
        std::cout << "========================================\n" << std::endl;
    }
    
    int getFailedCount() const { return failed; }
};

// Макросы для удобства
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Assertion failed: " #condition); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("Assertion failed: " #condition " should be false"); \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            throw std::runtime_error("Assertion failed: expected " + std::to_string(expected) + \
                                   ", got " + std::to_string(actual)); \
        } \
    } while(0)

#define ASSERT_STREQ(expected, actual) \
    do { \
        std::string exp_str = (expected); \
        std::string act_str = (actual); \
        if (exp_str != act_str) { \
            throw std::runtime_error("Assertion failed: expected \"" + exp_str + \
                                   "\", got \"" + act_str + "\""); \
        } \
    } while(0)

#define ASSERT_THROWS(statement, exception_type) \
    do { \
        bool caught = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) { \
            throw std::runtime_error("Expected " #exception_type " but got different exception"); \
        } \
        if (!caught) { \
            throw std::runtime_error("Expected exception " #exception_type " was not thrown"); \
        } \
    } while(0)

#endif // TEST_FRAMEWORK_HPP


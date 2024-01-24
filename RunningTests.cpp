// Inspired by Chapter 6: Running Tests
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/test/

#include <iostream>
#include <vector>

using namespace std;

void first()
{
    cout << "First" << endl;
}

void second()
{
    cout << "Second" << endl;
}

void third()
{
    cout << "Third" << endl;
}

void zero(int x)
{
    cout << "Zero (" << x << ")" << endl;
}

void first_example()
{
    cout << "=== Calling functions sequentially ===" << endl;
    vector<void(*)()> everything = { first, second, third };
    // Wouldn't work => everything.push_back(zero);

    for (const auto func : everything)
    {
        func();
    }
}

int sign(int value)
{
    if (value < 0)
        return -1;
    else
        return 1;
}

inline void assert(bool success)
{
    if (!success)
        throw -1;
}

void test_sign_negative()
{
    assert(sign(-3) == -1);
}

void test_sign_positive()
{
    assert(sign(19) == 1);
}

void test_sign_zero()
{
    assert(sign(0) == 0);
}

void test_sign_error()
{
    throw exception("random error");
}

void second_example()
{
    cout << "=== Explicit test functions ===" << endl;
    vector<void(*)()> all_tests = { test_sign_negative, test_sign_positive, test_sign_zero, test_sign_error };
    int pass = 0, fail = 0, error = 0;
    for (const auto func : all_tests)
    {
        try
        {
            func();
            pass++;
        }
        catch (int assertion_error)
        {
            fail++;
        }
        catch (const exception& e)
        {
            error++;
        }
    }
    cout << "Tests Passed: " << pass << endl;
    cout << "Tests Failed: " << fail << endl;
    cout << "Tests with Error: " << error << endl;
}

void third_example()
{
    cout << "=== Using lambdas ===" << endl;
    vector<void(*)()> all_tests = 
    {
        []() // test_sign_negative
        {
            assert(sign(-3) == -1);
        },

        []() // test_sign_positive
        {
            assert(sign(19) == 1);
        },

        []() // test_sign_zero
        {
            assert(sign(0) == 0);
        },

        []() // test_sign_error
        {
            throw exception("random error");
        },
    };

    int pass = 0, fail = 0, error = 0;
    for (const auto func : all_tests)
    {
        try
        {
            func();
            pass++;
        }
        catch (int assertion_error)
        {
            fail++;
        }
        catch (const exception& e)
        {
            error++;
        }
    }
    cout << "Tests Passed: " << pass << endl;
    cout << "Tests Failed: " << fail << endl;
    cout << "Tests with Error: " << error << endl;
}

void tests_main()
{
    cout << "Running Tests" << endl;
    first_example();
    second_example();
    third_example();
}
// Inspired by Chapter 7: An Interpreter
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/interp/

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

int eval(json &expr);

int eval_abs(json &expr)
{
    assert(expr.size() == 2);
    int val = expr[1].get<int>();
    return abs(val);
}

int eval_add(json &expr)
{
    assert(expr.size() == 3);
    int left = eval(expr[1]);
    int right = eval(expr[2]);
    return left + right;
}

int eval(json &expr)
{
    if (expr.is_number_integer())
    {
        return expr.get<int>();
    }

    assert(expr.is_array());
    assert(expr.size() > 0);

    string op = expr[0].get<string>();
    if (op == "abs")
    {
        return eval_abs(expr);
    }
    else if (op == "add")
    {
        return eval_add(expr);
    }

    assert(!"Unknown operation");
}

void interpreter_main()
{
    cout << "Interpreter" << endl;

    auto program1 = json::parse(R"(["add", ["abs", -3], 2])");
    auto result = eval(program1);

    cout << "=> " << result << endl;
}
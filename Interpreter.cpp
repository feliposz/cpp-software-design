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

int eval(json &expr, map<string, int> &env);

int eval_abs(json &expr, map<string, int> &env)
{
    assert(expr.size() == 2);
    int val = eval(expr[1], env);
    return abs(val);
}

int eval_add(json &expr, map<string, int> &env)
{
    assert(expr.size() == 3);
    int left = eval(expr[1], env);
    int right = eval(expr[2], env);
    return left + right;
}

int eval_get(json &expr, map<string, int> &env)
{
    assert(expr.size() == 2);
    assert(expr[1].is_string());
    string identifier = expr[1].get<string>();
    assert(env.count(identifier) > 0);
    return env[identifier];
}

int eval_set(json &expr, map<string, int> &env)
{
    assert(expr.size() == 3);
    assert(expr[1].is_string());
    string identifier = expr[1].get<string>();
    int value = eval(expr[2], env);
    env[identifier] = value;
    return value;
}

int eval_seq(json &expr, map<string, int> &env)
{
    assert(expr.size() > 1);
    int result = 0;
    for (int i = 1; i < expr.size(); i++)
    {
        result = eval(expr[i], env);
    }
    return result;
}

int eval(json &expr, map<string, int> &env)
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
        return eval_abs(expr, env);
    }
    else if (op == "add")
    {
        return eval_add(expr, env);
    }
    else if (op == "seq")
    {
        return eval_seq(expr, env);
    }
    else if (op == "set")
    {
        return eval_set(expr, env);
    }
    else if (op == "get")
    {
        return eval_get(expr, env);
    }

    assert(!"Unknown operation");
}

void interpreter_main()
{
    cout << "Interpreter" << endl;

    map<string, int> env1;
    auto program1 = json::parse(R"(["add", ["abs", -3], 2])");
    cout << "=> " << eval(program1, env1) << endl;

    map<string, int> env2;
    auto program2 = json::parse(R"(
        [
            "seq",
            ["set", "alpha", 1],
            ["set", "beta", 2],
            ["add", ["get", "alpha"], ["get", "beta"]]
        ]
    )");
    cout << "=> " << eval(program2, env2) << endl;

}
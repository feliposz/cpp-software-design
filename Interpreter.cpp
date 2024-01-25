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

int eval_print(json &expr, map<string, int> &env)
{
    assert(expr.size() > 1);
    for (int i = 1; i < expr.size(); i++)
    {
        if (expr[i].is_string())
        {
            cout << expr[i].get<string>();
        }
        else
        {
            cout << eval(expr[i], env);
        }
        cout << " ";
    }
    cout << endl;
    return 0;
}

int eval_repeat(json &expr, map<string, int> &env)
{
    assert(expr.size() == 3);
    assert(expr[1].is_number_integer());
    int countdown = expr[1].get<int>();
    int result = 0;
    while (countdown-- > 0)
    {
        result = eval(expr[2], env);
    }
    return 0;
}

int eval_if(json &expr, map<string, int> &env)
{
    assert(expr.size() == 4);
    if (eval(expr[1], env))
    {
        return eval(expr[2], env);
    }
    else
    {
        return eval(expr[3], env);
    }
}

int eval_leq(json &expr, map<string, int> &env)
{
    assert(expr.size() == 3);
    int left = eval(expr[1], env);
    int right = eval(expr[2], env);
    return left <= right;
}

map<string, int(*)(json&, map<string, int>&)> ops = {
    { "abs", eval_abs },
    { "add", eval_add },
    { "seq", eval_seq },
    { "set", eval_set },
    { "get", eval_get },
    { "print", eval_print },
    { "repeat", eval_repeat },
    { "if", eval_if},
    { "leq", eval_leq},
};

int eval(json &expr, map<string, int> &env)
{
    if (expr.is_number_integer())
    {
        return expr.get<int>();
    }

    assert(expr.is_array());
    assert(expr.size() > 0);

    string op = expr[0].get<string>();

    if (ops.count(op))
    {
        int result = ops[op](expr, env);
        //cout << "\t[DEBUG]\top = " << op << "\tresult = " << result << endl;
        return result;
    }
    
    assert(!"Unknown operation");    
}

void interpreter_main()
{
    cout << "Interpreter" << endl;

    {
        map<string, int> env;
        auto program = json::parse(R"(["add", ["abs", -3], 2])");
        int result = eval(program, env);
        cout << "=> " << result << endl;
    }

    {
        map<string, int> env;
        auto program = json::parse(R"(
            [
                "seq",
                ["set", "alpha", 1],
                ["set", "beta", 2],
                ["add", ["get", "alpha"], ["get", "beta"]]
            ]
        )");
        int result = eval(program, env);
        cout << "=> " << result << endl;
    }

    {
        map<string, int> env;
        auto program = json::parse(R"(
            [
                "seq",
                ["set", "a", 1],
                ["print", "initial", ["get", "a"]],
                [
                    "repeat", 4,
                    [
                        "seq",
                        ["set", "a", ["add", ["get", "a"], ["get", "a"]]],
                        ["if",
                            ["leq", ["get", "a"], 10],
                            ["print", "small", ["get", "a"]],
                            ["print", "large", ["get", "a"]]
                        ]
                    ]
                ]
            ]
        )");
        int result = eval(program, env);
        cout << "=> " << result << endl;
    }
}
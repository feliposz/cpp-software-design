// Inspired by Chapter 7: An Interpreter and Chapter 8: Functions and Closures
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/interp/
// https://third-bit.com/sdxpy/func/

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

struct environment
{
    vector<map<string, json>> stack;

    environment()
    {
        push();
    }

    void push()
    {
        stack.push_back({});
    }

    void pop()
    {
        stack.pop_back();
    }

    void set(string identifier, json value)
    {
        for (auto s = stack.rbegin(); s != stack.rend(); s++)
        {
            if ((*s).count(identifier))
            {
                (*s)[identifier] = value;
                return;
            }
        }
        stack.back()[identifier] = value;
    };

    json get(string identifier)
    {
        for (auto s = stack.rbegin(); s != stack.rend(); s++)
        {
            if ((*s).count(identifier))
            {
                return (*s)[identifier];
            }
        }
        throw exception("undeclared");
    }
};

json eval(json &expr, environment &env);

json eval_abs(json &expr, environment &env)
{
    assert(expr.size() == 2);
    int val = eval(expr[1], env);
    return abs(val);
}

json eval_add(json &expr, environment &env)
{
    assert(expr.size() == 3);
    int left = eval(expr[1], env);
    int right = eval(expr[2], env);
    return left + right;
}

json eval_get(json &expr, environment &env)
{
    assert(expr.size() == 2);
    assert(expr[1].is_string());
    string identifier = expr[1].get<string>();
    return env.get(identifier);
}

json eval_set(json &expr, environment &env)
{
    assert(expr.size() == 3);
    assert(expr[1].is_string());
    string identifier = expr[1].get<string>();
    json value = eval(expr[2], env);
    env.set(identifier, value);
    return value;
}

json eval_seq(json &expr, environment &env)
{
    assert(expr.size() > 1);
    json result;
    for (int i = 1; i < expr.size(); i++)
    {
        result = eval(expr[i], env);
    }
    return result;
}

json eval_print(json &expr, environment &env)
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
    json result;
    return result;
}

json eval_repeat(json &expr, environment &env)
{
    assert(expr.size() == 3);
    assert(expr[1].is_number_integer());
    int countdown = expr[1].get<int>();
    json result;
    while (countdown-- > 0)
    {
        result = eval(expr[2], env);
    }
    return result;
}

json eval_if(json &expr, environment &env)
{
    assert(expr.size() == 4);
    if (eval(expr[1], env).get<int>())
    {
        return eval(expr[2], env);
    }
    else
    {
        return eval(expr[3], env);
    }
}

json eval_leq(json &expr, environment &env)
{
    assert(expr.size() == 3);
    int left = eval(expr[1], env);
    int right = eval(expr[2], env);
    return left <= right;
}

json eval_while(json &expr, environment &env)
{
    assert(expr.size() == 3);
    json result;
    while (eval(expr[1], env).get<int>())
    {
        result = eval(expr[2], env);
    }
    return result;
}

json eval_func(json &expr, environment &env)
{
    assert(expr.size() == 3);
    return expr;
}

json eval_call(json &expr, environment &env)
{
    assert(expr.size() > 2);
    assert(expr[1].is_string());
    string name = expr[1];
    vector<json> values;
    for (int i = 2; i < expr.size(); i++)
    {
        values.push_back(eval(expr[i], env));
    }
    json func = env.get(name);
    if (func.is_array() && func[0] == "func")
    {
        auto params = func[1];
        auto body = func[2];
        assert(values.size() == params.size());
        env.push();
        for (int i = 0; i < params.size(); i++)
        {
            env.set(params[i], values[i]);
        }
        json result = eval(body, env);
        env.pop();
        return result;
    }
    throw exception("unknown function");
}

map<string, json(*)(json&, environment&)> ops = {
    { "abs", eval_abs },
    { "add", eval_add },
    { "seq", eval_seq },
    { "set", eval_set },
    { "get", eval_get },
    { "print", eval_print },
    { "repeat", eval_repeat },
    { "if", eval_if},
    { "leq", eval_leq},
    { "while", eval_while},
    { "func", eval_func},
    { "call", eval_call},
};

json eval(json &expr, environment &env)
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
        json result = ops[op](expr, env);
        //cout << "\t[DEBUG]\top = " << op << "\tresult = " << result << endl;
        return result;
    }
    
    throw exception("Unknown operation");
}

void interpreter_main()
{
    cout << "Interpreter" << endl;

    {
        environment env;
        auto program = json::parse(R"(["add", ["abs", -3], 2])");
        json result = eval(program, env);
        cout << "=> " << result << endl;
    }

    {
        environment env;
        auto program = json::parse(R"(
            [
                "seq",
                ["set", "alpha", 1],
                ["set", "beta", 2],
                ["add", ["get", "alpha"], ["get", "beta"]]
            ]
        )");
        json result = eval(program, env);
        cout << "=> " << result << endl;
    }

    {
        environment env;
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
        json result = eval(program, env);
        cout << "=> " << result << endl;
    }

    {
        environment env;
        auto program = json::parse(R"(
            [
                "seq",
                ["set", "a", -5],
                ["print", "initial", ["get", "a"]],
                [
                    "while", ["leq", ["get", "a"], 5],
                    [
                        "seq",
                        ["set", "a", ["add", ["get", "a"], 1]],
                        ["print", "a =", ["get", "a"]]
                    ]
                ]
            ]
        )");
        json result = eval(program, env);
        cout << "=> " << result << endl;
    }

    {
        environment env;
        auto program = json::parse(R"(
            ["seq",
              ["set", "double",
                ["func", ["num"],
                  ["add", ["get", "num"], ["get", "num"]]
                ]
              ],
              ["set", "a", 1],
              ["repeat", 4, ["seq",
                ["set", "a", ["call", "double", ["get", "a"]]],
                ["print", ["get", "a"]]
              ]]
            ]
        )");
        json result = eval(program, env);
        cout << "=> " << result << endl;
    }
}
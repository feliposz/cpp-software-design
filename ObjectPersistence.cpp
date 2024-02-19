// Inspired by Chapter 16: Object Persistence
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/persist/

#include <assert.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

enum PersistType
{
    PT_bool,
    PT_int,
    PT_float,
    PT_string,
    PT_list,
    PT_dict,
};

struct PersistValue
{
    PersistType type;
    union
    {
        bool bool_value;
        long int_value;
        double float_value;
    };
    string string_value;
    vector<PersistValue> list_value;
    map<string, PersistValue> dict_value;
};

bool operator!=(PersistValue const &left, PersistValue const &right);

bool operator==(PersistValue const &left, PersistValue const &right)
{
    if (left.type != right.type)
    {
        return false;
    }
    switch (right.type)
    {
        case PT_bool: return left.bool_value == right.bool_value;
        case PT_float: return left.float_value == right.float_value;
        case PT_int: return left.int_value == right.int_value;
        case PT_string: return left.string_value == right.string_value;

        case PT_list:
        {
            int count = left.list_value.size();
            if (count != right.list_value.size())
            {
                return false;
            }
            for (int i = 0; i < count; i++)
            {
                if (left.list_value[i] != right.list_value[i])
                {
                    return false;
                }
            }
            return true;
        }

        case PT_dict: 
        {
            int count = left.dict_value.size();
            if (count != right.dict_value.size())
            {
                return false;
            }
            for (const auto &[key, value] : left.dict_value)
            {
                if ((right.dict_value.count(key) == 0) || (right.dict_value.at(key) != value))
                {
                    return false;
                }
            }
            return true;
        }
    }
    throw exception("invalid data type");
}

bool operator!=(PersistValue const &left, PersistValue const &right)
{
    return !(left == right);
}

PersistValue val(bool value)
{
    PersistValue result;
    result.type = PT_bool;
    result.bool_value = value;
    return result;
}

PersistValue val(int value)
{
    PersistValue result;
    result.type = PT_int;
    result.int_value = value;
    return result;
}

PersistValue val(long value)
{
    PersistValue result;
    result.type = PT_int;
    result.int_value = value;
    return result;
}

PersistValue val(double value)
{
    PersistValue result;
    result.type = PT_float;
    result.float_value = value;
    return result;
}

PersistValue val(const string &value)
{
    PersistValue result;
    result.type = PT_string;
    result.string_value = value;
    return result;
}

PersistValue val(const char *value)
{
    PersistValue result;
    result.type = PT_string;
    result.string_value = value;
    return result;
}

PersistValue list(const vector<PersistValue> &value)
{
    PersistValue result;
    result.type = PT_list;
    result.list_value = value;
    return result;
}


PersistValue dict(const map<string, PersistValue> &value)
{
    PersistValue result;
    result.type = PT_dict;
    result.dict_value = value;
    return result;
}

int count_newlines(const string &s)
{
    int newlines = 0;
    for (const auto c : s)
    {
        if (c == '\n') newlines++;
    }
    return newlines;
}

void save(ostream &writer, const PersistValue &thing)
{
    switch (thing.type)
    {
        case PT_bool:
            writer << "bool:" << (thing.bool_value ? "True" : "False") << endl;
            break;
        case PT_float:
            writer << "float:" << thing.float_value << endl;
            break;
        case PT_int:
            writer << "int:" << thing.int_value << endl;
            break;
        case PT_string:
            writer << "str:" << (count_newlines(thing.string_value) + 1) << endl;
            writer << thing.string_value.c_str() << endl;
            break;
        case PT_list:
            writer << "list:" << thing.list_value.size() << endl;
            for (const auto &item : thing.list_value)
            {
                save(writer, item);
            }
            break;
        case PT_dict:
            writer << "dict:" << thing.dict_value.size() << endl;
            for (const auto &[key, value] : thing.dict_value)
            {
                save(writer, val(key));
                save(writer, value);
            }
            break;
    }
}

PersistValue load(istream &reader)
{
    string line;
    reader >> line;
    size_t sep_offset = line.find(":");
    if (sep_offset == string::npos)
    {
        throw exception("invalid format");
    }
    string type = line.substr(0, sep_offset);
    string content = line.substr(sep_offset + 1);
    if (type == "bool")
    {
        return val(content == "True");
    }
    else if (type == "int")
    {
        return val(atol(content.c_str()));
    }
    else if (type == "float")
    {
        return val(atof(content.c_str()));
    }
    else if (type == "str")
    {
        string data;
        int count = atol(content.c_str());
        for (int i = 0; i < count; i++)
        {
            string line;
            reader >> line;
            data += line;
            if (i < count - 1)
            {
                data += "\n";
            }
        }
        return val(data);
    }
    else if (type == "list")
    {
        int count = atol(content.c_str());
        vector<PersistValue> data(count);
        for (int i = 0; i < count; i++)
        {
            data[i] = load(reader);
        }
        return list(data);
    }
    else if (type == "dict")
    {
        int count = atol(content.c_str());
        map<string,PersistValue> data;
        for (int i = 0; i < count; i++)
        {
            PersistValue key = load(reader);
            PersistValue value = load(reader);
            assert(key.type == PT_string);
            data[key.string_value] = value;
        }
        return dict(data);
    }
    throw exception("invalid data type");
}

void test_save_list_flat()
{
    string expect =
        "list:4\n"
        "bool:False\n"
        "float:3.14\n"
        "str:1\n"
        "hello\n"
        "dict:2\n"
        "str:1\n"
        "left\n"
        "int:1\n"
        "str:1\n"
        "right\n"
        "list:2\n"
        "int:2\n"
        "int:3\n";

    auto data = list({
       val(false),
       val(3.14),
       val("hello"),
       dict({ {"left", val(1)}, {"right", list({val(2), val(3)}) } }) });

    stringstream ss;
    save(ss, data);
    assert(expect == ss.str());
}

void test_load_list_flat()
{
    string data =
        "list:4\n"
        "bool:False\n"
        "float:3.14\n"
        "str:1\n"
        "hello\n"
        "dict:2\n"
        "str:1\n"
        "left\n"
        "int:1\n"
        "str:1\n"
        "right\n"
        "list:2\n"
        "int:2\n"
        "int:3\n";

    auto expect = list({
       val(false),
       val(3.14),
       val("hello"),
       dict({ {"left", val(1)}, {"right", list({val(2), val(3)}) } }) });

    stringstream ss(data);
    PersistValue result = load(ss);
    assert(result == expect);
}

void persist_main()
{
    cout << "Object Persistence:" << endl;
    test_save_list_flat();
    test_load_list_flat();
    cout << "All tests passed!" << endl;
}
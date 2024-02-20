// Inspired by Chapter 16: Object Persistence
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/persist/

#include <assert.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <memory>

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
    PersistType type = PT_bool;
    // TODO: Is there a safe way to collapse these?
    shared_ptr<bool> bool_value;
    shared_ptr<long> int_value;
    shared_ptr<double> float_value;
    shared_ptr<string> string_value;
    shared_ptr<vector<PersistValue>> list_value;
    shared_ptr<map<string, PersistValue>> dict_value;
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
        case PT_bool: return *left.bool_value == *right.bool_value;
        case PT_float: return *left.float_value == *right.float_value;
        case PT_int: return *left.int_value == *right.int_value;
        case PT_string: return *left.string_value == *right.string_value;

        case PT_list:
        {
            size_t count = left.list_value->size();
            if (count != right.list_value->size())
            {
                return false;
            }
            for (int i = 0; i < count; i++)
            {
                if (left.list_value->at(i) != right.list_value->at(i))
                {
                    return false;
                }
            }
            return true;
        }

        case PT_dict: 
        {
            size_t count = left.dict_value->size();
            if (count != right.dict_value->size())
            {
                return false;
            }
            for (const auto &[key, value] : *left.dict_value)
            {
                if ((right.dict_value->count(key) == 0) || (right.dict_value->at(key) != value))
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
    result.bool_value = make_unique<bool>(value);
    return result;
}

PersistValue val(int value)
{
    PersistValue result;
    result.type = PT_int;
    result.int_value = make_unique<long>(value);
    return result;
}

PersistValue val(long value)
{
    PersistValue result;
    result.type = PT_int;
    result.int_value = make_unique<long>(value);
    return result;
}

PersistValue val(double value)
{
    PersistValue result;
    result.type = PT_float;
    result.float_value = make_unique<double>(value);
    return result;
}

PersistValue val(const string &value)
{
    PersistValue result;
    result.type = PT_string;
    result.string_value = make_unique<string>(value);
    return result;
}

PersistValue val(const char *value)
{
    PersistValue result;
    result.type = PT_string;
    result.string_value = make_unique<string>(value);
    return result;
}

PersistValue list(const vector<PersistValue> &value)
{
    PersistValue result;
    result.type = PT_list;
    result.list_value = make_unique<vector<PersistValue>>(value);
    return result;
}


PersistValue dict(const map<string, PersistValue> &value)
{
    PersistValue result;
    result.type = PT_dict;
    result.dict_value = make_unique<map<string, PersistValue>>(value);
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

void save(ostream &writer, const string &s)
{
    writer << "str:" << (count_newlines(s) + 1) << endl;
    writer << s << endl;
}

uint64_t id(const PersistValue &value)
{
    switch (value.type)
    {
        case PT_bool: return (uint64_t)value.bool_value.get();
        case PT_float: return (uint64_t)value.float_value.get();
        case PT_int: return (uint64_t)value.int_value.get();
        case PT_string: return (uint64_t)value.string_value.get();
        case PT_list: return (uint64_t)value.list_value.get();
        case PT_dict: return (uint64_t)value.dict_value.get();
    }
    throw exception("invalid data type");
}

void save(ostream &writer, const PersistValue &thing, bool aliasing = false, shared_ptr<set<uint64_t>> context = nullptr)
{
    string alias_str = "";
    if (aliasing)
    {
        if (!context)
        {
            context = make_shared<set<uint64_t>>();
        }
        uint64_t alias = id(thing);
        alias_str += to_string(alias);
        alias_str += ":";
        if (context->count(alias))
        {
            writer << "alias:" << alias_str << endl;
            return;
        }
        else
        {
            context->emplace(alias);
        }
    }
    switch (thing.type)
    {
        case PT_bool:
            writer << "bool:" << alias_str << (*thing.bool_value ? "True" : "False") << endl;
            break;
        case PT_float:
            writer << "float:" << alias_str << *thing.float_value << endl;
            break;
        case PT_int:
            writer << "int:" << alias_str << *thing.int_value << endl;
            break;
        case PT_string:
            writer << "str:" << alias_str << (count_newlines(*thing.string_value) + 1) << endl;
            writer << thing.string_value->c_str() << endl;
            break;
        case PT_list:
            writer << "list:" << alias_str << thing.list_value->size() << endl;
            for (const auto &item : *thing.list_value)
            {
                save(writer, item, aliasing, context);
            }
            break;
        case PT_dict:
            writer << "dict:" << alias_str << thing.dict_value->size() << endl;
            for (const auto &[key, value] : *thing.dict_value)
            {
                save(writer, key);
                save(writer, value, aliasing, context);
            }
            break;
    }
}

PersistValue load(istream &reader, bool aliasing = false, shared_ptr<map<uint64_t,PersistValue>> context = nullptr)
{
    bool destroy_context = false;
    string line;
    reader >> line;
    size_t sep_offset = line.find(":");
    if (sep_offset == string::npos)
    {
        throw exception("invalid format");
    }
    string type = line.substr(0, sep_offset);
    uint64_t alias = 0;

    if (aliasing)
    {
        if (!context)
        {
            context = make_shared<map<uint64_t, PersistValue>>();
        }
        size_t alias_offset = line.find(":", sep_offset + 1);
        if (sep_offset == string::npos)
        {
            throw exception("invalid aliasing format");
        }
        alias = atoll(line.substr(sep_offset + 1, alias_offset - sep_offset).c_str());
        sep_offset = alias_offset;
        if (type == "alias")
        {
            if (context->count(alias) == 0)
            {
                throw exception("invalid alias");
            }
            else
            {
                return context->at(alias);
            }
        }
    }

    string content = line.substr(sep_offset + 1);
    PersistValue result;

    if (type == "bool")
    {
        result = val(content == "True");
    }
    else if (type == "int")
    {
        result = val(atol(content.c_str()));
    }
    else if (type == "float")
    {
        result = val(atof(content.c_str()));
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
        result = val(data);
    }
    else if (type == "list")
    {
        int count = atol(content.c_str());
        vector<PersistValue> data(count);
        for (int i = 0; i < count; i++)
        {
            data[i] = load(reader, aliasing, context);
        }
        result = list(data);
    }
    else if (type == "dict")
    {
        int count = atol(content.c_str());
        map<string,PersistValue> data;
        for (int i = 0; i < count; i++)
        {
            PersistValue key = load(reader);
            PersistValue value = load(reader, aliasing, context);
            assert(key.type == PT_string);
            data[*key.string_value] = value;
        }
        result = dict(data);
    }
    else
    {
        throw exception("invalid data type");
    }

    if (aliasing)
    {
        context->emplace(alias, result);
    }

    return result;
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

    stringstream data_reader(data);
    PersistValue result = load(data_reader);
    assert(result == expect);
}

PersistValue roundtrip(PersistValue fixture)
{
    stringstream buffer;
    save(buffer, fixture, true);
    //cout << buffer.str();
    PersistValue result = load(buffer, true);
    return result;
}

void test_aliasing_no_aliasing()
{
    auto fixture = list({ val("a"),dict({{ "b", val(true)},{"7", dict({{"c", val("d")}}) }}) });
    auto result = roundtrip(fixture);
    assert(result == fixture);
}

void test_aliasing_shared_child()
{
    auto shared = list({ val("content") });
    auto fixture = list({ shared, shared });
    auto result = roundtrip(fixture);
    assert(result == fixture);
    assert(id(result.list_value->at(0)) == id(result.list_value->at(1)));
    result.list_value->at(0).list_value->at(0).string_value = make_shared<string>("changed");
    assert(*result.list_value->at(1).list_value->at(0).string_value == "changed");
}

void persist_main()
{
    cout << "Object Persistence:" << endl;
    test_save_list_flat();
    test_load_list_flat();
    test_aliasing_no_aliasing();
    test_aliasing_shared_child();
    cout << "All tests passed!" << endl;
}
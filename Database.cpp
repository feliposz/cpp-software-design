// Inspired by Chapter 18: A Database
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/db/

#include <assert.h>
#include <iostream>
#include <map>
#include <string>

using namespace std;

template<typename K, typename V>
class Database
{
public:
    virtual void add(const V &record) = 0;
    virtual V get(const K &key) = 0;
};

template<typename K, typename V>
class MemDb : public Database<K, V>
{
    map<K, V> data;
    K (*key_func)(const V &);
public:
    MemDb(K (*key_func)(const V &)): key_func(key_func)
    {
    }

    void add(const V &record) override
    {
        data[key_func(record)] = record;
    }

    V get(const K &key) override
    {
        return data[key];
    }
};

struct BasicRecord
{
    string name;
    int timestamp = 0;
    int readings[10] = {};
};

bool operator==(const BasicRecord &left, const BasicRecord &right)
{
    for (int i = 0; i < 10; i++)
    {
        if (left.readings[i] != right.readings[i])
        {
            return false;
        }
    }
    return left.name == right.name && left.timestamp == right.timestamp;
}

string get_key(const BasicRecord &record)
{
    return record.name;
}

void test_get_nothing_from_empty_db()
{
    MemDb<string, BasicRecord> db(get_key);
    BasicRecord result = db.get("something");
    BasicRecord empty;
    assert(result == empty);
}

void test_add_then_get()
{
    MemDb<string, BasicRecord> db(get_key);
    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    db.add(ex01);
    assert(db.get("ex01") == ex01);
}

void test_add_two_then_get_both()
{
    MemDb<string, BasicRecord> db(get_key);
    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    BasicRecord ex02{ "ex02", 67890, {3, 4} };
    db.add(ex01);
    db.add(ex02);
    assert(db.get("ex01") == ex01);
    assert(db.get("ex02") == ex02);
}

void test_add_then_overwrite()
{
    MemDb<string, BasicRecord> db(get_key);
    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    db.add(ex01);
    ex01.timestamp = 67890;
    db.add(ex01);
    assert(db.get("ex01") == ex01);
}

void database_main()
{
    cout << "Database:" << endl;
    test_get_nothing_from_empty_db();
    test_add_then_get();
    test_add_two_then_get_both();
    test_add_then_overwrite();
    cout << "All tests passed" << endl;
}
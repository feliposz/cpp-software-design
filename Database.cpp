// Inspired by Chapter 18: A Database
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/db/

#include <assert.h>
#include <iostream>
#include <fstream>
#include <map>
#include <string>

// requires: /std:c++17
#include <filesystem>

using namespace std;

struct BasicRecord
{
    string name;
    int timestamp = 0;
    int readings[10] = {};

    static const size_t max_name = 20;
    static const size_t packed_size = sizeof(size_t) + max_name + sizeof(timestamp) + sizeof(readings);

    string key() const
    {
        return name;
    }

    void pack(char *buffer) const
    {
        size_t *strlen_ptr = (size_t *)buffer;
        *strlen_ptr = name.length();
        strcpy_s(buffer + sizeof(size_t), max_name, name.c_str());
        int *timestamp_ptr = (int *)(buffer + sizeof(size_t) + max_name);
        *timestamp_ptr = timestamp;
        int *readings_ptr = (int *)(timestamp_ptr + 1);
        for (int i = 0; i < 10; i++)
        {
            readings_ptr[i] = readings[i];
        }
    }

    static BasicRecord unpack(char *buffer)
    {
        BasicRecord result;
        size_t *strlen_ptr = (size_t *)buffer;
        result.name = string(buffer + sizeof(size_t), *strlen_ptr);
        int *timestamp_ptr = (int *)(buffer + sizeof(size_t) + max_name);
        result.timestamp = *timestamp_ptr;
        int *readings_ptr = (int *)(timestamp_ptr + 1);
        for (int i = 0; i < 10; i++)
        {
            result.readings[i] = readings_ptr[i];
        }
        return result;
    }
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

class Database
{
public:
    virtual void add(const BasicRecord &record) = 0;
    virtual BasicRecord get(const string &key) = 0;
};

class MemDb : public Database
{
protected:
    map<string, BasicRecord> data;
public:

    void add(const BasicRecord &record) override
    {
        string key = record.key();
        data[key] = record;
    }

    BasicRecord get(const string &key) override
    {
        BasicRecord result = data[key];
        return result;
    }
};

class FileDb : public MemDb
{
    filesystem::path file_path;
public:
    FileDb(const filesystem::path &file_path): file_path(file_path)
    {
        load();
    }

    void add(const BasicRecord &record) override
    {
        MemDb::add(record);
        save();
    }

private:
    void load()
    {
        ifstream reader(file_path.c_str(), ios_base::binary);
        if (reader.is_open())
        {
            reader.seekg(0, ios_base::end);
            size_t total_size = (size_t)reader.tellg();
            char *buffer = new char[total_size];
            reader.seekg(0, ios_base::beg);
            reader.read(buffer, total_size);
            reader.close();
            size_t record_size = BasicRecord::packed_size;
            for (size_t offset = 0; offset < total_size; offset += record_size)
            {
                MemDb::add(BasicRecord::unpack(buffer + offset));
            }
            delete[] buffer;
        }
    }

    void save()
    {
        ofstream writer(file_path.c_str(), ios_base::binary);
        if (writer.is_open())
        {
            size_t record_size = BasicRecord::packed_size;
            size_t total_size = data.size() * record_size;
            size_t offset = 0;
            char *buffer = new char[total_size];
            for (const auto &[key, value] : data)
            {
                value.pack(buffer + offset);
                offset += record_size;
            }
            writer.write(buffer, total_size);
            writer.close();
            delete[] buffer;
        }
    }
};

void test_get_nothing_from_empty_db()
{
    MemDb db;
    BasicRecord result = db.get("something");
    BasicRecord empty;
    assert(result == empty);
}

void test_add_then_get()
{
    MemDb db;
    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    db.add(ex01);
    assert(db.get("ex01") == ex01);
}

void test_add_two_then_get_both()
{
    MemDb db;
    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    BasicRecord ex02{ "ex02", 67890, {3, 4} };
    db.add(ex01);
    db.add(ex02);
    assert(db.get("ex01") == ex01);
    assert(db.get("ex02") == ex02);
}

void test_add_then_overwrite()
{
    MemDb db;
    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    db.add(ex01);
    ex01.timestamp = 67890;
    db.add(ex01);
    assert(db.get("ex01") == ex01);
}

void test_filedb()
{
    filesystem::path db_file_path = filesystem::temp_directory_path().append("SoftwareDesignByExample.db");

    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    BasicRecord ex02{ "ex02", 67890, {3, 4} };

    {
        FileDb db(db_file_path);
        db.add(ex01);
        db.add(ex02);
    }

    {
        FileDb db(db_file_path);
        assert(db.get("ex01") == ex01);
        assert(db.get("ex02") == ex02);
    }

    filesystem::remove(db_file_path);
}

void database_main()
{
    cout << "Database:" << endl;
    test_get_nothing_from_empty_db();
    test_add_then_get();
    test_add_two_then_get_both();
    test_add_then_overwrite();
    test_filedb();
    cout << "All tests passed" << endl;
}
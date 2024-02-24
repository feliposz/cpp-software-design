// Inspired by Chapter 18: A Database
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/db/

#include <assert.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
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
        BasicRecord result;
        if (data.count(key))
        {
            result = data.at(key);
        }
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

class BlockDb : public Database
{
protected:
    size_t next_id = 0;
    map<string, size_t> index;
    vector<map<size_t, BasicRecord>> blocks;

public:
    constexpr static size_t records_per_block = 2;

    size_t size()
    {
        return records_per_block;
    }

    size_t num_blocks()
    {
        return blocks.size();
    }

    size_t num_records()
    {
        return index.size();
    }

    size_t get_block_id(size_t seq_id)
    {
        return seq_id / records_per_block;
    }

    map<size_t, BasicRecord>& get_block(size_t bloq_id)
    {
        while (bloq_id >= blocks.size())
        {
            blocks.push_back({});
        }
        return blocks.at(bloq_id);
    }

    virtual void add(const BasicRecord & record) override
    {
        string key = record.key();
        size_t seq_id = next_id++;
        index.emplace(key, seq_id);
        size_t block_id = get_block_id(seq_id);
        auto &block = get_block(block_id);
        block.emplace(seq_id, record);
    }

    virtual BasicRecord get(const string & key) override
    {
        BasicRecord result;
        if (index.count(key) == 0)
        {
            return result;
        }
        size_t seq_id = index.at(key);
        size_t block_id = get_block_id(seq_id);
        auto &block = get_block(block_id);
        result = block.at(seq_id);
        return result;
    }
};

class BlockFileDb : public BlockDb
{
    filesystem::path db_dir;

public:
    BlockFileDb(const filesystem::path &db_dir) : db_dir(db_dir)
    {
        build_index();
    }

    virtual void add(const BasicRecord & record) override
    {
        BlockDb::add(record);
        save(record);
    }

    virtual BasicRecord get(const string & key) override
    {
        if (index.count(key) == 0)
        {
            load(key);
        }
        return BlockDb::get(key);
    }

private:
    void build_index()
    {
        set<filesystem::path> files;
        for (const auto &entry : filesystem::directory_iterator(db_dir))
        {
            if (entry.is_regular_file())
            {
                auto extension = entry.path().extension();
                if (extension == L".db")
                {
                    files.emplace(entry.path());
                }
            }
        }
        for (const auto &file : files)
        {
            size_t block_id = _wtoi(file.stem().c_str());
            load_block(block_id);
        }
    }

    filesystem::path get_file_path(size_t block_id)
    {
        filesystem::path result(db_dir);
        result /= to_string(block_id);
        result += ".db";
        return result;
    }

    void save(const BasicRecord & record)
    {
        string key = record.key();
        size_t seq_id = index.at(key);
        size_t block_id = get_block_id(seq_id);
        save_block(block_id);
    }

    void load(const string &key)
    {
        size_t seq_id = index.at(key);
        size_t block_id = get_block_id(seq_id);
        load_block(block_id);
    }

    void save_block(size_t block_id)
    {
        auto &block = get_block(block_id);
        auto file_path = get_file_path(block_id);
        ofstream writer(file_path.c_str(), ios_base::binary);
        if (writer.is_open())
        {
            size_t record_size = BasicRecord::packed_size;
            size_t total_size = block.size() * record_size;
            size_t offset = 0;
            char *buffer = new char[total_size];
            for (const auto &[key, value] : block)
            {
                value.pack(buffer + offset);
                offset += record_size;
            }
            writer.write(buffer, total_size);
            writer.close();
            delete[] buffer;
        }
    }

    void load_block(size_t block_id)
    {
        auto &block = get_block(block_id);
        auto file_path = get_file_path(block_id);
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
                BlockDb::add(BasicRecord::unpack(buffer + offset));
            }
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

void test_blockdb()
{
    BlockDb db;
    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    BasicRecord ex02{ "ex02", 67890, {3, 4} };
    BasicRecord ex03{ "ex03", 77777, {7, 7} };
    db.add(ex01);
    db.add(ex02);
    db.add(ex03);
    assert(db.get("ex01") == ex01);
    assert(db.get("ex02") == ex02);
    assert(db.get("ex03") == ex03);
}

void test_blockfiledb()
{
    filesystem::path db_dir_path = filesystem::temp_directory_path();
    db_dir_path.append("sdbxdb");
    filesystem::create_directory(db_dir_path);

    BasicRecord ex01{ "ex01", 12345, {1, 2} };
    BasicRecord ex02{ "ex02", 67890, {3, 4} };
    BasicRecord ex03{ "ex03", 77777, {7, 7} };

    {
        BlockFileDb db(db_dir_path);
        db.add(ex01);
        db.add(ex02);
        db.add(ex03);
    }

    {
        BlockFileDb db(db_dir_path);
        assert(db.get("ex01") == ex01);
        assert(db.get("ex02") == ex02);
        assert(db.get("ex03") == ex03);
    }
    
    filesystem::remove_all(db_dir_path);
}

void database_main()
{
    cout << "Database:" << endl;
    test_get_nothing_from_empty_db();
    test_add_then_get();
    test_add_two_then_get_both();
    test_add_then_overwrite();
    test_filedb();
    test_blockdb();
    test_blockfiledb();
    cout << "All tests passed" << endl;
}
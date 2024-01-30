#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>

// requires: /std:c++17
#include <filesystem>

using namespace std;

// from FindDuplicateFiles.cpp
string sha_hash(const string &);

struct file_hash
{
    string filename;
    string hash;
};

void hash_all(vector<file_hash> &result, const string &root)
{
    filesystem::current_path(root);
    for (const auto &file : filesystem::recursive_directory_iterator(""))
    {
        if (file.is_regular_file())
        {
            string filename = file.path().string();
            string hash = sha_hash(filename.c_str());
            result.push_back({ filename, hash });
        }
    }
}

filesystem::path test_path, saved_path;

void test_init()
{
    saved_path = filesystem::current_path();
    test_path = filesystem::temp_directory_path().append("FileArchiverTest");
}

void test_setup()
{
    filesystem::create_directory(test_path);
    filesystem::current_path(test_path);
    filesystem::create_directory("sub_dir");

    ofstream a("a.txt"), b("b.txt"), c("sub_dir\\c.txt");
    a.write("aaa", 3);
    b.write("bbb", 3);
    c.write("ccc", 3);
    a.close();
    b.close();
    c.close();
}

void test_teardown()
{
    filesystem::current_path(saved_path);
    filesystem::remove_all(test_path);
}

void test_nested_example()
{
    test_setup();
    assert(filesystem::exists("a.txt"));
    assert(filesystem::exists("b.txt"));
    assert(filesystem::exists("sub_dir\\c.txt"));
    test_teardown();
}

void test_deletion_example()
{
    test_setup();
    assert(filesystem::exists("a.txt"));
    filesystem::remove("a.txt");
    assert(!filesystem::exists("a.txt"));
    test_teardown();
}

void test_hashing()
{
    test_setup();
    vector<file_hash> result;
    hash_all(result, test_path.string());
    set<string> expected = { "a.txt", "b.txt", "sub_dir\\c.txt" };
    for (const auto &fh : result)
    {
        assert(expected.count(fh.filename) > 0);
        assert(fh.hash.length() == 64);
    }
    test_teardown();
}

void test_change()
{
    test_setup();
    vector<file_hash> original;
    hash_all(original, test_path.string());

    map<string, string> original_map;
    for (const auto &fh : original)
    {
        original_map[fh.filename] = fh.hash;
    }

    ofstream a("a.txt");
    a.write("XXX", 3);
    a.close();

    vector<file_hash> changed;
    hash_all(changed, test_path.string());

    for (const auto &fh : changed)
    {
        if (fh.filename == "a.txt")
        {
            assert(original_map[fh.filename] != fh.hash);
        }
        else
        {
            assert(original_map[fh.filename] == fh.hash);
        }
    }
    test_teardown();
}

void archiver_main()
{
    cout << "File Archiver:" << endl;

    test_init();
    test_nested_example();
    test_deletion_example();
    test_hashing();
    test_change();

    cout << "All tests passed" << endl;

}
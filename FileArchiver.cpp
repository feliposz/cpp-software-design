#include <assert.h>
#include <time.h>
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

void write_manifest(const string &target, char *timestamp_str, vector<file_hash> &manifest, string &manifest_filename)
{
    if (!filesystem::exists(target))
    {
        filesystem::create_directory(target);
    }
    manifest_filename = target + '\\' + timestamp_str + ".csv";
    ofstream out(manifest_filename);
    out << "filename,hash" << endl;
    for (const auto &fh : manifest)
    {
        out << fh.filename << "," << fh.hash << endl;
    }
    out.close();
}

void copy_files(const string &source, const string &target, vector<file_hash> &manifest)
{
    for (const auto &fh : manifest)
    {
        string source_path = source + '\\' + fh.filename;
        string target_path = target + '\\' + fh.hash + ".bck";
        if (!filesystem::exists(target_path))
        {
            filesystem::copy_file(source_path, target_path);
        }
    }
}

void backup(const string &source, const string &target, vector<file_hash> &manifest, string &manifest_filename)
{
    hash_all(manifest, source);
    time_t timestamp = time(0);
    char timestamp_str[80];
    struct tm timeinfo;
    localtime_s(&timeinfo, &timestamp);
    strftime(timestamp_str, 80, "%Y%m%d_%H%M%S", &timeinfo);
    write_manifest(target, timestamp_str, manifest, manifest_filename);
    copy_files(source, target, manifest);
}

filesystem::path files_path, saved_path, backup_path;

void test_init()
{
    saved_path = filesystem::current_path();
    files_path = filesystem::temp_directory_path().append("FileArchiverTest");
    backup_path = filesystem::temp_directory_path().append("FileArchiverBackup");
}

void test_setup()
{
    filesystem::create_directory(files_path);
    filesystem::current_path(files_path);
    filesystem::create_directory("sub_dir");

    ofstream a("a.txt"), b("b.txt"), c("sub_dir\\c.txt");
    a << "aaa";
    b << "bbb";
    c << "ccc";
    a.close();
    b.close();
    c.close();
}

void test_teardown()
{
    filesystem::current_path(saved_path);
    filesystem::remove_all(files_path);
    filesystem::remove_all(backup_path);
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
    hash_all(result, files_path.string());
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
    hash_all(original, files_path.string());

    map<string, string> original_map;
    for (const auto &fh : original)
    {
        original_map[fh.filename] = fh.hash;
    }

    ofstream a("a.txt");
    a << "XXX";
    a.close();

    vector<file_hash> changed;
    hash_all(changed, files_path.string());

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

void test_backup()
{
    test_setup();
    vector<file_hash> manifest;
    string manifest_filename;
    backup(files_path.string(), backup_path.string(), manifest, manifest_filename);
    for (const auto &fh : manifest)
    {
        string file_path = backup_path.string() + '\\' + fh.hash + ".bck";
        assert(filesystem::exists(file_path));
    }
    assert(filesystem::exists(manifest_filename));
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
    test_backup();

    cout << "All tests passed" << endl;
}
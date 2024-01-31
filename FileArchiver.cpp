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

string current_timestamp()
{
    time_t timestamp = time(0);
    char timestamp_str[80];
    struct tm timeinfo;
    localtime_s(&timeinfo, &timestamp);
    strftime(timestamp_str, 80, "%Y%m%d_%H%M%S", &timeinfo);
    string result(timestamp_str);
    return result;
}

void write_manifest(const string &target, vector<file_hash> &manifest, string &manifest_filename)
{
    if (!filesystem::exists(target))
    {
        filesystem::create_directory(target);
    }
    manifest_filename = target + '\\' + current_timestamp() + ".csv";
    ofstream out(manifest_filename);
    out << "filename,hash" << endl;
    for (const auto &fh : manifest)
    {
        out << fh.filename << "," << fh.hash << endl;
    }
    out.close();
}

void read_manifest(vector<file_hash> &manifest, const string &manifest_filepath)
{
    ifstream in(manifest_filepath);
    if (in.is_open())
    {
        string line;
        in >> line;
        assert(line == "filename,hash");
        while (true)
        {
            in >> line;
            if (in.eof())
            {
                break;
            }
            string filename = line.substr(0, line.find(","));
            string hash = line.substr(line.find(",") + 1);
            manifest.push_back({ filename, hash });
        }
        in.close();
    }
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

void compare_manifest(const vector<file_hash> &left, const vector<file_hash> &right, vector<string> &changelog)
{
    map<string, string> left_file2hash, left_hash2file, right_file2hash, right_hash2file;

    for (const auto &left_fh : left)
    {
        left_file2hash[left_fh.filename] = left_fh.hash;
        left_hash2file[left_fh.hash] = left_fh.filename;
    }

    for (const auto &right_fh : right)
    {
        right_file2hash[right_fh.filename] = right_fh.hash;
        right_hash2file[right_fh.hash] = right_fh.filename;
    }

    for (const auto &left_fh : left)
    {
        if (right_file2hash.count(left_fh.filename) == 0 && right_hash2file.count(left_fh.hash) == 0)
        {
            changelog.push_back(left_fh.filename + " deleted");
        }
        else if (right_file2hash.count(left_fh.filename) && right_file2hash[left_fh.filename] != left_fh.hash)
        {
            changelog.push_back(left_fh.filename + " updated");
        }
        else if (right_hash2file.count(left_fh.hash) && right_hash2file[left_fh.hash] != left_fh.filename)
        {
            changelog.push_back(left_fh.filename + " renamed to " + right_hash2file[left_fh.hash]);
        }
    }

    for (const auto &right_fh : right)
    {
        if (left_file2hash.count(right_fh.filename) == 0 && left_hash2file.count(right_fh.hash) == 0)
        {
            changelog.push_back(right_fh.filename + " added");
        }
    }
}

void backup(const string &source, const string &target, vector<file_hash> &manifest, string &manifest_filename)
{
    hash_all(manifest, source);
    write_manifest(target, manifest, manifest_filename);
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

void test_compare_manifest()
{
    vector<file_hash> original = { { "a.txt", "aaa" }, { "b.txt", "bbb" }, { "sub_dir\\c.txt", "ccc" }, { "unchanged.txt", "unchanged" } };
    vector<file_hash> changed = { { "a.txt", "XXX" }, { "Y.txt", "bbb" }, { "d.txt", "ddd" }, { "unchanged.txt", "unchanged" } };
    vector<string> expect = { "a.txt updated", "b.txt renamed to Y.txt", "sub_dir\\c.txt deleted", "d.txt added" };
    vector<string> changelog;
    compare_manifest(original, changed, changelog);
    assert(changelog == expect);
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
    test_compare_manifest();

    cout << "All tests passed" << endl;
}
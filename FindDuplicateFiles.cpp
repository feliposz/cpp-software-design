// Inspired by Chapter 3: Finding Duplicate Files
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/dup/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

// requires: /std:c++17
#include <filesystem>

// OpenSSL Installed and configured according to: https://stackoverflow.com/questions/32156336/how-to-include-openssl-in-visual-studio
#include <openssl/evp.h>
// OpenSSL engine implementation
#define OPENSSL_ENGINE NULL

using namespace std;

const int BUFSIZE = 1024 * 1024;
char left_buf[BUFSIZE], right_buf[BUFSIZE];

bool same_bytes(const string &left, const string &right)
{
    ifstream left_f(left, ios_base::binary);
    ifstream right_f(right, ios_base::binary);

    if (left_f.is_open() && right_f.is_open())
    {
        while (!left_f.eof() && !right_f.eof())
        {
            left_f.read(left_buf, BUFSIZE);
            right_f.read(right_buf, BUFSIZE);

            if (left_f.gcount() != right_f.gcount())
            {
                return false;
            }

            streamsize bytes_read = left_f.gcount();
            for (int i = 0; i < bytes_read; i++)
            {
                if (left_buf[i] != right_buf[i])
                {
                    return false;
                }
            }
        }

        if (left_f.eof() && right_f.eof())
        {
            return true;
        }
    }

    return false;
}

void duplicate_no_hash()
{
    cout << "Duplicate File Finder (no hash, direct file comparisons)" << endl;

    vector<pair<string, string>> matches;

    int count_files = 0;
    int count_cmp = 0;

    for (const auto &left : filesystem::directory_iterator(""))
    {
        string left_s = left.path().string();
        if (left.is_regular_file())
        {
            count_files++;
            for (const auto &right : filesystem::directory_iterator(""))
            {
                // Avoid comparing twice and with same file
                if (left < right && right.is_regular_file())
                {
                    string right_s = right.path().string();
                    count_cmp++;
                    if (same_bytes(left_s, right_s))
                    {
                        matches.push_back({ left_s, right_s });
                    }
                }
            }
        }
    }

    cout << "File: " << count_files << " Comparisons: " << count_cmp << " Duplicates: " << matches.size() << endl;

    for (const auto &pair : matches)
    {
        cout << pair.first << "\t" << pair.second << endl;
    }
}

const int HASH_BUCKETS = 27;

int naive_hash(string filename)
{
    ifstream file(filename, ios_base::binary);
    int hash = 0;

    if (file.is_open())
    {
        while (!file.eof())
        {
            file.read(left_buf, BUFSIZE);
            streamsize bytes_read = file.gcount();
            for (int i = 0; i < bytes_read; i++)
            {
                hash += left_buf[i];
            }
        }
    }

    return hash % HASH_BUCKETS;
}

void duplicate_with_naive_hash()
{
    cout << "Duplicate File Finder (with naive hash)" << endl;

    vector<pair<string, string>> matches;
    vector<string> groups[HASH_BUCKETS];

    int count_files = 0;
    int count_cmp = 0;

    for (const auto &file : filesystem::directory_iterator(""))
    {
        if (file.is_regular_file())
        {
            count_files++;
            string filename = file.path().string();
            int hash = naive_hash(filename);
            groups[hash].push_back(filename);
        }
    }

    for (int i = 0; i < HASH_BUCKETS; i++)
    {
        for (const string &left : groups[i])
        {
            //cout << i << "\t" << left << endl;
            for (const string &right : groups[i])
            {
                if (left < right)
                {
                    count_cmp++;
                    if (same_bytes(left, right))
                    {
                        matches.push_back({ left, right });
                    }
                }
            }
        }
    }

    cout << "File: " << count_files << " Comparisons: " << count_cmp << " Duplicates: " << matches.size() << endl;

    for (const auto &pair : matches)
    {
        cout << pair.first << "\t" << pair.second << endl;
    }
}

int histogram[256];

// Based on: https://stackoverflow.com/questions/2262386/generate-sha256-with-openssl-and-c/10632725
string sha_hash(string filename)
{
    ifstream file(filename, ios_base::binary);
    
    EVP_MD_CTX *mdCtx = EVP_MD_CTX_new();
    unsigned char mdVal[EVP_MAX_MD_SIZE], *md;
    unsigned int mdLen, i;

    if (!EVP_DigestInit_ex(mdCtx, EVP_sha256(), OPENSSL_ENGINE))
    {
        printf("Message digest initialization failed.\n");
        EVP_MD_CTX_free(mdCtx);
        exit(EXIT_FAILURE);
    }

    if (file.is_open())
    {
        while (!file.eof())
        {
            file.read(left_buf, BUFSIZE);
            streamsize bytes_read = file.gcount();
           
            // Hashes cnt bytes of data at d into the digest context mdCtx
            if (!EVP_DigestUpdate(mdCtx, left_buf, bytes_read))
            {
                printf("Message digest update failed.\n");
                EVP_MD_CTX_free(mdCtx);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (!EVP_DigestFinal_ex(mdCtx, mdVal, &mdLen))
    {
        printf("Message digest finalization failed.\n");
        EVP_MD_CTX_free(mdCtx);
        exit(EXIT_FAILURE);
    }
    EVP_MD_CTX_free(mdCtx);

    // convert bytes of the hash to hexadecimal string
    stringstream ss;
    for (i = 0; i < mdLen; i++)
    {
        histogram[mdVal[i]]++;
        ss << setfill('0') << setw(2) << hex << (int)mdVal[i];
    }

    md = mdVal;

    string result(ss.str());
    return result;
}

void duplicate_with_sha()
{
    cout << "Duplicate File Finder (with SHA-256)" << endl;

    vector<pair<string, string>> matches;
    unordered_map<string, vector<string>> groups;

    int count_files = 0;
    int count_cmp = 0;

    for (const auto &file : filesystem::directory_iterator(""))
    {
        if (file.is_regular_file())
        {
            count_files++;
            string filename = file.path().string();
            string hash = sha_hash(filename.c_str());
            //cout << hash << "\t" << filename << endl;
            groups[hash].push_back(filename);
        }
    }

    for (const auto &group : groups)
    {
        for (const string &left : group.second)
        {
            //cout << i << "\t" << left << endl;
            for (const string &right : group.second)
            {
                if (left < right)
                {
                    count_cmp++;
                    if (same_bytes(left, right))
                    {
                        matches.push_back({ left, right });
                    }
                }
            }
        }
    }

    cout << "File: " << count_files << " Comparisons: " << count_cmp << " Duplicates: " << matches.size() << endl;

    for (const auto &pair : matches)
    {
        cout << pair.first << "\t" << pair.second << endl;
    }
}

void duplicate_main()
{
    string path = "V:\\GitHub\\feliposz\\tutorial-compiladores\\src";
    filesystem::current_path(path);

    for (int i = 0; i < 256; i++)
    {
        histogram[i] = 0;
    }

    duplicate_no_hash();
    duplicate_with_naive_hash();
    duplicate_with_sha();

#if 0
    // check "randomness" of the distribution of values in the sha hash
    for (int i = 0; i < 256; i++)
    {
        cout << (i % 8 ? "\t" : "\n") << histogram[i];
    }
#endif

}
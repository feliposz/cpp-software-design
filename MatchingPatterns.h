#pragma once

#include <vector>

using namespace std;

struct Match
{
    Match *rest;
    bool match(const string &text);
    virtual size_t _match(const string &text, size_t start = 0);
};

struct Null : Match
{
    Null();
    size_t _match(const string &text, size_t start = 0) override;
};

struct Lit : Match
{
    string chars;
    Lit(string chars, Match *rest = new Null());
    size_t _match(const string &text, size_t start = 0) override;
};

struct Any : Match
{
    Any(Match *rest = new Null());
    size_t _match(const string &text, size_t start = 0) override;
};

struct Either : Match
{
    Match *left, *right;
    Either(Match *left, Match *right, Match *rest = new Null());
    size_t _match(const string &text, size_t start = 0) override;
};

struct Choice : Match
{
    vector<Match *> patterns;
    Choice(vector<Match *> patterns, Match *rest = new Null());
    size_t _match(const string &text, size_t start = 0) override;
};

struct OnePlus : Match
{
    char c;
    OnePlus(char c, Match *rest = new Null());
    size_t _match(const string &text, size_t start = 0) override;
};

struct Charset : Match
{
    string charset;
    Charset(string charset, Match *rest = new Null());
    size_t _match(const string &text, size_t start = 0) override;
};

struct Range : Match
{
    char left, right;
    Range(char left, char right, Match *rest = new Null());
    size_t _match(const string &text, size_t start = 0) override;
};
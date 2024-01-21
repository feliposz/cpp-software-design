// Inspired by Chapter 4: Matching Patterns
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/glob/

#include <assert.h>
#include <iostream>
#include <vector>

using namespace std;

struct Match
{
    Match *rest;

    bool match(const string &text)
    {
        size_t result = _match(text, 0);
        return result == text.length();
    }

    virtual size_t _match(const string &text, size_t start = 0)
    {
        throw new exception("not implemented");
    }
};

struct Null : Match
{
    Null()
    {
        this->rest = nullptr;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        return start;
    }
};

Match *NULL_MATCH = new Null();
const size_t NOT_A_MATCH = SIZE_MAX;

struct Lit : Match
{
    string chars;

    Lit(string chars, Match *rest = NULL_MATCH)
    {
        this->chars = chars;
        this->rest = rest;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        size_t end = start + chars.length();
        if (text.substr(start, chars.length()) != chars)
        {
            return NOT_A_MATCH;
        }
        return this->rest->_match(text, end);
    }
};

struct Any : Match
{
    Any(Match *rest = NULL_MATCH)
    {
        this->rest = rest;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        // less than or EQUAL because it needs to match an empty string too
        for (size_t i = 0; i <= text.length(); i++)
        {
            size_t end = this->rest->_match(text, i);
            if (end == text.length())
            {
                return end;
            }
        }
        return NOT_A_MATCH;
    }
};

struct Either : Match
{
    Match *left, *right;

    Either(Match *left, Match *right, Match *rest = NULL_MATCH)
    {
        this->left = left;
        this->right = right;
        this->rest = rest;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        vector<Match *> patterns = { this->left, this->right };
        for (const auto &pat : patterns)
        {
            size_t end = pat->_match(text, start);
            if (end != NOT_A_MATCH)
            {
                end = this->rest->_match(text, end);
                if (end == text.length())
                {
                    return end;
                }
            }
        }
        return NOT_A_MATCH;
    }
};

void test_literal_match_entire_string()
{
    assert((new Lit("abc"))->match("abc"));
}

void test_literal_substring_alone_no_match()
{
    assert(!(new Lit("ab"))->match("abc"));
}

void test_literal_superstring_no_match()
{
    assert(!(new Lit("abc"))->match("ab"));
}

void test_literal_followed_by_literal_match()
{
    assert((new Lit("a", new Lit("b")))->match("ab"));
}

void test_literal_followed_by_literal_no_match()
{
    assert(!(new Lit("a", new Lit("b")))->match("ac"));
}

void test_any_matches_empty()
{
    assert((new Any())->match(""));
}

void test_any_matches_entire_string()
{
    assert((new Any())->match("abc"));
}

void test_any_matches_as_prefix()
{
    assert((new Any(new Lit("def")))->match("abcdef"));
}

void test_any_matches_as_suffix()
{
    assert((new Lit("abc", new Any()))->match("abcdef"));
}

void test_any_matches_interior()
{
    assert((new Lit("a", new Any(new Lit("c"))))->match("abc"));
}

void test_either_two_literals_first()
{
    assert((new Either(new Lit("a"), new Lit("b")))->match("a"));
}

void test_either_two_literals_not_both()
{
    assert(!(new Either(new Lit("a"), new Lit("b")))->match("ab"));
}

void test_either_followed_by_literal_match()
{
    assert((new Either(new Lit("a"), new Lit("b"), new Lit("c")))->match("ac"));
}

void test_either_followed_by_literal_no_match()
{
    assert(!(new Either(new Lit("a"), new Lit("b"), new Lit("c")))->match("ax"));
}

void matching_main()
{
    cout << "Matching Patterns" << endl;

    test_literal_match_entire_string();
    test_literal_substring_alone_no_match();
    test_literal_superstring_no_match();
    test_literal_followed_by_literal_match();
    test_literal_followed_by_literal_no_match();

    test_any_matches_empty();
    test_any_matches_entire_string();
    test_any_matches_as_prefix();
    test_any_matches_as_suffix();
    test_any_matches_interior();

    test_either_two_literals_first();
    test_either_two_literals_not_both();
    test_either_followed_by_literal_match();
    test_either_followed_by_literal_no_match();
}
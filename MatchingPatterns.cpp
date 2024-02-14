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

    virtual ~Match() = default;

    bool match(const string &text)
    {
        size_t result = _match(text, 0);
        return result == text.length();
    }

    virtual size_t _match(const string &text, size_t start = 0) = 0;
};

struct Null : Match
{
    Null()
    {
        this->rest = nullptr;
    }

    ~Null() override
    {
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        return start;
    }
};

const size_t NOT_A_MATCH = SIZE_MAX;

struct Lit : Match
{
    string chars;

    Lit(string chars, Match *rest = new Null())
    {
        this->chars = chars;
        this->rest = rest;
    }

    ~Lit() override
    {
        delete rest;
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
    Any(Match *rest = new Null())
    {
        this->rest = rest;
    }

    ~Any() override
    {
        delete rest;
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

    Either(Match *left, Match *right, Match *rest = new Null())
    {
        this->left = left;
        this->right = right;
        this->rest = rest;
    }

    ~Either() override
    {
        delete left;
        delete right;
        delete rest;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        vector<Match *> patterns;
        patterns.push_back(this->left);
        patterns.push_back(this->right);
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

struct Choice : Match
{
    vector<Match *> patterns;

    Choice(vector<Match *> patterns, Match *rest = new Null())
    {
        this->patterns = patterns;
        this->rest = rest;
    }

    ~Choice() override
    {
        delete rest;
        for (auto p : patterns)
        {
            delete p;
        }
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        for (const auto &pat : this->patterns)
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

struct OnePlus : Match
{
    char c;

    OnePlus(char c, Match *rest = new Null())
    {
        this->c = c;
        this->rest = rest;
    }

    ~OnePlus() override
    {
        delete rest;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        for (size_t i = start; i < text.length(); i++)
        {
            if (this->c == text[i])
            {
                size_t end = this->rest->_match(text, i + 1);
                if (end == text.length())
                {
                    return end;
                }
            }
            else
            {
                break;
            }
        }
        return NOT_A_MATCH;
    }
};

struct Charset : Match
{
    string charset;

    Charset(string charset, Match *rest = new Null())
    {
        this->charset = charset;
        this->rest = rest;
    }

    ~Charset() override
    {
        delete rest;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        for (const char &c : this->charset)
        {
            if (text[start] == c)
            {
                size_t end = this->rest->_match(text, start + 1);
                if (end == text.length())
                {
                    return end;
                }
            }
        }
        return NOT_A_MATCH;
    }
};

struct Range : Match
{
    char left, right;

    Range(char left, char right, Match *rest = new Null())
    {
        this->left = left;
        this->right= right;
        this->rest = rest;
    }

    ~Range() override
    {
        delete rest;
    }

    size_t _match(const string &text, size_t start = 0) override
    {
        if (text[start] >= left && text[start] <= right)
        {
            return this->rest->_match(text, start + 1);
        }
        return NOT_A_MATCH;
    }
};

void test_literal_match_entire_string()
{
    Match *m = new Lit("abc");
    assert(m->match("abc"));
    delete m;
}

void test_literal_substring_alone_no_match()
{
    Match *m = new Lit("ab");
    assert(!m->match("abc"));
    delete m;
}

void test_literal_superstring_no_match()
{
    Match *m = new Lit("abc");
    assert(!m->match("ab"));
    delete m;
}

void test_literal_followed_by_literal_match()
{
    Match *m = new Lit("a", new Lit("b"));
    assert(m->match("ab"));
    delete m;
}

void test_literal_followed_by_literal_no_match()
{
    Match *m = new Lit("a", new Lit("b"));
    assert(!m->match("ac"));
    delete m;
}

void test_any_matches_empty()
{
    Match *m = new Any();
    assert(m->match(""));
    delete m;
}

void test_any_matches_entire_string()
{
    Match *m = new Any();
    assert(m->match("abc"));
    delete m;
}

void test_any_matches_as_prefix()
{
    Match *m = new Any(new Lit("def"));
    assert(m->match("abcdef"));
    delete m;
}

void test_any_matches_as_suffix()
{
    Match *m = new Lit("abc", new Any());
    assert(m->match("abcdef"));
    delete m;
}

void test_any_matches_interior()
{
    Match *m = new Lit("a", new Any(new Lit("c")));
    assert(m->match("abc"));
    delete m;
}

void test_either_two_literals_first()
{
    Match *m = new Either(new Lit("a"), new Lit("b"));
    assert(m->match("a"));
    delete m;
}

void test_either_two_literals_not_both()
{
    Match *m = new Either(new Lit("a"), new Lit("b"));
    assert(!m->match("ab"));
    delete m;
}

void test_either_followed_by_literal_match()
{
    Match *m = new Either(new Lit("a"), new Lit("b"), new Lit("c"));
    assert(m->match("ac"));
    delete m;
}

void test_either_followed_by_literal_no_match()
{
    Match *m = new Either(new Lit("a"), new Lit("b"), new Lit("c"));
    assert(!m->match("ax"));
    delete m;
}

void test_oneplus_empty_no_match()
{
    Match *m = new OnePlus('a');
    assert(!m->match(""));
    delete m;
}

void test_oneplus_matches_one()
{
    Match *m = new OnePlus('a');
    assert(m->match("a"));
    delete m;
}

void test_oneplus_matches_multiple()
{
    Match *m = new OnePlus('a');
    assert(m->match("aaa"));
    delete m;
}

void test_oneplus_one_no_match()
{
    Match *m = new OnePlus('a');
    assert(!m->match("x"));
    delete m;
}

void test_oneplus_multiple_no_match()
{
    Match *m = new OnePlus('a');
    assert(!m->match("xax"));
    delete m;
}

void test_oneplus_matches_as_prefix()
{
    Match *m = new OnePlus('x', new Lit("abc"));
    assert(m->match("xxabc"));
    delete m;
}

void test_oneplus_matches_as_suffix()
{
    Match *m = new Lit("abc", new OnePlus('x'));
    assert(m->match("abcxx"));
    delete m;
}

void test_oneplus_matches_as_infix()
{
    Match *m = new Lit("abc", new OnePlus('x', new Lit("def")));
    assert(m->match("abcxxdef"));
    delete m;
}

void test_charset_matches()
{
    Match *m = new Charset("aeiou");
    assert(m->match("i"));
    delete m;
}

void test_charset_no_match()
{
    Match *m = new Charset("aeiou");
    assert(!m->match("x"));
    delete m;
}

void test_charset_empty_no_match()
{
    Match *m = new Charset("aeiou");
    assert(!m->match(""));
    delete m;
}

void test_range_start_matches()
{
    Match *m = new Range('a', 'f');
    assert(m->match("a"));
    delete m;
}

void test_range_mid_matches()
{
    Match *m = new Range('a', 'f');
    assert(m->match("c"));
    delete m;
}

void test_range_end_matches()
{
    Match *m = new Range('a', 'f');
    assert(m->match("f"));
    delete m;
}

void test_range_no_match()
{
    Match *m = new Range('a', 'f');
    assert(!m->match("z"));
    delete m;
}

void test_choice_one_literal_matches()
{
    Match *m = new Choice({ new Lit("a") });
    assert(m->match("a"));
    delete m;
}

void test_choice_one_literal_no_match()
{
    Match *m = new Choice({ new Lit("a") });
    assert(!m->match("b"));
    delete m;
}

void test_choice_two_literals_first()
{
    Match *m = new Choice({ new Lit("a"), new Lit("b") });
    assert(m->match("a"));
    delete m;
}

void test_choice_three_literals_second()
{
    Match *m = new Choice({ new Lit("a"), new Lit("b"), new Lit("c") });
    assert(m->match("b"));
    delete m;
}

void test_choice_four_literals_last()
{
    Match *m = new Choice({ new Lit("a"), new Lit("b"), new Lit("c"), new Lit("d") });
    assert(m->match("d"));
    delete m;
}

void test_choice_two_literals_not_both()
{
    Match *m = new Choice({ new Lit("a"), new Lit("b") });
    assert(!m->match("ab"));
    delete m;
}

void test_choice_three_literals_not_both()
{
    Match *m = new Choice({ new Lit("a"), new Lit("b"), new Lit("c") });
    assert(!m->match("x"));
    delete m;
}

void test_choice_followed_by_literal_match()
{
    Match *m = new Choice({ new Lit("a"), new Lit("b") }, new Lit("c"));
    assert(m->match("ac"));
    delete m;
}

void test_choice_followed_by_literal_no_match()
{
    Match *m = new Choice({ new Lit("a"), new Lit("b") }, new Lit("c"));
    assert(!m->match("ax"));
    delete m;
}

void test_choice_empty_no_match()
{
    Match *m = new Choice({ });
    assert(!m->match("x"));
    delete m;
}

void matching_main()
{
    cout << "Matching Patterns:" << endl;

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

    test_oneplus_empty_no_match();
    test_oneplus_matches_one();
    test_oneplus_matches_multiple();
    test_oneplus_one_no_match();
    test_oneplus_multiple_no_match();
    test_oneplus_matches_as_prefix();
    test_oneplus_matches_as_suffix();
    test_oneplus_matches_as_infix();

    test_charset_matches();
    test_charset_no_match();
    test_charset_empty_no_match();

    test_range_start_matches();
    test_range_mid_matches();
    test_range_end_matches();
    test_range_no_match();

    test_choice_one_literal_matches();
    test_choice_one_literal_no_match();
    test_choice_two_literals_first();
    test_choice_three_literals_second();
    test_choice_four_literals_last();
    test_choice_two_literals_not_both();
    test_choice_three_literals_not_both();
    test_choice_followed_by_literal_match();
    test_choice_followed_by_literal_no_match();
    test_choice_empty_no_match();

    cout << "All tests passed!" << endl;
}
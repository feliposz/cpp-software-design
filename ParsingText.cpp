// Inspired by Chapter 5: Parsing Text
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/parse/

#include <assert.h>
#include <iostream>
#include <vector>
#include "MatchingPatterns.h"

using namespace std;

enum TokenType
{
    TT_None,
    TT_Literal,
    TT_Any,
    TT_EitherStart,
    TT_EitherEnd,
    TT_CharsetStart,
    TT_CharsetEnd,
};

struct Token
{
    TokenType type;
    string text;

    Token(TokenType type, string text = "")
    {
        this->type = type;
        this->text = text;
    }
};

struct Tokenizer
{
    vector<Token*> tokens;
    string current = "";

    ~Tokenizer()
    {
        for (auto t : tokens)
        {
            delete t;
        }
    }

    void add(TokenType type)
    {
        if (current.length() > 0)
        {
            tokens.push_back(new Token(TT_Literal, current));
            current = "";
        }
        if (type != TT_None)
        {
            tokens.push_back(new Token(type));
        }
    }

    void tokenize(string text)
    {
        bool escape_next = false;
        for (const auto ch : text)
        {
            if (escape_next)
            {
                current += ch;
                escape_next = false;
            }
            else if (ch == '\\')
            {
                escape_next = true;
            }
            else if (ch == '*')
            {
                add(TT_Any);
            }
            else if (ch == '{')
            {
                add(TT_EitherStart);
            }
            else if (ch == '}')
            {
                add(TT_EitherEnd);
            }
            else if (ch == '[')
            {
                add(TT_CharsetStart);
            }
            else if (ch == ']')
            {
                add(TT_CharsetEnd);
            }
            else if (ch == ',')
            {
                add(TT_None);
            }
            else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))
            {
                current += ch;
            }
            else
            {
                throw exception("invalid character");
            }
        }
        add(TT_None);
    }
};

struct Parser
{
    Tokenizer tokenizer;

    Match *parse(string text)
    {
        tokenizer.tokenize(text);
        return _parse(tokenizer.tokens);
    }

    Match *_parse(vector<Token*> &tokens, size_t start = 0)
    {
        if (start >= tokens.size())
        {
            return new Null();
        }
        else if (tokens[start]->type == TT_Any)
        {
            return new Any(_parse(tokens, start + 1));
        }
        else if (tokens[start]->type == TT_EitherStart)
        {
            Token *left = tokens[start + 1];
            Token *right = tokens[start + 2];
            Token *end = tokens[start + 3];
            if (tokens.size() - start < 3 || left->type != TT_Literal || right->type != TT_Literal || end->type != TT_EitherEnd)
            {
                throw exception("badly-formatted Either");
            }
            return new Either(new Lit(left->text), new Lit(right->text), _parse(tokens, start + 4));
        }
        else if (tokens[start]->type == TT_CharsetStart)
        {
            Token *chars = tokens[start + 1];
            Token *end = tokens[start + 2];
            if (tokens.size() - start < 2 || chars->type != TT_Literal || end->type != TT_CharsetEnd)
            {
                throw exception("badly-formatted Charset");
            }
            return new Charset(chars->text, _parse(tokens, start + 3));
        }
        else if (tokens[start]->type == TT_Literal)
        {
            return new Lit(tokens[start]->text, _parse(tokens, start + 1));
        }
        throw exception("invalid code path");
    }
};

bool compare_tokens(vector<Token*> &a, vector<Token*> &b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (int i = 0; i < a.size(); i++)
    {
        if (a[i]->type != b[i]->type || a[i]->text != b[i]->text)
        {
            return false;
        }
    }
    return true;
}

bool compare_match(Match *a, Match *b)
{
    {
        Null* _a = dynamic_cast<Null*>(a);
        Null* _b = dynamic_cast<Null*>(b);
        if (_a && _b)
        {
            return true;
        }
    }
    {
        Lit* _a = dynamic_cast<Lit*>(a);
        Lit* _b = dynamic_cast<Lit*>(b);
        if (_a && _b)
        {
            return _a->chars == _b->chars && compare_match(_a->rest, _b->rest);
        }
    }
    {
        Any* _a = dynamic_cast<Any*>(a);
        Any* _b = dynamic_cast<Any*>(b);
        if (_a && _b)
        {
            return compare_match(_a->rest, _b->rest);
        }
    }
    {
        Either* _a = dynamic_cast<Either*>(a);
        Either* _b = dynamic_cast<Either*>(b);
        if (_a && _b)
        {
            return compare_match(_a->left, _b->left)
                && compare_match(_a->right, _b->right)
                && compare_match(_a->rest, _b->rest);
        }
    }
    {
        Charset* _a = dynamic_cast<Charset*>(a);
        Charset* _b = dynamic_cast<Charset*>(b);
        if (_a && _b)
        {
            return _a->charset == _b->charset && compare_match(_a->rest, _b->rest);
        }
    }
    return false;

    /*
    Choice
    OnePlus
    Range
    */
}

void test_tok_empty_string()
{
    Tokenizer t;
    t.tokenize("");
    assert(t.tokens.size() == 0);
}

void test_tok_any_either()
{
    Tokenizer t;
    t.tokenize("*{abc,def}");
    vector<Token*> expected = {
        new Token(TT_Any),
        new Token(TT_EitherStart),
        new Token(TT_Literal, "abc"),
        new Token(TT_Literal, "def"),
        new Token(TT_EitherEnd),
    };
    assert(compare_tokens(t.tokens, expected));
    for (auto t : expected)
    {
        delete t;
    }
}

void test_tok_escape()
{
    Tokenizer t;
    t.tokenize("\\*{abc,def}\\{xyz\\}");
    vector<Token*> expected = {
        new Token(TT_Literal, "*"),
        new Token(TT_EitherStart),
        new Token(TT_Literal, "abc"),
        new Token(TT_Literal, "def"),
        new Token(TT_EitherEnd),
        new Token(TT_Literal, "{xyz}"),
    };
    assert(compare_tokens(t.tokens, expected));
    for (auto t : expected)
    {
        delete t;
    }
}

void test_parse_either_two_lit()
{
    Parser p;
    Match *result = p.parse("{abc,def}");
    Match *expected = new Either(new Lit("abc"), new Lit("def"));
    assert(compare_match(result, expected));
    delete expected;
    delete result;
}

void test_parse_charset()
{
    Parser p;
    Match *result = p.parse("[abc]");
    Match *expected = new Charset("abc");
    assert(compare_match(result, expected));
    delete result;
    delete expected;
}

void parsing_main()
{
    cout << "Parsing Text:" << endl;

    test_tok_empty_string();
    test_tok_any_either();
    test_tok_escape();
    test_parse_either_two_lit();
    test_parse_charset();

    cout << "All tests passed!" << endl;
}
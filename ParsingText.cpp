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

    vector<Token*> *tokenize(string text)
    {
        for (const auto ch : text)
        {
            if (ch == '*')
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
                throw new exception("invalid character");
            }
        }
        add(TT_None);
        return &tokens;
    }
};

struct Parser
{
    Tokenizer tokenizer;

    Match *parse(string text)
    {
        vector<Token*> *tokens = tokenizer.tokenize(text);
        return _parse(*tokens);
    }

    Match *_parse(vector<Token*> &tokens, int start = 0)
    {
        if (start >= tokens.size())
        {
            return NULL_MATCH;
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
                throw new exception("badly-formatted Either");
            }
            return new Either(new Lit(left->text), new Lit(right->text), _parse(tokens, start + 4));
        }
        else if (tokens[start]->type == TT_Literal)
        {
            return new Lit(tokens[start]->text, _parse(tokens, start + 1));
        }
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
    return false;

    /*
    Choice
    OnePlus
    Charset
    Range
    */
}

void test_tok_empty_string()
{
    assert((new Tokenizer())->tokenize("")->size() == 0);
}

void test_tok_any_either()
{
    vector<Token*> *result = (new Tokenizer())->tokenize("*{abc,def}");
    vector<Token*> expected = {
        new Token(TT_Any),
        new Token(TT_EitherStart),
        new Token(TT_Literal, "abc"),
        new Token(TT_Literal, "def"),
        new Token(TT_EitherEnd),
    };
    assert(compare_tokens(*result, expected));
}

void test_parse_either_two_lit()
{
    Match *result = (new Parser())->parse("{abc,def}");
    Match *expected = new Either(new Lit("abc"), new Lit("def"));
    assert(compare_match(result, expected));
}

void parsing_main()
{
    cout << "Parsing Text:" << endl;

    test_tok_empty_string();
    test_tok_any_either();
    test_parse_either_two_lit();

    cout << "All tests passed!" << endl;
}
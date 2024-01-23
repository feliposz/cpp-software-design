// Inspired by Chapter 5: Parsing Text
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/parse/

#include <assert.h>
#include <iostream>
#include <vector>

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

void parsing_main()
{
    cout << "Parsing Text:" << endl;

    test_tok_empty_string();
    test_tok_any_either();

    cout << "All tests passed!" << endl;
}
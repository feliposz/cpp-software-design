// Inspired by Chapter 15: Performance Profiling
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/perf/

#include <assert.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>

using namespace std;

struct DataFrame
{
    // Report the number of columns.
    virtual size_t ncol() = 0;

    // Report the number of rows.
    virtual size_t nrow() = 0;

    // Return the set of column names.
    virtual vector<string> cols() = 0;

    // Check equality with another dataframe.
    virtual bool eq(DataFrame *other) = 0;

    // Get a scalar value.
    virtual int get(string col, size_t row) = 0;

    // Select a named subset of columns.
    virtual DataFrame* select(const set<string> &names) = 0;

    //Select a subset of rows by testing values.
    virtual DataFrame* filter(bool(*func)(map<string, int> &row)) = 0;
};

struct DfRow : DataFrame
{
    vector<map<string, int>> rows;

    DfRow(const vector<map<string, int>> &rows) : rows(rows)
    {
        set<string> prototype_cols;
        for (const auto &kv : rows[0])
        {
            prototype_cols.emplace(kv.first);
        }
        for (const auto &row : rows)
        {
            set<string> row_cols;
            for (const auto &kv : row)
            {
                row_cols.emplace(kv.first);
            }
            if (prototype_cols != row_cols)
            {
                throw exception("columns don't match");
            }
        }
    }

    virtual size_t ncol() override
    {
        return rows[0].size();
    }

    virtual size_t nrow() override
    {
        return rows.size();
    }

    virtual vector<string> cols() override
    {
        vector<string> cols;
        for (const auto &kv : rows[0])
        {
            cols.push_back(kv.first);
        }
        return cols;
    }

    virtual bool eq(DataFrame * other) override
    {
        if (this->ncol() != other->ncol())
        {
            return false;
        }
        if (this->nrow() != other->nrow())
        {
            return false;
        }
        if (this->cols() != other->cols())
        {
            return false;
        }
        for (size_t i = 0; i < rows.size(); i++)
        {
            for (const auto &kv : rows[i])
            {
                if (other->get(kv.first, i) != kv.second)
                {
                    return false;
                }
            }
        }
        return true;
    }

    virtual int get(string col, size_t row) override
    {
        return rows[row][col];
    }

    virtual DataFrame* select(const set<string>& name_set) override
    {
        vector<map<string, int>> result;
        for (size_t i = 0; i < rows.size(); i++)
        {
            map<string, int> selected_row;
            for (const auto &kv : rows[i])
            {
                if (name_set.count(kv.first))
                {
                    selected_row[kv.first] = kv.second;
                }
            }
            result.push_back(selected_row);
        }
        return new DfRow(result);
    }

    virtual DataFrame* filter(bool(*func)(map<string, int> &row)) override
    {
        vector<map<string, int>> result;
        for (size_t i = 0; i < rows.size(); i++)
        {
            if (func(rows[i]))
            {
                result.push_back(rows[i]);
            }
        }
        return new DfRow(result);
    }
};

DataFrame* odd_even()
{
    return new DfRow({ { { "a", 1 }, { "b" , 3 } }, { {"a", 2}, {"b", 4} } });
}

DataFrame* a_only()
{
    return new DfRow({ {{"a", 1}}, {{"a", 2}} });
}

void test_construct_with_single_value()
{
    DataFrame *df = new DfRow({ {{"a", 1}} });
    assert(df->get("a", 0) == 1);
    delete df;
}

void test_construct_with_two_pairs()
{
    DataFrame *df = odd_even();
    assert(df->get("a", 0) == 1);
    assert(df->get("a", 1) == 2);
    assert(df->get("b", 0) == 3);
    assert(df->get("b", 1) == 4);
    delete df;
}

void test_nrow()
{
    DataFrame *df = odd_even();
    assert(df->nrow() == 2);
    delete df;
}

void test_ncol()
{
    DataFrame *df = odd_even();
    assert(df->ncol() == 2);
    delete df;
}

void test_equality()
{
    DataFrame *left = odd_even();
    DataFrame *right = new DfRow({ { { "a", 1 }, { "b" , 3 } }, { {"a", 2}, {"b", 4} } });
    assert(left->eq(right) && right->eq(left));
    delete left;
    delete right;
}

void test_inequality()
{
    DataFrame *left = odd_even();
    DataFrame *right = a_only();
    assert(!left->eq(right));
    DataFrame *repeated = new DfRow({ { { "a", 1 }, { "b" , 3 } }, { {"a", 1}, {"b", 3} } });
    assert(!left->eq(repeated));
    delete left;
    delete right;
    delete repeated;
}

void test_select()
{
    DataFrame *df = odd_even();
    DataFrame *selected = df->select({ "a" });
    DataFrame *expect = a_only();
    assert(selected->eq(expect));
    delete df;
    delete selected;
    delete expect;
}

void test_filter()
{
    auto is_odd = [](map<string, int> &row)
    {
        return (row["a"] % 2) == 1;
    };

    DataFrame *df = odd_even();
    DataFrame *filtered = df->filter(is_odd);
    DataFrame *expect = new DfRow({ {{"a", 1}, {"b", 3} } });
    assert(filtered->eq(expect));
    delete df;
    delete filtered;
    delete expect;
}

void profiling_main()
{
    cout << "Performance Profiling:" << endl;
    test_construct_with_single_value();
    test_construct_with_two_pairs();
    test_nrow();
    test_ncol();
    test_equality();
    test_inequality();
    test_select();
    test_filter();
    cout << "All tests passed!" << endl;
}
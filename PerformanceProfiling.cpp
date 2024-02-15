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
    vector<map<string, int>> data;

    DfRow(const vector<map<string, int>> &data) : data(data)
    {
        set<string> prototype_cols;
        for (const auto &kv : data[0])
        {
            prototype_cols.emplace(kv.first);
        }
        for (const auto &row : data)
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
        return data[0].size();
    }

    virtual size_t nrow() override
    {
        return data.size();
    }

    virtual vector<string> cols() override
    {
        vector<string> result;
        for (const auto &kv : data[0])
        {
            result.push_back(kv.first);
        }
        return result;
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
        for (size_t i = 0; i < data.size(); i++)
        {
            for (const auto &kv : data[i])
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
        return data[row][col];
    }

    virtual DataFrame* select(const set<string>& name_set) override
    {
        vector<map<string, int>> result;
        for (size_t i = 0; i < data.size(); i++)
        {
            map<string, int> selected_row;
            for (const auto &kv : data[i])
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
        for (size_t i = 0; i < data.size(); i++)
        {
            if (func(data[i]))
            {
                result.push_back(data[i]);
            }
        }
        return new DfRow(result);
    }
};

struct DfCol : DataFrame
{
    map<string,vector<int>> data;

    DfCol(const map<string,vector<int>> &data) : data(data)
    {
        size_t first_size;
        for (const auto &kv : data)
        {
            first_size = kv.second.size();
            break;
        }
        if (first_size == 0)
        {
            throw exception("empty column data");
        }
        for (const auto &kv : data)
        {
            size_t size = kv.second.size();
            if (size != first_size)
            {
                throw exception("size mismatch");
            }
        }
    }

    virtual size_t ncol() override
    {
        return data.size();
    }

    virtual size_t nrow() override
    {
        for (const auto &kv : data)
        {
            return kv.second.size();
        }
        throw exception("unreachable");
    }

    virtual vector<string> cols() override
    {
        vector<string> result;
        for (const auto &kv : data)
        {
            result.push_back(kv.first);
        }
        return result;
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
        for (const auto &kv : data)
        {
            for (size_t i = 0; i < kv.second.size(); i++)
            {
                if (other->get(kv.first, i) != kv.second[i])
                {
                    return false;
                }
            }
        }
        return true;
    }

    virtual int get(string col, size_t row) override
    {
        return data[col][row];
    }

    virtual DataFrame* select(const set<string>& name_set) override
    {
        map<string,vector<int>> result;
        for (const auto &kv : data)
        {
            if (name_set.count(kv.first))
            {
                result[kv.first] = kv.second;
            }
        }
        return new DfCol(result);
    }

    virtual DataFrame* filter(bool(*func)(map<string, int> &row)) override
    {
        map<string,vector<int>> result;
        size_t nrow = this->nrow();
        for (size_t i = 0; i < nrow; i++)
        {
            map<string, int> row;
            for (const auto &kv : data)
            {
                row[kv.first] = kv.second[i];
            }
            if (func(row))
            {
                for (const auto &kv : data)
                {
                    result[kv.first].push_back(kv.second[i]);
                }
            }
        }
        return new DfCol(result);
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

void test_dfrow_construct_with_single_value()
{
    DataFrame *df = new DfRow({ {{"a", 1}} });
    assert(df->get("a", 0) == 1);
    delete df;
}

void test_dfrow_construct_with_two_pairs()
{
    DataFrame *df = odd_even();
    assert(df->get("a", 0) == 1);
    assert(df->get("a", 1) == 2);
    assert(df->get("b", 0) == 3);
    assert(df->get("b", 1) == 4);
    delete df;
}

void test_dfrow_nrow()
{
    DataFrame *df = odd_even();
    assert(df->nrow() == 2);
    delete df;
}

void test_dfrow_ncol()
{
    DataFrame *df = odd_even();
    assert(df->ncol() == 2);
    delete df;
}

void test_dfrow_equality()
{
    DataFrame *left = odd_even();
    DataFrame *right = new DfRow({ { { "a", 1 }, { "b" , 3 } }, { {"a", 2}, {"b", 4} } });
    assert(left->eq(right) && right->eq(left));
    delete left;
    delete right;
}

void test_dfrow_inequality()
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

void test_dfrow_select()
{
    DataFrame *df = odd_even();
    DataFrame *selected = df->select({ "a" });
    DataFrame *expect = a_only();
    assert(selected->eq(expect));
    delete df;
    delete selected;
    delete expect;
}

void test_dfrow_filter()
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

void test_dfcol_construct_with_single_value()
{
    DataFrame *df = new DfCol({ { "a", {1} } });
    assert(df->get("a", 0) == 1);
    delete df;
}
void test_dfcol_construct_with_two_pairs()
{
    DataFrame *df = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    assert(df->get("a", 0) == 1);
    assert(df->get("a", 1) == 2);
    assert(df->get("b", 0) == 3);
    assert(df->get("b", 1) == 4);
    delete df;
}
void test_dfcol_nrow()
{
    DataFrame *df = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    assert(df->nrow() == 2);
    delete df;
}

void test_dfcol_ncol()
{
    DataFrame *df = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    assert(df->ncol() == 2);
    delete df;
}

void test_dfcol_equality()
{
    DataFrame *left = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    DataFrame *right = new DfCol({ {"b", {3, 4} }, { "a", {1, 2} } });
    assert(left->eq(right) && right->eq(left));
    delete left;
    delete right;
}

void test_dfcol_inequality()
{
    DataFrame *left = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    DataFrame *right = new DfCol({ { "a", {1, 2} } });
    DataFrame *repeated = new DfCol({ { "a", {1, 2} }, {"b", {1, 2}} });
    assert(!left->eq(right));
    assert(!left->eq(repeated));
    delete left;
    delete right;
    delete repeated;
}

void test_dfcol_select()
{
    DataFrame *df = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    DataFrame *selected = df->select({ "a" });
    DataFrame *expected = new DfCol({ { "a", {1, 2} } });
    assert(selected->eq(expected));
    delete df;
    delete selected;
    delete expected;
}

void test_dfcol_filter()
{
    auto is_odd = [](map<string, int> &row)
    {
        return (row["a"] % 2) == 1;
    };

    DataFrame *df = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    DataFrame *filtered = df->filter(is_odd);
    DataFrame *expected = new DfCol({ { "a", {1} }, {"b", {3}} });
    assert(filtered->eq(expected));
    delete df;
    delete filtered;
    delete expected;
}

void profiling_main()
{
    cout << "Performance Profiling:" << endl;
    test_dfrow_construct_with_single_value();
    test_dfrow_construct_with_two_pairs();
    test_dfrow_nrow();
    test_dfrow_ncol();
    test_dfrow_equality();
    test_dfrow_inequality();
    test_dfrow_select();
    test_dfrow_filter();
    test_dfcol_construct_with_single_value();
    test_dfcol_construct_with_two_pairs();
    test_dfcol_nrow();
    test_dfcol_ncol();
    test_dfcol_equality();
    test_dfcol_inequality();
    test_dfcol_select();
    test_dfcol_filter();
    cout << "All tests passed!" << endl;
}
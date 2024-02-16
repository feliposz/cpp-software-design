// Inspired by Chapter 15: Performance Profiling
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/perf/

#include <assert.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <chrono>

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
    virtual DataFrame* filter(bool(*func)(DataFrame *df, size_t row)) = 0;
};

struct DfRow : DataFrame
{
    vector<unordered_map<string, int>> data;

    DfRow(const vector<unordered_map<string, int>> &data) : data(data)
    {
        set<string> prototype_cols;
        for (const auto &[key, value] : data[0])
        {
            prototype_cols.emplace(key);
        }
        for (const auto &row : data)
        {
            set<string> row_cols;
            for (const auto &[key, value] : row)
            {
                row_cols.emplace(key);
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
        for (const auto &[key, value] : data[0])
        {
            result.push_back(key);
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
            for (const auto &[key, value] : data[i])
            {
                if (other->get(key, i) != value)
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
        vector<unordered_map<string, int>> result(data.size());
        for (size_t i = 0; i < data.size(); i++)
        {
            for (const auto &name : name_set)
            {
                result[i][name] = data[i][name];
            }
        }
        return new DfRow(result);
    }

    virtual DataFrame* filter(bool(*func)(DataFrame *df, size_t row)) override
    {
        vector<unordered_map<string, int>> result;
        for (size_t i = 0; i < data.size(); i++)
        {
            if (func(this, i))
            {
                result.push_back(data[i]);
            }
        }
        return new DfRow(result);
    }
};

struct DfCol : DataFrame
{
    unordered_map<string, vector<int>> data;

    DfCol(const unordered_map<string, vector<int>> &data) : data(data)
    {
        size_t first_size = 0;
        for (const auto &[key, value] : data)
        {
            first_size = value.size();
            break;
        }
        if (first_size == 0)
        {
            throw exception("empty column data");
        }
        for (const auto &[key, value] : data)
        {
            size_t size = value.size();
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
        for (const auto &[key, value] : data)
        {
            return value.size();
        }
        throw exception("unreachable");
    }

    virtual vector<string> cols() override
    {
        vector<string> result;
        for (const auto &[key, value] : data)
        {
            result.push_back(key);
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
        for (const auto &[key, value] : data)
        {
            for (size_t i = 0; i < value.size(); i++)
            {
                if (other->get(key, i) != value[i])
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
        unordered_map<string, vector<int>> result;
        for (const auto &name : name_set)
        {
            result[name] = data[name];
        }
        return new DfCol(result);
    }

    virtual DataFrame* filter(bool(*func)(DataFrame *df, size_t row)) override
    {
        unordered_map<string, vector<int>> result;
        size_t nrow = this->nrow();
        for (size_t i = 0; i < nrow; i++)
        {
            if (func(this, i))
            {
                for (const auto &[key, value] : data)
                {
                    result[key].push_back(value[i]);
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
    auto is_odd = [](DataFrame *df, size_t row)
    {
        return (df->get("a", row) % 2) == 1;
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
    auto is_odd = [](DataFrame *df, size_t row)
    {
        return (df->get("a", row) % 2) == 1;
    };

    DataFrame *df = new DfCol({ { "a", {1, 2} }, {"b", {3, 4} } });
    DataFrame *filtered = df->filter(is_odd);
    DataFrame *expected = new DfCol({ { "a", {1} }, {"b", {3}} });
    assert(filtered->eq(expected));
    delete df;
    delete filtered;
    delete expected;
}

const size_t RANGE = 10;

DataFrame *make_col(size_t nrow, size_t ncol)
{
    unordered_map<string, vector<int>> data;
    vector<string> col_names;
    for (size_t c = 0; c < ncol; c++)
    {
        col_names.push_back("label_" + to_string(c));
    }
    for (size_t c = 0; c < ncol; c++)
    {
        for (size_t r = 0; r < nrow; r++)
        {
            data[col_names[c]].push_back((c + r) % RANGE);
        }
    }
    return new DfCol(data);
}

DataFrame *make_row(size_t nrow, size_t ncol)
{
    vector<unordered_map<string, int>> data(nrow);
    vector<string> col_names(ncol);
    for (size_t c = 0; c < ncol; c++)
    {
        col_names[c] = "label_" + to_string(c);
    }
    for (size_t r = 0; r < nrow; r++)
    {
        for (size_t c = 0; c < ncol; c++)
        {
            data[r][col_names[c]] = (c + r) % RANGE;
        }
    }
    return new DfRow(data);
}

auto time_filter(DataFrame *df)
{
    auto first_is_odd = [](DataFrame *df, size_t row)
    {
        return (df->get("label_0", row) % 2) == 1;
    };
    auto start = chrono::steady_clock::now();
    DataFrame *filtered = df->filter(first_is_odd);
    auto end = chrono::steady_clock::now();
    auto time = end - start;
    delete filtered;
    return time;
}

auto time_select(DataFrame *df)
{
    set<string> selected_cols;
    int i = 0;
    for (auto col : df->cols())
    {
        if ((i % 3) == 0)
        {
            selected_cols.emplace(col);
        }
    }
    auto start = chrono::steady_clock::now();
    DataFrame *selected = df->select(selected_cols);
    auto end = chrono::steady_clock::now();
    auto time = end - start;
    delete selected;
    return time;
}

void sweep()
{
    const double NANO_TO_MS = 1.0 / 1000000.0;
#if 1
    vector<size_t> sizes = { 10, 50, 100, 500, 1000 };
#else
    vector<size_t> sizes = { 10, 100, 1000, 2500 };
#endif
    cout << "Profiling... (times are in ms)" << endl;
    cout << "nrow\tncol\tflt_col\tsel_col\tflt_row\tsel_row" << endl;
    for (auto size : sizes)
    {
        DataFrame *df_col = make_col(size, size);
        DataFrame *df_row = make_row(size, size);
        assert(df_col->eq(df_row) && df_row->eq(df_col));
        vector<double> times = {
            time_filter(df_col).count() * NANO_TO_MS,
            time_select(df_col).count() * NANO_TO_MS,
            time_filter(df_row).count() * NANO_TO_MS,
            time_select(df_row).count() * NANO_TO_MS,
        };
        cout << size << "\t" << size << "\t" << times[0] << "\t" << times[1] << "\t" << times[2] << "\t" << times[3] << endl;
        delete df_col;
        delete df_row;
    }
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
    sweep();
}
// Inspired by Chapter 14: Page Layout
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/layout/

#include <assert.h>
#include <iostream>
#include <vector>

using namespace std;

inline int max(int a, int b)
{
    return a > b ? a : b;
}

class IRect
{
public:
    virtual ~IRect() = default;
    virtual int get_width() = 0;
    virtual int get_height() = 0;
};

class IContainer
{
protected:
    vector<IRect*> children;

public:
    IContainer(vector<IRect*> children) : children(children)
    {
    }

    ~IContainer()
    {
        for (auto child : children)
        {
            delete child;
        }
    }
};

class Block : public IRect
{
    int width, height;

public:
    Block(int w, int h) : width(w), height(h)
    {
    }

    int get_width() override
    {
        return width;
    }

    int get_height() override
    {
        return height;
    }
};

class Row : public IRect, IContainer
{
public:
    Row(vector<IRect*> children) : IContainer(children)
    {
    }
    
    int get_width() override
    {
        int sum_width = 0;
        for (auto &child : children)
        {
            sum_width += child->get_width();
        }
        return sum_width;
    }

    int get_height() override
    {
        int max_height = 0;
        for (auto &child : children)
        {
            max_height = max(max_height, child->get_height());
        }
        return max_height;
    }
};

class Col : public IRect, IContainer
{

public:
    Col(vector<IRect*> children) : IContainer(children)
    {
    }

    int get_width() override
    {
        int max_width = 0;
        for (auto child : children)
        {
            max_width = max(max_width, child->get_width());
        }
        return max_width;
    }

    int get_height() override
    {
        int sum_height = 0;
        for (auto child : children)
        {
            sum_height += child->get_height();
        }
        return sum_height;
    }
};

void test_lays_out_a_grid_of_rows_of_columns()
{
    IRect *fixture = new Col({
        new Row({ new Block(1, 2), new Block(3, 4) }),
        new Row({ new Block(5, 6), new Col({new Block(7, 8), new Block(9, 10)}) })
    });

    assert(fixture->get_width() == 14);
    assert(fixture->get_height() == 22);

    delete fixture;
}

void layout_main()
{
    cout << "Page Layout:" << endl;
    test_lays_out_a_grid_of_rows_of_columns();
    cout << "All tests passed!" << endl;
}
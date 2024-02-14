// Inspired by Chapter 14: Page Layout
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/layout/

#include <assert.h>
#include <iostream>
#include <sstream>
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
    virtual void place(int, int) = 0;
    virtual string report() = 0;
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
    int width, height, x0, y0;

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

    void place(int x, int y)
    {
        x0 = x;
        y0 = y;
    }

    string report() override
    {
        stringstream ss;
        ss << "[block, " << x0 << ", " << y0 << ", " << (x0 + width) << ", " << (y0 + height) << "]";
        return ss.str();
    }
};

class Row : public IRect, IContainer
{
    int x0, y0;

public:
    Row(vector<IRect*> children) : IContainer(children)
    {
    }
    
    int get_width() override
    {
        int sum_width = 0;
        for (auto child : children)
        {
            sum_width += child->get_width();
        }
        return sum_width;
    }

    int get_height() override
    {
        int max_height = 0;
        for (auto child : children)
        {
            max_height = max(max_height, child->get_height());
        }
        return max_height;
    }

    void place(int x, int y)
    {
        x0 = x;
        y0 = y;
        int x_current = x0;
        int y1 = y0 + get_height();
        for (auto child : children)
        {
            int child_y = y1 - child->get_height();
            child->place(x_current, child_y);
            x_current += child->get_width();
        }
    }

    string report() override
    {
        stringstream ss;
        ss << "[row, " << x0 << ", " << y0 << ", " << (x0 + get_width()) << ", " << (y0 + get_height());
        for (auto child : children)
        {
            ss << ", " << child->report();
        }
        ss << "]";
        return ss.str();
    }
};

class Col : public IRect, IContainer
{
    int x0, y0;

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

    void place(int x, int y)
    {
        x0 = x;
        y0 = y;
        int y_current = y0;
        for (auto child : children)
        {
            child->place(x0, y_current);
            y_current += child->get_height();
        }
    }

    string report() override
    {
        stringstream ss;
        ss << "[col, " << x0 << ", " << y0 << ", " << (x0 + get_width()) << ", " << (y0 + get_height());
        for (auto child : children)
        {
            ss << ", " << child->report();
        }
        ss << "]";
        return ss.str();
    }
};

void test_lays_out_a_single_unit_block()
{
    IRect *fixture = new Block(1, 1);
    assert(fixture->get_width() == 1);
    assert(fixture->get_height() == 1);

    delete fixture;
}
void test_lays_out_a_large_block()
{
    IRect *fixture = new Block(3, 4);
    assert(fixture->get_width() == 3);
    assert(fixture->get_height() == 4);

    delete fixture;
}
void test_lays_out_a_row_of_two_blocks()
{
    IRect *fixture = new Row({ new Block(1, 1), new Block(2, 4) });
    assert(fixture->get_width() == 3);
    assert(fixture->get_height() == 4);

    delete fixture;
}
void test_lays_out_a_column_of_two_blocks()
{
    IRect *fixture = new Col({ new Block(1, 1), new Block(2, 4) });
    assert(fixture->get_width() == 2);
    assert(fixture->get_height() == 5);

    delete fixture;
}

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

void test_places_a_single_unit_block()
{
    IRect *fixture = new Block(1, 1);
    fixture->place(0, 0);
    assert(fixture->report() == "[block, 0, 0, 1, 1]");
    delete fixture;
}

void test_places_a_large_block()
{
    IRect *fixture = new Block(3, 4);
    fixture->place(0, 0);
    assert(fixture->report() == "[block, 0, 0, 3, 4]");
    delete fixture;
}

void test_places_a_row_of_two_blocks()
{
    IRect *fixture = new Row({ new Block(1, 1), new Block(2, 4) });
    fixture->place(0, 0);
    assert(fixture->report() == "[row, 0, 0, 3, 4, [block, 0, 3, 1, 4], [block, 1, 0, 3, 4]]");
    delete fixture;
}

void test_places_a_column_of_two_blocks()
{
    IRect *fixture = new Col({ new Block(1, 1), new Block(2, 4) });
    fixture->place(0, 0);
    assert(fixture->report() == "[col, 0, 0, 2, 5, [block, 0, 0, 1, 1], [block, 0, 1, 2, 5]]");
    delete fixture;
}

void test_places_a_grid_of_rows_of_columns()
{
    IRect *fixture = new Col({ new Row({new Block(1, 2), new Block(3, 4)}), new Row({new Block(5, 6), new Col({new Block(7, 8), new Block(9, 10)})}) });
    fixture->place(0, 0);
    assert(fixture->report() == "[col, 0, 0, 14, 22, [row, 0, 0, 4, 4, [block, 0, 2, 1, 4], [block, 1, 0, 4, 4]], [row, 0, 4, 14, 22, [block, 0, 16, 5, 22], [col, 5, 4, 14, 22, [block, 5, 4, 12, 12], [block, 5, 12, 14, 22]]]]");
    delete fixture;
}

void layout_main()
{
    cout << "Page Layout:" << endl;
    test_lays_out_a_single_unit_block();
    test_lays_out_a_large_block();
    test_lays_out_a_row_of_two_blocks();
    test_lays_out_a_column_of_two_blocks();
    test_lays_out_a_grid_of_rows_of_columns();
    test_places_a_single_unit_block();
    test_places_a_large_block();
    test_places_a_row_of_two_blocks();
    test_places_a_column_of_two_blocks();
    test_places_a_grid_of_rows_of_columns();
    cout << "All tests passed!" << endl;
}
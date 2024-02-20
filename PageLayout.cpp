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
protected:
    int x0 = 0, y0 = 0;
public:
    virtual ~IRect() = default;
    virtual int get_width() = 0;
    virtual int get_height() = 0;
    virtual void place(int, int) = 0;
    virtual string report() = 0;
    virtual IRect* wrap() = 0;

    void render(vector<string> &screen, char fill = 'a')
    {
        for (size_t y = 0; y < get_height(); y++)
        {
            for (size_t x = 0; x < get_width(); x++)
            {
                screen[y0 + y][x0 + x] = fill;
            }
        }
    }
};

class IContainer
{
public:
    vector<IRect*> children;

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

    IRect* wrap() override
    {
        return new Block(width, height);
    }
};

class Row : public IRect, public IContainer
{
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

    IRect* wrap() override
    {
        vector<IRect*> wrapped_children;
        for (auto child : children)
        {
            wrapped_children.push_back(child->wrap());
        }
        return new Row(wrapped_children);
    }
};

class Col : public IRect, public IContainer
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

    IRect* wrap() override
    {
        vector<IRect*> wrapped_children;
        for (auto child : children)
        {
            wrapped_children.push_back(child->wrap());
        }
        return new Col(wrapped_children);
    }
};

class WrappedRow : public Row
{
    int width;

private:
    void bucket(vector<vector<IRect*>> &result, vector<IRect*> &wrapped_children)
    {
        vector<IRect*> current_row;
        int current_x = 0;

        for (auto child : wrapped_children)
        {
            int child_width = child->get_width();
            if ((current_x + child_width) <= width)
            {
                current_row.push_back(child);
                current_x += child_width;
            }
            else
            {
                result.push_back(current_row);
                current_row = { child };
                current_x = child_width;
            }
        }
        result.push_back(current_row);
    }

public:
    WrappedRow(int width, vector<IRect*> children) : Row(children), width(width)
    {
        assert(width >= 0);
    }

    int get_width() override
    {
        return width;
    }

    IRect* wrap() override
    {
        vector<IRect*> wrapped_children;
        for (auto child : children)
        {
            wrapped_children.push_back(child->wrap());
        }
        vector<vector<IRect*>> rows;
        bucket(rows, wrapped_children);
        vector<IRect*> new_rows;
        for (auto &row : rows)
        {
            new_rows.push_back(new Row(row));
        }
        IRect *new_col = new Col(new_rows);
        return new Row({ new_col });
    }
};

void make_screen(vector<string> &screen, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        string row(width, ' ');
        screen.push_back(row);
    }
}

char draw(vector<string> &screen, IRect *root, char fill = 0)
{
    fill = fill == 0 ? 'a' : fill + 1;
    root->render(screen, fill);
    if (IContainer *c = dynamic_cast<IContainer *>(root))
    {
        for (auto child : c->children)
        {
            fill = draw(screen, child, fill);
        }
    }
    return fill;
}

void render(vector<string> &screen, IRect *root)
{
    root->place(0, 0);
    make_screen(screen, root->get_width(), root->get_height());
    draw(screen, root);
    for (const string &line : screen)
    {
        //cout << line << endl;
    }
}

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

void test_renders_a_single_unit_block()
{
    IRect *fixture = new Block(1, 1);
    vector<string> screen;
    render(screen, fixture);
    vector<string> expect = { "a" };
    assert(screen == expect);
    delete fixture;
}

void test_renders_a_large_block()
{
    IRect *fixture = new Block(3, 4);
    vector<string> screen;
    render(screen, fixture);
    vector<string> expect = { "aaa", "aaa", "aaa", "aaa" };
    assert(screen == expect);
    delete fixture;
}

void test_renders_a_row_of_two_blocks()
{
    IRect *fixture = new Row({ new Block(1, 1), new Block(2, 4) });
    vector<string> screen;
    render(screen, fixture);
    vector<string> expect = { "acc", "acc", "acc", "bcc" };
    assert(screen == expect);
    delete fixture;
}

void test_renders_a_column_of_two_blocks()
{
    IRect *fixture = new Col({ new Block(1, 1), new Block(2, 4) });
    vector<string> screen;
    render(screen, fixture);
    vector<string> expect = { "ba", "cc", "cc", "cc", "cc" };
    assert(screen == expect);
    delete fixture;
}

void test_renders_a_grid_of_rows_of_columns()
{
    IRect *fixture = new Col({ new Row({new Block(1, 2), new Block(3, 4)}), new Row({new Block(1, 2), new Col({new Block(3, 4), new Block(2, 3)})}) });
    vector<string> screen;
    render(screen, fixture);
    vector<string> expect = {
            "bddd",
            "bddd",
            "cddd",
            "cddd",
            "ehhh",
            "ehhh",
            "ehhh",
            "ehhh",
            "eiig",
            "fiig",
            "fiig",
    };
    assert(screen == expect);
    delete fixture;
}

void test_wraps_a_single_unit_block()
{
    IRect *fixture = new Block(1, 1);
    IRect *wrapped = fixture->wrap();
    wrapped->place(0, 0);
    string got = wrapped->report();
    string expect = "[block, 0, 0, 1, 1]";
    assert(got == expect);
    delete fixture;
    delete wrapped;
}

void test_wraps_a_large_block()
{
    IRect *fixture = new Block(3, 4);
    IRect *wrapped = fixture->wrap();
    wrapped->place(0, 0);
    string got = wrapped->report();
    string expect = "[block, 0, 0, 3, 4]";
    assert(got == expect);
    delete fixture;
    delete wrapped;
}

void test_wrap_a_row_of_two_blocks_that_fit_on_one_row()
{
    IRect *fixture = new WrappedRow(100, { new Block(1, 1), new Block(2, 4) });
    IRect *wrapped = fixture->wrap();
    wrapped->place(0, 0);
    string got = wrapped->report();
    string expect = "[row, 0, 0, 3, 4, [col, 0, 0, 3, 4, [row, 0, 0, 3, 4, [block, 0, 3, 1, 4], [block, 1, 0, 3, 4]]]]";
    assert(got == expect);
    delete fixture;
    delete wrapped;
}

void test_wraps_a_column_of_two_blocks()
{
    IRect *fixture = new Col({ new Block(1, 1), new Block(2, 4) });
    IRect *wrapped = fixture->wrap();
    wrapped->place(0, 0);
    string got = wrapped->report();
    string expect = "[col, 0, 0, 2, 5, [block, 0, 0, 1, 1], [block, 0, 1, 2, 5]]";
    assert(got == expect);
    delete fixture;
    delete wrapped;
}

void test_wraps_a_grid_of_rows_of_columns_that_all_fit_on_their_row()
{
    IRect *fixture = new Col({
        new WrappedRow(100, { new Block(1, 2), new Block(3, 4) }),
        new WrappedRow(100, { new Block(5, 6), new Col({ new Block(7, 8), new Block(9, 10)}) }),
                             });
    IRect *wrapped = fixture->wrap();
    wrapped->place(0, 0);
    string got = wrapped->report();
    string expect = "[col, 0, 0, 14, 22, [row, 0, 0, 4, 4, [col, 0, 0, 4, 4, [row, 0, 0, 4, 4, [block, 0, 2, 1, 4], [block, 1, 0, 4, 4]]]], [row, 0, 4, 14, 22, [col, 0, 4, 14, 22, [row, 0, 4, 14, 22, [block, 0, 16, 5, 22], [col, 5, 4, 14, 22, [block, 5, 4, 12, 12], [block, 5, 12, 14, 22]]]]]]";
    assert(got == expect);
    delete fixture;
    delete wrapped;
}

void test_wrap_a_row_of_two_blocks_that_do_not_fit_on_one_row()
{
    IRect *fixture = new WrappedRow(3, { new Block(2, 1), new Block(2, 1) });
    IRect *wrapped = fixture->wrap();
    wrapped->place(0, 0);
    string got = wrapped->report();
    string expect = "[row, 0, 0, 2, 2, [col, 0, 0, 2, 2, [row, 0, 0, 2, 1, [block, 0, 0, 2, 1]], [row, 0, 1, 2, 2, [block, 0, 1, 2, 2]]]]";
    assert(got == expect);
    delete fixture;
    delete wrapped;
}

void test_wrap_multiple_blocks_that_do_not_fit_on_one_row()
{
    IRect *fixture = new WrappedRow(3, { new Block(2, 1), new Block(2, 1), new Block(1, 1), new Block(2, 1) });
    IRect *wrapped = fixture->wrap();
    wrapped->place(0, 0);
    string got = wrapped->report();
    string expect = "[row, 0, 0, 3, 3, [col, 0, 0, 3, 3, [row, 0, 0, 2, 1, [block, 0, 0, 2, 1]], [row, 0, 1, 3, 2, [block, 0, 1, 2, 2], [block, 2, 1, 3, 2]], [row, 0, 2, 2, 3, [block, 0, 2, 2, 3]]]]";
    assert(got == expect);
    delete fixture;
    delete wrapped;
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
    test_renders_a_single_unit_block();
    test_renders_a_large_block();
    test_renders_a_row_of_two_blocks();
    test_renders_a_column_of_two_blocks();
    test_renders_a_grid_of_rows_of_columns();
    test_wraps_a_single_unit_block();
    test_wraps_a_large_block();
    test_wrap_a_row_of_two_blocks_that_fit_on_one_row();
    test_wraps_a_column_of_two_blocks();
    test_wraps_a_grid_of_rows_of_columns_that_all_fit_on_their_row();
    test_wrap_a_row_of_two_blocks_that_do_not_fit_on_one_row();
    test_wrap_multiple_blocks_that_do_not_fit_on_one_row();
    cout << "All tests passed!" << endl;
}
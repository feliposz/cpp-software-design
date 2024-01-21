// Inspired by Chapter 2: Objects and Classes
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/oop/

#include <iostream>
#include <string>
#include <vector>
#include <map>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

// =======================================================

class not_implemented
    : public exception
{
public:

    not_implemented(const char *msg) noexcept
        : exception(msg)
    {
    }
};

struct Shape
{
    string name;

    Shape(string name)
    {
        this->name = name;
    }

    virtual double perimeter()
    {
        throw new not_implemented("perimeter");
    }

    virtual double area()
    {
        throw new not_implemented("perimeter");
    }

    double density(double weight)
    {
        return weight / this->area();
    }
};

struct Square : Shape
{
    double side;

    Square(string name, double side) : Shape(name)
    {
        this->side = side;
    }

    double perimeter() override
    {
        return 4 * this->side;
    }

    double area() override
    {
        return pow(this->side, 2);
    }
};

struct Circle : Shape
{
    double radius;

    Circle(string name, double radius) : Shape(name)
    {
        this->radius = radius;
    }

    double perimeter() override
    {
        return 2 * M_PI * this->radius;
    }

    double area() override
    {
        return pow(this->radius, 2) * M_PI;
    }
};

void objects_example1()
{
    Square sq("sq", 3);
    Circle ci("ci", 2);

    vector<Shape*> examples = { &sq, &ci };

    cout.precision(2);
    cout.setf(ios::fixed, ios::floatfield);

    for (const auto thing : examples)
    {
        string n = thing->name;
        double p = thing->perimeter();
        double a = thing->area();
        double d = thing->density(5);
        cout << n << " has perimeter " << p << " and area " << a << " with density " << d << endl;
    }
}

// =======================================================

typedef map<string, void*> shape_obj;
typedef shape_obj* (*shape_ctor)(string name, double a1);
typedef void (*shape_dtor)(shape_obj *);
typedef double(*shape_fn)(shape_obj *t);
typedef double(*shape_fn_1)(shape_obj *t, double a1);

shape_obj ShapeClass, SquareClass, CircleClass;

void *find_method(shape_obj *thing, string method_name)
{
    // methods on the object itself...
    if (thing->count(method_name))
    {
        return (*thing)[method_name];
    }
    shape_obj *_class = (shape_obj *)(*thing)["_class"];
    while (_class)
    {
        if (_class->count(method_name))
        {
            return (*_class)[method_name];
        }
        _class = (shape_obj *)(*_class)["_parent"];
    }
    throw new not_implemented(method_name.c_str());
}

void *find_method_recursive(shape_obj *_class, string method_name)
{
    if (_class)
    {
        if (_class->count(method_name))
        {
            return (*_class)[method_name];
        }
        shape_obj *_parent = (shape_obj *)(*_class)["_parent"];
        return find_method_recursive(_parent, method_name);
    }
    throw new not_implemented(method_name.c_str());
}

void *find_method_r(shape_obj *thing, string method_name)
{
    shape_obj *_class = (shape_obj *)(*thing)["_class"];
    return find_method_recursive(_class, method_name);
}

double shape_fn_call(shape_obj *thing, string method_name)
{
    shape_fn fn = (shape_fn)find_method_r(thing, method_name);
    return (double)fn(thing);
}

double shape_fn_call(shape_obj *thing, string method_name, double a1)
{
    shape_fn_1 fn = (shape_fn_1)find_method_r(thing, method_name);
    return (double)fn(thing, a1);
}

shape_obj *make(shape_obj *_class, string name, double a1)
{
    shape_ctor ctor = (shape_ctor)(*_class)["_new"];
    shape_obj *result = ctor(name, a1);
    (*result)["_class"] = _class;
    return result;
}

void destroy(shape_obj *thing)
{
    shape_obj *_class = (shape_obj *)(*thing)["_class"];
    shape_dtor dtor = (shape_dtor)(*_class)["_delete"];
    dtor(thing);
}

double shape_larger(shape_obj *thing, double size)
{
    double a = shape_fn_call(thing, "area");
    double result = (double)(a > size);
    return result;
}

double shape_density(shape_obj *thing, double weight)
{
    double a = shape_fn_call(thing, "area");
    double result = weight / a;
    return result;
}

double square_perimeter(shape_obj *thing)
{
    double side = *(double*)(*thing)["side"];
    return 4 * side;
}

double square_area(shape_obj *thing)
{
    double side = *(double*)(*thing)["side"];
    return pow(side, 2);
}

shape_obj *square_new(string name, double side)
{
    shape_obj *result = new shape_obj();
    (*result)["name"] = new string(name);
    (*result)["side"] = new double(side);
    return result;
}

void square_delete(shape_obj *thing)
{
    delete (*thing)["name"];
    delete (*thing)["side"];
    delete thing;
}

double circle_perimeter(shape_obj *thing)
{
    double radius = *(double*)(*thing)["radius"];
    return 2 * M_PI * radius;
}

double circle_area(shape_obj *thing)
{
    double radius = *(double*)(*thing)["radius"];
    return pow(radius, 2) * M_PI;
}

shape_obj *circle_new(string name, double radius)
{
    shape_obj *result = new shape_obj();
    (*result)["name"] = new string(name);
    (*result)["radius"] = new double(radius);
    return result;
}

void circle_delete(shape_obj *thing)
{
    delete (*thing)["name"];
    delete (*thing)["radius"];
    delete thing;
}

string type_of(shape_obj *thing)
{
    shape_obj *_class = (shape_obj *)(*thing)["_class"];
    string _classname = *(string *)(*_class)["_classname"];
    return _classname;
}

bool is_instance_of(shape_obj *thing, shape_obj *_other)
{
    shape_obj *_class = (shape_obj *)(*thing)["_class"];
    while (_class)
    {
        if (_class == _other)
        {
            return true;
        }
        _class = (shape_obj *)(*_class)["_parent"];
    }
    return false;
}

inline const char* bool_str(const bool b)
{
    return b ? "true" : "false";
}

void objects_example2()
{
    ShapeClass["_classname"] = new string("Shape");
    ShapeClass["density"] = shape_density;
    ShapeClass["_class"] = &ShapeClass;
    ShapeClass["_new"] = nullptr;
    ShapeClass["_parent"] = nullptr;

    SquareClass["_classname"] = new string("Square");
    SquareClass["perimeter"] = square_perimeter;
    SquareClass["area"] = square_area;
    SquareClass["larger"] = shape_larger;
    SquareClass["_new"] = square_new;
    SquareClass["_delete"] = square_delete;
    SquareClass["_parent"] = &ShapeClass;

    CircleClass["_classname"] = new string("Circle");
    CircleClass["perimeter"] = circle_perimeter;
    CircleClass["area"] = circle_area;
    CircleClass["larger"] = shape_larger;
    CircleClass["_new"] = circle_new;
    CircleClass["_delete"] = circle_delete;
    CircleClass["_parent"] = &ShapeClass;

    vector<shape_obj *> examples = { make(&SquareClass, "sq2", 5), make(&CircleClass, "ci2", 4) };

    for (const auto thing : examples)
    {
        string n = *(string *)(*thing)["name"];
        //string c = *(string *)(*(shape_obj *)(*thing)["_class"])["_classname"];
        string c = type_of(thing);
        double p = shape_fn_call(thing, "perimeter");
        double a = shape_fn_call(thing, "area");
        double d = shape_fn_call(thing, "density", 5);

        double larger_size = 30;
        bool is_larger = shape_fn_call(thing, "larger", larger_size) > 0;
        bool is_shape = is_instance_of(thing, &ShapeClass);
        bool is_square = is_instance_of(thing, &SquareClass);
        cout << n << " is a " << c << " and has a perimeter " << p << " and area " << a << " with density " << d << endl;
        cout << "is " << n << " larger than " << larger_size << "? " << bool_str(is_larger) << endl;
        cout << "is Shape? " << bool_str(is_shape) << " is Square? " << bool_str(is_square) << endl;
    }

    for (const auto thing : examples)
    {
        destroy(thing);
    }
}

void objects_main()
{
    objects_example1();
    objects_example2();
}
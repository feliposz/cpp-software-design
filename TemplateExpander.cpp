// Inspired by Chapter 12: A Template Expander
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/template

#include "libxml2/libxml/HTMLparser.h"

#include <assert.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <string>

using namespace std;

vector<string> split(string str, string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> result;

    while ((pos_end = str.find(delimiter, pos_start)) != string::npos)
    {
        token = str.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        result.push_back(token);
    }

    result.push_back(str.substr(pos_start));
    return result;
}

class Environment
{
    vector<map<string, string>> stack;

public:
    Environment()
    {
        push();
    }

    void push()
    {
        stack.push_back({});
    }

    void pop()
    {
        stack.pop_back();
    }

    void set(string identifier, string value)
    {
        for (auto s = stack.rbegin(); s != stack.rend(); s++)
        {
            if ((*s).count(identifier))
            {
                (*s)[identifier] = value;
                return;
            }
        }
        stack.back()[identifier] = value;
    };

    string get(string identifier)
    {
        for (auto s = stack.rbegin(); s != stack.rend(); s++)
        {
            if ((*s).count(identifier))
            {
                return (*s)[identifier];
            }
        }
        assert(!"undeclared");
    }
};

class BaseVisitor
{
public:
    htmlParserCtxtPtr parser;
    xmlNode *root;

    BaseVisitor(const string &data)
    {
        parser = htmlCreateMemoryParserCtxt(data.c_str(), data.length());
        if (!parser)
        {
            fprintf(stderr, "error creating context\n");
            exit(1);
        }
        htmlCtxtUseOptions(parser, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
        htmlParseDocument(parser);
        root = xmlDocGetRootElement(parser->myDoc);
        if (!root)
        {
            htmlFreeParserCtxt(parser);
            fprintf(stderr, "empty document\n");
            exit(1);
        }
    }

    ~BaseVisitor()
    {
        if (root)
        {
            xmlFree(root);
        }
        if (parser)
        {
            htmlFreeParserCtxt(parser);
        }
    }

    void walk()
    {
        walk(root);
    }

    void walk(xmlNode *node)
    {
        if (open(node))
        {
            for (xmlNode *child = node->children; child; child = child->next)
            {
                walk(child);
            }
            close(node);
        }
    }

    virtual bool open(xmlNode *node) { return false; }

    virtual void close(xmlNode *node) {}
};

class Expander : public BaseVisitor
{
    vector<string> output;
    Environment &env;

public:
    Expander(const string &data, Environment &env) : BaseVisitor(data), env(env)
    {
    }

    bool open(xmlNode *node) override
    {
        if (node->type == XML_TEXT_NODE)
        {
            output.push_back((char *)node->content);
            return false;
        }
        else if (has_handler(node))
        {
            for (xmlAttr *attr = node->properties; attr; attr = attr->next)
            {
                if (!strcmp((char *)attr->name, "z-if"))
                {
                    return handle_if(node, attr);
                }
                else if (!strcmp((char *)attr->name, "z-loop"))
                {
                    return handle_loop(node, attr);
                }
                else if (!strcmp((char *)attr->name, "z-range"))
                {
                    return handle_range(node, attr);
                }
                else if (!strcmp((char *)attr->name, "z-num"))
                {
                    return handle_num(node, attr);
                }
                else if (!strcmp((char *)attr->name, "z-var"))
                {
                    return handle_var(node, attr);
                }
            }
        }
        else if (node->type == XML_ELEMENT_NODE)
        {
            show_tag(node);
            return true;
        }
        return false;
    }

    void close(xmlNode *node) override
    {
        show_tag(node, true);
    }

    bool has_handler(xmlNode *node)
    {
        int count = 0;
        for (xmlAttr *attr = node->properties; attr; attr = attr->next)
        {
            if (!strcmp((char *)attr->name, "z-if") ||
                !strcmp((char *)attr->name, "z-loop") ||
                !strcmp((char *)attr->name, "z-range") ||
                !strcmp((char *)attr->name, "z-num") ||
                !strcmp((char *)attr->name, "z-var"))
            {
                count++;
            }
        }
        if (count > 1)
        {
            assert(!"Should be exactly one handler");
        }
        return count == 1;
    }

    bool is_truthy(const string &condition)
    {
        return condition == "true" || condition == "True" || condition == "TRUE";
    }

    bool handle_if(xmlNode *node, xmlAttr *attr)
    {
        string condition = env.get((char *)attr->children->content);
        if (is_truthy(condition))
        {
            show_tag(node);
            return true;
        }
        return false;
    }

    bool handle_loop(xmlNode *node, xmlAttr *attr)
    {
        vector<string> loop_variables = split((char *)attr->children->content, ":");
        string &index_variable = loop_variables[0];
        string &data_variable = loop_variables[1];
        vector<string> values = split(env.get(data_variable), ",");
        show_tag(node);
        for (const string &value: values)
        {
            env.push();
            env.set(index_variable, value);
            for (xmlNode *child = node->children; child; child = child->next)
            {
                walk(child);
            }
            env.pop();
        }
        show_tag(node, true);
        return false;
    }

    bool handle_range(xmlNode *node, xmlAttr *attr)
    {
        vector<string> loop_variables = split((char *)attr->children->content, ":");
        assert(loop_variables.size() >= 3);
        string &index_variable = loop_variables[0];
        int start = stoi(loop_variables[1].c_str());
        int end = stoi(loop_variables[2].c_str());
        int step = loop_variables.size() == 3 ? 1 : stoi(loop_variables[3].c_str());
        assert(step != 0);
        assert(((step > 0) && (start <= end)) || ((step < 0) && (start >= end)));
        show_tag(node);
        for (int index = start; ((step > 0) && (index <= end)) || ((step < 0 && index >= end)); index += step)
        {
            env.push();
            env.set(index_variable, to_string(index));
            for (xmlNode *child = node->children; child; child = child->next)
            {
                walk(child);
            }
            env.pop();
        }
        show_tag(node, true);
        return false;
    }

    bool handle_num(xmlNode *node, xmlAttr *attr)
    {
        show_tag(node);
        output.push_back((char *)attr->children->content);
        return true;
    }

    bool handle_var(xmlNode *node, xmlAttr *attr)
    {
        show_tag(node);
        string value = env.get((char *)attr->children->content);
        output.push_back(value);
        return true;
    }

    void show_tag(xmlNode *node, bool closing = false)
    {
        if (closing)
        {
            output.push_back("</");
            output.push_back((char *)node->name);
            output.push_back(">");
        }
        else
        {
            output.push_back("<");
            output.push_back((char *)node->name);
            for (xmlAttr *attr = node->properties; attr; attr = attr->next)
            {
                if (attr->name[0] != 'z' && attr->name[1] != '-')
                {
                    output.push_back(" ");
                    output.push_back((char *)attr->name);
                    if (attr->children)
                    {
                        output.push_back("=\"");
                        output.push_back((char *)attr->children->content);
                        output.push_back("\"");
                    }
                }
            }
            output.push_back(">");
        }
    }

    string get_result()
    {
        string result;
        for (const auto &s : output)
        {
            result += s;
        }
        return result;
    }
};

void test_static()
{
    string tmpl = "<html lang=\"en\"><body><h1 class=\"header\">Static Text</h1><p id=\"par\">test</p></body></html>";
    Environment env;
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    assert(result == tmpl);
}

void test_z_num()
{
    string tmpl = "<html><body><p><span z-num=\"123\"/></p></body></html>";
    Environment env;
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    string expect = "<html><body><p><span>123</span></p></body></html>";
    assert(result == expect);
}

void test_z_var()
{
    string tmpl = "<html><body><p><span z-var=\"varName\"/></p></body></html>";
    Environment env;
    env.set("varName", "varValue");
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    string expect = "<html><body><p><span>varValue</span></p></body></html>";
    assert(result == expect);
}

void test_z_var2()
{
    string tmpl = "<html><body><p><span z-var=\"firstVar\" /></p><p><span z-var=\"secondVar\" /></p></body></html>";
    Environment env;
    env.set("firstVar", "firstValue");
    env.set("secondVar", "secondValue");
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    string expect = "<html><body><p><span>firstValue</span></p><p><span>secondValue</span></p></body></html>";
    assert(result == expect);
}

void test_z_if()
{
    string tmpl = "<html><body><p z-if=\"yes\">Should be shown.</p><p z-if=\"no\">Should <em>not</em> be shown.</p></body></html>";
    Environment env;
    env.set("yes", "true");
    env.set("no", "false");
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    string expect = "<html><body><p>Should be shown.</p></body></html>";
    assert(result == expect);
}

void test_z_loop()
{
    string tmpl = "<html><body><ul z-loop=\"item:names\"><li><span z-var=\"item\"/></li></ul></body></html>";
    Environment env;
    env.set("names", "Johnson,Vaughan,Jackson");
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    string expect = "<html><body><ul><li><span>Johnson</span></li><li><span>Vaughan</span></li><li><span>Jackson</span></li></ul></body></html>";
    assert(result == expect);
}

void test_z_range()
{
    string tmpl = "<html><body><ul z-range=\"item:1:5\"><li z-var=\"item\"></li></ul></body></html>";
    Environment env;
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    string expect = "<html><body><ul><li>1</li><li>2</li><li>3</li><li>4</li><li>5</li></ul></body></html>";
    assert(result == expect);
}

void test_z_range_reverse()
{
    string tmpl = "<html><body><ul z-range=\"item:10:0:-3\"><li z-var=\"item\"></li></ul></body></html>";
    Environment env;
    Expander exp(tmpl, env);
    exp.walk();
    string result = exp.get_result();
    string expect = "<html><body><ul><li>10</li><li>7</li><li>4</li><li>1</li></ul></body></html>";
    assert(result == expect);
}

void template_main()
{
    cout << "Template Expander:" << endl;
    test_static();
    test_z_num();
    test_z_var();
    test_z_var2();
    test_z_if();
    test_z_loop();
    test_z_range();
    test_z_range_reverse();
    cout << "All tests passed" << endl;
}
#include "libxml2/libxml/HTMLparser.h"

#include <assert.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <string>

using namespace std;

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
public:
    Expander(const string &data) : BaseVisitor(data)
    {
    }

    bool open(xmlNode *node) override
    {
        if (node->type == XML_TEXT_NODE)
        {
            output.push_back((char *)node->content);
            return false;
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
    Expander e(tmpl);
    e.walk();
    string result = e.get_result();
    assert(tmpl == result);
}

void template_main()
{
    cout << "Template Expander:" << endl;
    test_static();
    cout << "All tests passed" << endl;
}
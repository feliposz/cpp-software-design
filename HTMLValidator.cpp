#include "libxml2/libxml/HTMLparser.h"

#include <iostream>
#include <string>
#include <map>
#include <set>

using namespace std;

void indent(int depth)
{
    for (int d = 0; d < depth; d++)
    {
        printf("  ");
    }
}

void walk_html_tree(xmlNode *a_node, int depth = 0, bool is_inline = false)
{
    for (xmlNode *cur_node = a_node; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (!is_inline)
            {
                indent(depth);
            }
            printf("Element: %s", cur_node->name);
            for (xmlAttr *cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next)
            {
                printf(cur_attr == cur_node->properties ? " {" : ", ");
                printf("'%s' = ", cur_attr->name);
                for (xmlNode *attr_child = cur_attr->children; attr_child; attr_child = attr_child->next)
                {
                    walk_html_tree(attr_child, depth + 1, true);
                }
            }
            if (cur_node->properties)
            {
                printf("}");
            }
            printf("\n");
            walk_html_tree(cur_node->children, depth + 1);
        }
        else if (cur_node->type == XML_TEXT_NODE)
        {
            if (is_inline)
            {
                printf("'%s'", cur_node->content);
            }
            else
            {
                if (!is_inline)
                {
                    indent(depth);
                }
                printf("Text: '");
                for (xmlChar *c = cur_node->content; *c; c++)
                {
                    if (*c == '\n')
                    {
                        printf("\\n");
                    }
                    else if (*c == '\r')
                    {
                        printf("\\r");
                    }
                    else if (*c == '\t')
                    {
                        printf("\\t");
                    }
                    else if (*c == ' ')
                    {
                        printf("_");
                    }
                    else
                    {
                        putchar(*c);
                    }
                }
                printf("'\n");
            }
        }
        else
        {
            fprintf(stderr, "not implemented type: %d\n", cur_node->type);
            exit(1);
        }
    }
}

void parse_html_document(const string &data)
{
    htmlParserCtxtPtr parser = htmlCreateMemoryParserCtxt(data.c_str(), data.length());
    if (!parser)
    {
        fprintf(stderr, "error creating context\n");
        return;
    }
    htmlCtxtUseOptions(parser, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    htmlParseDocument(parser);
    xmlNode *root = xmlDocGetRootElement(parser->myDoc);
    if (!root)
    {
        fprintf(stderr, "empty document\n");
        return;
    }

    printf("\n=== Begin ===\n\n");
    walk_html_tree(root);
    printf("\n=== End ===\n");

    xmlFree(root);
    htmlFreeParserCtxt(parser);
}

void catalog_html_tree(xmlNode *a_node, map<string, set<string>> &catalog)
{
    if (a_node && a_node->name)
    {
        string parent_name((char *)a_node->name);
        if (catalog.count(parent_name) == 0)
        {
            catalog[parent_name] = {};
        }
        for (xmlNode *cur_node = a_node->children; cur_node; cur_node = cur_node->next)
        {
            if (cur_node->type == XML_ELEMENT_NODE)
            {
                string child_name((char *)cur_node->name);
                catalog[parent_name].emplace(child_name);
                catalog_html_tree(cur_node, catalog);
            }
        }
    }
}

void catalog_html_document(const string &data)
{
    htmlParserCtxtPtr parser = htmlCreateMemoryParserCtxt(data.c_str(), data.length());
    if (!parser)
    {
        fprintf(stderr, "error creating context\n");
        return;
    }
    htmlCtxtUseOptions(parser, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    htmlParseDocument(parser);
    xmlNode *root = xmlDocGetRootElement(parser->myDoc);
    if (!root)
    {
        fprintf(stderr, "empty document\n");
        return;
    }

    map<string, set<string>> catalog;

    catalog_html_tree(root, catalog);

    xmlFree(root);
    htmlFreeParserCtxt(parser);

    for (const auto &elem : catalog)
    {
        cout << elem.first << ": ";
        bool is_first_child = true;
        for (const auto &child : elem.second)
        {
            if (is_first_child)
            {
                is_first_child = false;
            }
            else
            {
                cout << ", ";
            }
            cout << child;
        }
        cout << endl;
    }
}

void test_parsing()
{
    parse_html_document(R"(
        <html lang="en">
        <body class="outline narrow">
        <h1>Title</h1>
        <p align="left" align="right">paragraph</p>
        </body>
        </html>
    )");
}

void test_catalog()
{
    catalog_html_document(R"(
        <html>
          <head>
            <title>Software Design by Example</title>
          </head>
          <body>
            <h1>Main Title</h1>
            <p>introductory paragraph</p>
            <ul>
              <li>first item</li>
              <li>second item is <em>emphasized</em></li>
            </ul>
          </body>
        </html>
    )");
}

void validator_main()
{
    LIBXML_TEST_VERSION;

    test_parsing();
    test_catalog();
}
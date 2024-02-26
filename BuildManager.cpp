// Inspired by Chapter 19: A Build Manager
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/build/

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

struct BuildTarget
{
    string name;
    vector<string> depends;
    string rule;
    int timestamp = -1;
};

typedef vector<BuildTarget> BuildConfig;

class BuildManagerException : public exception {};
class DuplicateTarget : public BuildManagerException {};
class InvalidTargetName : public BuildManagerException {};
class InvalidTargetRule : public BuildManagerException {};
class UnknownDepend : public BuildManagerException {};
class CircularDepends : public BuildManagerException {};

class BuildBase
{
public:
    BuildBase(const BuildConfig &config)
    {
        for (const BuildTarget &target : config)
        {
            if (targets.count(target.name) == 0)
            {
                targets[target.name] = target;
            }
            else
            {
                throw DuplicateTarget();
            }
        }
        for (const BuildTarget &target : config)
        {
            check(target);
        }
    }

    virtual vector<string> build()
    {
        vector<string> result; 
        vector<string> ordered = topo_sort();
        for (const string &target : ordered)
        {
            auto &current = targets[target];
            bool needs_update = false;
            if (current.timestamp == -1) // Forced update
            {
                needs_update = true;
            }
            else
            {
                for (const string &depend : current.depends)
                {
                    if (current.timestamp < targets[depend].timestamp)
                    {
                        needs_update = true;
                        break;
                    }
                }
            }
            if (needs_update)
            {
                result.push_back(current.rule);
            }
        }
        return result;
    }

protected:
    map<string, BuildTarget> targets;

    void check(const BuildTarget &target)
    {
        if (target.name.empty())
        {
            throw InvalidTargetName();
        }
        if (target.rule.empty())
        {
            throw InvalidTargetRule();
        }
        for (const string &depend : target.depends)
        {
            if (targets.count(depend) == 0)
            {
                throw UnknownDepend();
            }
        }
    }

    vector<string> topo_sort()
    {
        vector<string> result;
        map<string, set<string>> graph;
        for (const auto &[name, target] : targets)
        {
            graph[name] = {};
            for (const string &depend : target.depends)
            {
                graph[name].emplace(depend);
            }
        }
        while (!graph.empty())
        {
            string next;
            for (const auto &[target, depends] : graph)
            {
                if (depends.size() == 0)
                {
                    next = target;
                }
            }
            if (!next.empty())
            {
                for (auto &[target, depends]: graph)
                {
                    depends.erase(next);
                }
                graph.erase(next);
                result.push_back(next);
            }
            else if (!graph.empty())
            {
                throw CircularDepends();
            }
        }
        return result;
    }
};

void test_build_base()
{
    try
    {
        BuildBase base({ { "A", {}, "build A" },{ "A", {}, "build A" } });
        assert(!"DuplicateTarget not thrown");
    }
    catch (DuplicateTarget)
    {
    }

    try
    {
        BuildBase base({ { "", {}, "build A" } });
        assert(!"InvalidTargetName not thrown");
    }
    catch (InvalidTargetName)
    {
    }

    try
    {
        BuildBase base({ { "A", {}, "" } });
        assert(!"InvalidTargetRule not thrown");
    }
    catch (InvalidTargetRule)
    {
    }

    try
    {
        BuildBase base({ { "A", {"C"}, "build A" }, { "B", {}, "build B" } });
        assert(!"UnknownDepend not thrown");
    }
    catch (UnknownDepend)
    {
    }

    {
        BuildBase base({ { "A", {"B"}, "build A" }, { "B", {}, "build B" } });
    }
}

void test_topo_sort()
{
    {
        BuildBase base({ { "A", {"B", "C"}, "build A" }, { "B", {"D"}, "build B" }, { "C", {"D"}, "build C" }, { "D", {}, "build D" } });
        vector<string> result = base.build();
        vector<string> expect = { "build D", "build C", "build B", "build A" };
        assert(result == expect);
    }

    try
    {
        BuildBase base({ { "A", {"B"}, "build A" }, { "B", {"A"}, "build B" } });
        vector<string> result = base.build();
        assert(!"CircularDepends not thrown");
    }
    catch (CircularDepends)
    {
    }
}

void test_timestamps()
{
    BuildBase base({ { "A", {"B", "C"}, "build A", 0 }, { "B", {"D"}, "build B", 0 }, { "C", {"D"}, "build C", 1 }, { "D", {}, "build D", 1 } });
    vector<string> result = base.build();
    vector<string> expect = { "build B", "build A" };
    assert(result == expect);
}

void build_main()
{
    cout << "Build Manager:" << endl;
    test_build_base();
    test_topo_sort();
    test_timestamps();
    cout << "All tests passed" << endl;
}
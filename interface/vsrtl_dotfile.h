#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>

namespace vsrtl {

class DotFile {
public:
    DotFile(const std::string& filename, const std::string& graphname);
    ~DotFile();

    void addVar(const std::string& var, const std::string& label);
    void addEdge(const std::string& from, const std::string& to);
    void dump();

private:
    struct Var {
        std::string name;
        std::string label;
    };
    std::map<std::string, Var> m_vars;
    std::map<std::string, std::vector<std::string>> m_edges;
    std::ofstream m_file;
    std::string m_graphName;
};

}  // namespace vsrtl

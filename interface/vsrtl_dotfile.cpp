#include "vsrtl_dotfile.h"

#include <assert.h>

namespace vsrtl {

DotFile::DotFile(const std::string& filename, const std::string& graphname) {
    m_file.open(filename, std::ios_base::trunc);
    m_graphName = graphname;
}

DotFile::~DotFile() {
    m_file.close();
}

void DotFile::addVar(const std::string& name, const std::string& label) {
    assert(m_vars.count(name) == 0);
    Var var;
    var.name = name;
    var.label = label;

    m_vars[name] = var;
}
void DotFile::addEdge(const std::string& from, const std::string& to) {
    assert(m_vars.count(from) != 0);
    assert(m_vars.count(to) != 0);
    m_edges[from].push_back(to);
}
void DotFile::dump() {
    m_file << "digraph " << m_graphName << " {\n";
    for (const auto& v : m_vars) {
        m_file << "    " << v.second.name << " [label=\"" << v.second.label << "\"];\n";
    }

    for (const auto& from : m_edges) {
        for (const auto& to : from.second) {
            m_file << "    " << from.first << " -> " << to << ";\n";
        }
    }

    m_file << "}\n";
}

}  // namespace vsrtl

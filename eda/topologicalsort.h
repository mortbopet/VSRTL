#ifndef TOPOLOGICALSORT_H
#define TOPOLOGICALSORT_H

#include <deque>
#include <map>
#include <vector>

namespace vsrtl {
namespace eda {

namespace {
template <typename T, typename F>
void topologicalSortUtil(T* node, F adjacencyFunction, std::map<T*, bool>& visited, std::deque<T*>& stack) {
    visited[node] = true;

    for (const auto& cc : (node->*adjacencyFunction)()) {
        if (visited.find(cc.first) != visited.end() && !visited[cc.first]) {
            topologicalSortUtil(cc.first, adjacencyFunction, visited, stack);
        }
    }
    stack.push_front(node);
}

template <typename T, typename F>
std::deque<T*> topologicalSort(const std::vector<T*>& nodes, F adjacencyFunction) {
    std::map<T*, bool> visited;
    std::deque<T*> stack;

    for (const auto& n : nodes)
        visited[n] = false;

    for (const auto& node : visited) {
        if (!node.second) {
            topologicalSortUtil(node.first, adjacencyFunction, visited, stack);
        }
    }

    return stack;
}

}  // namespace

}  // namespace eda
}  // namespace vsrtl

#endif  // TOPOLOGICALSORT_H

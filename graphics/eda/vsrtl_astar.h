#pragma once
#include <limits.h>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <QPoint>

namespace vsrtl {

namespace {
template <typename K, typename V>
V getDef(const std::map<K, V>& m, const K& key, const V& defval) {
    typename std::map<K, V>::const_iterator it = m.find(key);
    if (it == m.end()) {
        return defval;
    } else {
        return it->second;
    }
}

template <typename T>
std::vector<T*> reconstructPath(std::map<T*, T*> cameFromMap, T* current) {
    std::vector<T*> totalPath{current};

    while (cameFromMap.count(current) > 0) {
        const auto& cameFrom = cameFromMap[current];
        current = cameFrom;
        totalPath.insert(totalPath.begin(), current);
    }
    return totalPath;
}
}  // namespace

/** Generic A Star shortest path implementation
 * T: Graph node type
 * adjacentFunc: T member function pointer type for retrieving the set of adjacent nodes to a given T node
 * costFunction: A function which given two T nodes will return the heuristic cost between the two nodes
 */
template <typename T, typename F>
std::vector<T*> AStar(T* start, T* goal, F adjacentFunc, std::function<int(T*, T*)> costFunction) {
    // With reference to from https://en.wikipedia.org/wiki/A*_search_algorithm
    // Precondition: start- and stop regions must have their horizontal and vertical capacities pre-decremented for the
    // given number of terminals within them

    // The set of nodes already evaluated
    std::set<T*> closedSet;

    // The set of currently discovered nodes that are not evaluated yet.
    // Initially, only the start node is known.
    std::set<T*> openSet{start};

    // For each node, which node it can most efficiently be reached from.
    // If a node can be reached from many nodes, cameFrom will eventually contain the
    // most efficient previous step.
    std::map<T*, T*> cameFrom;

    // For each node, the cost of getting from the start node to that node.
    std::map<T*, int> gScore;

    // For each node, the total cost of getting from the start node to the goal
    // by passing by that node. That value is partly known, partly heuristic.
    std::map<T*, int> fScore;

    // The cost of going from start to start is zero.
    gScore[start] = 0;

    // For the first node, that value is completely heuristic.
    fScore[start] = costFunction(start, goal);

    T* current = nullptr;
    while (!openSet.empty()) {
        // Find node in openSet with the lowest fScore value
        int lowestScore = INT_MAX;
        for (const auto& node : openSet) {
            if (getDef(fScore, node, INT_MAX) < lowestScore) {
                current = node;
                lowestScore = fScore[node];
            }
        }

        if (current == goal) {
            return reconstructPath(cameFrom, current);
        }

        openSet.erase(current);
        closedSet.emplace(current);

        for (const auto& neighbour : (current->*adjacentFunc)()) {
            if (neighbour == nullptr) {
                continue;
            }
            if (closedSet.count(neighbour) > 0) {
                // Ignore the neighbor which is already evaluated.
                continue;
            }

            // The distance from start to a neighbor
            int tentative_gScore = getDef(gScore, current, INT_MAX) + costFunction(current, neighbour);

            if (openSet.count(neighbour) == 0) {
                // Discovered a new node
                openSet.emplace(neighbour);
            } else if (tentative_gScore >= getDef(gScore, neighbour, INT_MAX)) {
                continue;
            }

            // This path is the best until now. Record it!
            cameFrom[neighbour] = current;
            gScore[neighbour] = tentative_gScore;
            fScore[neighbour] = getDef(gScore, neighbour, INT_MAX) + costFunction(neighbour, goal);
        }
    }
    Q_UNREACHABLE();
}

}  // namespace vsrtl

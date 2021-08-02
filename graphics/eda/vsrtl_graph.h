#pragma once

#include <assert.h>
#include <algorithm>
#include <vector>

namespace vsrtl {
namespace eda {

class Vertex {
    template <typename T>
    constexpr void assertSubclass() const {
        static_assert(std::is_base_of<Vertex, T>::value, "T must subclass Vertex");
    }

public:
    virtual ~Vertex() = default;

    /**
     * @brief addNeighbour
     * @param vt: vertex to add
     * @returns the index of the vertex in the neighbour set
     */
    size_t addNeighbour(Vertex* vt) {
        m_neighbours.push_back(vt);
        return m_neighbours.size() - 1;
    }

    template <typename T>
    std::vector<T*> neighbours(const std::function<bool(T*)>& pred = [](T*) { return true; }) const {
        assertSubclass<T>();
        std::vector<T*> _neighbours;
        for (const auto& n : m_neighbours) {
            T* _n = static_cast<T*>(n);
            if (pred(_n)) {
                _neighbours.push_back(_n);
            }
        }
        return _neighbours;
    }

    const Vertex* neighbour(size_t i) const { return m_neighbours.at(i); }
    Vertex* neighbour(size_t i) { return m_neighbours.at(i); }
    Vertex*& neighbourRef(size_t i) { return m_neighbours.at(i); }

    template <typename T>
    const T* neighbour(size_t i) const {
        assertSubclass<T>();
        return static_cast<T*>(m_neighbours.at(i));
    }

    template <typename T>
    T* neighbour(size_t i) {
        assertSubclass<T>();
        return const_cast<T*>(static_cast<const Vertex&>(*this).neighbour<T>(i));
    }

    void neighbour(size_t i, Vertex* vt) {
        assert(i < m_neighbours.size() && "i >= m_neighbours.size()");
        m_neighbours.at(i) = vt;
    }

private:
    std::vector<Vertex*> m_neighbours;
};

template <class T>
class Graph {
    static_assert(std::is_base_of<T, Vertex>::value, "T must subclass Vertex");

public:
    Graph();

    const std::vector<T>& vertices() const;

protected:
    std::vector<T> m_vertices;
};

}  // namespace eda
}  // namespace vsrtl

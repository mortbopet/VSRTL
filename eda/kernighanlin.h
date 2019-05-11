#ifndef KERNIGHANLIN_H
#define KERNIGHANLIN_H

#include <limits.h>
#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace vsrtl {
namespace eda {

namespace {
template <typename T>
void largestRunningSum(const T& c, unsigned int& i_max, int& sum_max) {
    int sum = c[0];
    sum_max = sum;
    i_max = 0;
    for (unsigned int i = 1; i < c.size(); i++) {
        const auto newSum = sum + c[i];
        if (newSum > sum) {
            i_max = i;
            sum_max = newSum;
        }
        sum = newSum;
    }
}

template <typename T>
void splitContainer(const T& in, T& A, T& B) {
    unsigned int i = 0;
    for (const auto& it : in) {
        if (i < (in.size() / 2)) {
            A.emplace(it);
        } else {
            B.emplace(it);
        }
        i++;
    }
}

template <typename T, typename F>
int KLdValue(T* node, const std::set<T*>& A, const std::set<T*>& B, F connectedComponentsFunc) {
    // Assert that the node is only present in one of the sets
    Q_ASSERT((A.count(node) == 0) ^ (B.count(node) == 0));
    const auto& internalSet = A.count(node) > 0 ? A : B;
    const auto& externalSet = B.count(node) > 0 ? A : B;

    int I_cost = 0;
    int E_cost = 0;

    // Compute internal and external costs
    for (const auto& c : (node->*connectedComponentsFunc)()) {
        if (internalSet.count(c.first) > 0) {
            I_cost += c.second;
        } else if (externalSet.count(c.first) > 0) {
            E_cost += c.second;
        } else {
            // Ignore connections to components which are not considered part of the current partitioning run
        }
    }

    return E_cost - I_cost;
}

}  // namespace

template <typename T, typename F>
std::pair<std::set<T*>, std::set<T*>> KernighanLin(const std::set<T*>& graph, F connectedComponentsFunc) {
    Q_ASSERT(graph.size() > 1);
    // Create an initial balanced distribution
    std::set<T*> A, B;
    splitContainer(graph, A, B);

    // Initially all nodes may be selected and the locked nodes of each set are empty
    std::set<T*> A_locked, B_locked;

    int g_max = INT_MIN;
    do {
        // A maximum number of swaps has been performed if either of the locked sets are equal to half of the original
        // set size. Note that this adds support of when graph is an uneven number of nodes.
        if (A_locked.size() == graph.size() / 2) {
            break;
        }
        // Calculate D value for each node in A, B,
        std::map<T*, int> D;
        for (const auto& c : graph) {
            D[c] = KLdValue(c, A, B, connectedComponentsFunc);
        }

        // let gv, av, and bv be empty lists
        std::vector<T*> av, bv;
        std::vector<int> gv;

        // The nodes to be considered in this pass are all nodes of A and B which have not previously been moved
        std::set<T*> A_pass;
        std::set<T*> B_pass;
        std::set_difference(A.begin(), A.end(), A_locked.begin(), A_locked.end(),
                            std::inserter(A_pass, A_pass.begin()));
        std::set_difference(B.begin(), B.end(), B_locked.begin(), B_locked.end(),
                            std::inserter(B_pass, B_pass.begin()));
        const size_t passSize = (A_pass.size() + B_pass.size());
        for (unsigned int i = 0; i < passSize / 2; i++) {
            // find a from A and b from B, such that g = D[a] + D[b] - 2*c(a, b) is maximal
            T* a = nullptr;
            T* b = nullptr;
            int g_pass_max = INT_MIN;
            for (const auto& a_it : A_pass) {
                for (const auto& b_it : B_pass) {
                    // Cost between two nodes is equal to the D-value between the two nodes.
                    const int g = D[a_it] + D[b_it] - 2 * KLdValue(a_it, {a_it}, {b_it}, connectedComponentsFunc);
                    if (g > g_pass_max) {
                        a = a_it;
                        b = b_it;
                        g_pass_max = g;
                    }
                }
            }
            Q_ASSERT(a != nullptr && b != nullptr);
            // a and b which maximizes g has been found. Add values to gv, av, bv
            gv.push_back(g_pass_max);
            av.push_back(a);
            bv.push_back(b);
            // remove a and b from further consideration in this pass.
            A_pass.erase(a);
            B_pass.erase(b);

            // Update D values for the elements of A_pass, B_pass
            for (const auto& c : A_pass) {
                D[c] = KLdValue(c, A_pass, B_pass, connectedComponentsFunc);
            }
            for (const auto& c : A_pass) {
                D[c] = KLdValue(c, A_pass, B_pass, connectedComponentsFunc);
            }
        }

        // find index i which maximizes g_max, the sum of gv[0],...,gv[i]
        unsigned int i_max;
        largestRunningSum(gv, i_max, g_max);
        if (g_max > 0) {
            // Permanently exchange av[0],av[1],...,av[i] with bv[0],bv[1],...,bv[i]
            for (size_t i = 0; i <= i_max; i++) {
                A.erase(av[i]);
                A.emplace(bv[i]);
                B.erase(bv[i]);
                B.emplace(av[i]);
                A_locked.emplace(bv[i]);
                B_locked.emplace(av[i]);
            }
        }
    } while (g_max > 0);

    // A min-cut partitioned graph can now be returned
    return {A, B};
}

}  // namespace eda

}  // namespace vsrtl

#endif  // KERNIGHANLIN_H

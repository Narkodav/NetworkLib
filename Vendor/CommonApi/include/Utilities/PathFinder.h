#pragma once
#include "../Namespaces.h"

#include <queue>
#include <unordered_set>
#include <functional>
#include <vector>
#include <map>

// path finder class
// implements A star in findPath and Dijkstra's algorithm for all the others
// do not use findPath if your context cannot estimate the shortest path (can't create an admissible heuristic)
// do not use if the graph weights of your context have negative costs, use Bellman-Ford algorithm for that (not implemented here)
// might be not optimal if memory is limited
namespace Utilities
{
    template<typename Context>
    class PathFinder {
    public:
        // Required types that Context must define
        using Node = typename Context::Node;
        using Cost = typename Context::Cost;
        using Path = typename Context::Path;
        using NodeHash = typename Context::NodeHash;

        struct PathCost {
            Path path;
            Cost cost;
        };

        struct SearchResult {
            Path path;
            Cost cost;
            Node foundNode;
            bool found;
        };

    private:
        struct NodeRecord {
            Node node;
            Path path;
            Cost costSoFar;
            Cost estimatedCost;
        };

        struct CompareNodes {
            bool operator()(const NodeRecord& a, const NodeRecord& b) {
                return (a.costSoFar + a.estimatedCost) > (b.costSoFar + b.estimatedCost);
            }
        };

        const Context& m_context;

    public:

        PathFinder(const Context& context) : m_context(context) {}

        Path findPath(const Node& start, const Node& goal) {
            std::priority_queue<NodeRecord, std::vector<NodeRecord>, CompareNodes> openSet;
            std::unordered_set<Node, NodeHash> closedSet;
            std::unordered_map<Node, Cost, NodeHash> bestCosts;

            // Initialize with start node
            openSet.push({
                start,
                Path(),
                Cost(),
                m_context.estimateCost(start, goal)
                });
            bestCosts[start] = Cost();

            while (!openSet.empty()) {
                NodeRecord current = openSet.top();
                openSet.pop();

                if (m_context.isGoal(current.node, goal)) {
                    return current.path;
                }

                if (closedSet.contains(current.node)) {
                    continue;
                }

                closedSet.insert(current.node);

                for (const auto& neighbor : m_context.getNeighbors(current.node)) {
                    if (closedSet.contains(neighbor)) {
                        continue;
                    }

                    Cost newCost = current.costSoFar + m_context.getCost(current.node, neighbor);

                    // Skip if we already found a better path to this neighbor
                    auto it = bestCosts.find(neighbor);
                    if (it != bestCosts.end() && it->second <= newCost) {
                        continue;
                    }

                    // Record this better path
                    bestCosts[neighbor] = newCost;

                    Path newPath = current.path;
                    m_context.addToPath(newPath, neighbor);

                    Cost heuristic = m_context.estimateCost(neighbor, goal);
                    openSet.push({
                        neighbor,
                        newPath,
                        newCost,
                        newCost + heuristic  // f = g + h
                        });
                }
            }

            return Path(); // No path found
        }

        std::vector<PathCost> exploreWithinCost(const Node& start, const Cost& maxCost) {
            std::priority_queue<NodeRecord, std::vector<NodeRecord>, CompareNodes> openSet;
            std::unordered_set<Node, NodeHash> closedSet;
            std::vector<PathCost> validPaths;

            {
                Path path;
                m_context.addToPath(path, start);
                openSet.push({ start, path, Cost(), Cost() });
            }

            while (!openSet.empty()) {
                NodeRecord current = openSet.top();
                openSet.pop();

                // Skip if we've found this node through a shorter path
                if (closedSet.contains(current.node)) {
                    continue;
                }

                closedSet.insert(current.node);

                // Store valid path
                if (current.node != start) {  // More explicit about the intention
                    validPaths.push_back({ current.path, current.costSoFar });
                }

                // Explore neighbors if we haven't exceeded max cost
                for (const auto& neighbor : m_context.getNeighbors(current.node)) {
                    if (closedSet.contains(neighbor)) {
                        continue;
                    }

                    Cost newCost = current.costSoFar + m_context.getCost(current.node, neighbor);

                    // Skip if exceeds budget
                    if (newCost > maxCost) {
                        continue;
                    }

                    Path newPath = current.path;
                    m_context.addToPath(newPath, neighbor);

                    openSet.push({
                        neighbor,
                        newPath,
                        newCost,
                        Cost()  // No heuristic needed for this search
                        });
                }
            }

            return validPaths;
        }

        // Overload that returns paths grouped by cost
        std::map<Cost, std::vector<Path>> exploreWithinCostGrouped(const Node& start, const Cost& maxCost) {
            std::map<Cost, std::vector<Path>> pathsByDistance;
            auto paths = exploreWithinCost(start, maxCost);

            for (const auto& pathCost : paths) {
                pathsByDistance[pathCost.cost].push_back(pathCost.path);
            }

            return pathsByDistance;
        }

        // Get only the furthest reachable paths within cost
        std::vector<Path> getFurthestPaths(const Node& start, const Cost& maxCost) {
            auto groupedPaths = exploreWithinCostGrouped(start, maxCost);
            return groupedPaths.empty() ? std::vector<Path>() : groupedPaths.rbegin()->second;
        }

        // Search for a node that satisfies a predicate
        SearchResult findNodeWhere(const Node& start,
            std::function<bool(const Node&)> predicate,
            const Cost& maxCost = std::numeric_limits<Cost>::max()) {
            std::priority_queue<NodeRecord, std::vector<NodeRecord>, CompareNodes> openSet;
            std::unordered_set<Node, NodeHash> closedSet;

            {
                Path path;
                m_context.addToPath(path, start);
                openSet.push({ start, path, Cost(), Cost() });
            }

            while (!openSet.empty()) {
                NodeRecord current = openSet.top();
                openSet.pop();

                // Check if current node satisfies the predicate
                if (predicate(current.node)) {
                    return SearchResult{
                        current.path,
                        current.costSoFar,
                        current.node,
                        true
                    };
                }

                if (closedSet.contains(current.node)) {
                    continue;
                }

                closedSet.insert(current.node);

                // Stop exploring if we've exceeded max cost
                if (current.costSoFar > maxCost) {
                    continue;
                }

                for (const auto& neighbor : m_context.getNeighbors(current.node)) {
                    if (closedSet.contains(neighbor)) {
                        continue;
                    }

                    Cost newCost = current.costSoFar + m_context.getCost(current.node, neighbor);

                    if (newCost > maxCost) {
                        continue;
                    }

                    Path newPath = current.path;
                    m_context.addToPath(newPath, neighbor);

                    openSet.push({
                        neighbor,
                        newPath,
                        newCost,
                        Cost()  // No heuristic needed for this search
                        });
                }
            }

            // No matching node found
            return SearchResult{ Path(), Cost(), Node(), false };
        }

        // Find all nodes that match a predicate within max cost
        std::vector<SearchResult> findAllNodesWhere(const Node& start,
            std::function<bool(const Node&)> predicate,
            const Cost& maxCost = std::numeric_limits<Cost>::max()) {
            std::priority_queue<NodeRecord, std::vector<NodeRecord>, CompareNodes> openSet;
            std::unordered_set<Node, NodeHash> closedSet;
            std::vector<SearchResult> results;

            {
                Path path;
                m_context.addToPath(path, start);
                openSet.push({ start, path, Cost(), Cost() });
            }

            while (!openSet.empty()) {
                NodeRecord current = openSet.top();
                openSet.pop();

                if (closedSet.contains(current.node)) {
                    continue;
                }

                closedSet.insert(current.node);

                // Check if current node satisfies the predicate
                if (predicate(current.node)) {
                    results.push_back(SearchResult{
                        current.path,
                        current.costSoFar,
                        current.node,
                        true
                        });
                }

                // Stop exploring this path if we've exceeded max cost
                if (current.costSoFar > maxCost) {
                    continue;
                }

                for (const auto& neighbor : m_context.getNeighbors(current.node)) {
                    if (closedSet.contains(neighbor)) {
                        continue;
                    }

                    Cost newCost = current.costSoFar + m_context.getCost(current.node, neighbor);

                    if (newCost > maxCost) {
                        continue;
                    }

                    Path newPath = current.path;
                    m_context.addToPath(newPath, neighbor);

                    openSet.push({
                        neighbor,
                        newPath,
                        newCost,
                        Cost()
                        });
                }
            }

            return results;
        }
    };
}
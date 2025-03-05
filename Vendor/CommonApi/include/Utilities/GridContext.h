#pragma once
#include "../Namespaces.h"

#include <vector>
#include <functional>
#include <array>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "PathFinderContext.h"

// Grid context implementation
// example implementation for grid worlds
// m_isReachable lambda determines whether the next node is reachable
namespace Utilities
{
    struct ivec3Hash {
        size_t operator()(const glm::ivec3& node) const {
            // Custom hash for glm::ivec2
            return std::hash<int>()(node.x) ^
                std::hash<int>()(node.y);
        }
    };

    class GridContext : public PathFinderContext<glm::ivec3, size_t, std::vector<glm::ivec3>, GridContext> {
    private:
        std::function<bool(Node)> m_isReachable;
        std::vector<glm::ivec3> m_explorableDirections;
    public:
        //this must be defined in child class
        using PathFinderContext = PathFinderContext<glm::ivec3, size_t, std::vector<glm::ivec3>, GridContext>;
        using Node = typename PathFinderContext::Node;
        using Cost = typename PathFinderContext::Cost;
        using Path = typename PathFinderContext::Path;
        using NodeHash = typename PathFinderContext::NodeHash;


        GridContext(auto isReachable, const std::vector<glm::ivec3>& directions) :
            m_isReachable(isReachable), m_explorableDirections(directions) {};

        virtual bool isGoal(const Node& current, const Node& goal) const override {
            return current == goal;
        }

        virtual Cost estimateCost(const Node& from, const Node& to) const override {
            glm::ivec2 result = glm::abs(to - from);
            return result.x + result.y;
        }

        virtual std::vector<Node> getNeighbors(const Node& node) const override {
            std::vector<Node> neighbors;
            for (const auto& dir : m_explorableDirections) {
                Node next = node + dir;
                if (m_isReachable(next)) {
                    neighbors.push_back(next);
                }
            }
            return neighbors;
        }

        virtual Cost getCost(const Node& from, const Node& to) const override {
            glm::ivec3 result = glm::abs(to - from);
            return result.x + result.y;
        }

        virtual void addToPath(Path& path, const Node& node) const override {
            path.push_back(node);
        }

        static std::size_t hashNode(const Node& node) {
            ivec3Hash hash;
            return hash(node);
        }
    };

}
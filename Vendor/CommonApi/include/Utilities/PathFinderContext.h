#pragma once
#include "../Namespaces.h"

#include <vector>

namespace Utilities
{
    template<typename NodeType, typename CostType, typename PathType, typename Derived>
    class PathFinderContext
    {
    public:
        using Node = NodeType;
        using Cost = CostType;
        using Path = PathType;

        struct NodeHash
        {
            std::size_t operator()(const Node& node) const
            {
                return Derived::hashNode(node); // requires hashNode to be implemented in derived contexts
            }
        };

        virtual bool isGoal(const Node& current, const Node& goal) const = 0;

        virtual Cost estimateCost(const Node& from, const Node& to) const = 0;

        virtual std::vector<Node> getNeighbors(const Node& node) const = 0;

        virtual Cost getCost(const Node& from, const Node& to) const = 0;

        virtual void addToPath(Path& path, const Node& node) const = 0;

    };
}

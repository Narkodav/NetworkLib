#pragma once
#ifndef ARRAY_ND_H
#define ARRAY_ND_H
#include "../Namespaces.h"

#include <array>
#include <stdexcept>

namespace Utilities
{
    template<typename Type, std::size_t... Dims>

    class ArrayNd
    {
    private:
        static inline const std::size_t dimensionAmount = sizeof...(Dims);
        static inline constexpr  std::array<std::size_t, dimensionAmount> dimensions = { Dims... };

        static_assert(dimensionAmount > 0, "Array must have at least one dimension");
        static_assert((... && (Dims > 0)), "All dimensions must be greater than 0");

        static constexpr std::array<std::size_t, dimensionAmount> calculateMultipliers(
            const std::array<std::size_t, dimensionAmount>& dims) {
            std::array<std::size_t, dimensionAmount> mults;
            mults[0] = 1;
            for (std::size_t i = 0; i < dimensionAmount - 1; ++i) {
                mults[i + 1] = mults[i] * dims[i];
            }
            return mults;
        }

        static inline constexpr std::size_t calculateTotalSize(
            const std::array<std::size_t, dimensionAmount>& dims)
        {
            std::size_t size = 1;
            for (auto dim : dimensions) {
                size *= dim;
            }
            return size;
        }

        static inline constexpr std::array<std::size_t, dimensionAmount> multipliers = calculate_multipliers(dimensions);
        static inline constexpr std::size_t totalSize = calculateTotalSize(dimensions);

        using Array = ArrayNd <Type, Dims...>;
        using ArrayData = std::array <Type, totalSize>;

        ArrayData m_data;

        template<typename... Indices>
        std::size_t calculateIndex(Indices... indices) const {
            static_assert(sizeof...(indices) == dimensionAmount, "Wrong number of indices");
            std::size_t index = 0;
            std::size_t coords[] = { indices... };

            for (std::size_t i = 0; i < dimensionAmount; ++i) {
                index += coords[i] * multipliers[i];
            }

            return index;
        }

        template<typename... Indices>
        bool checkBounds(Indices... indices) const {
            static_assert(sizeof...(indices) == dimensionAmount, "Wrong number of indices");
            std::size_t coords[] = { indices... };

            for (std::size_t i = 0; i < dimensionAmount; ++i) {
                if (coords[i] >= dimensions[i])
                    return 0;
            }

            return 1;
        }

    public:

        using iterator = typename ArrayData::iterator;
        using const_iterator = typename ArrayData::const_iterator;
        using reverse_iterator = typename ArrayData::reverse_iterator;
        using const_reverse_iterator = typename ArrayData::const_reverse_iterator;

        ArrayNd() = default;
        ArrayNd(const Array& other) : m_data(other.m_data) {

        };

        ArrayNd(const Array&& other) : m_data(std::move(other.m_data)) {

        };

        ArrayNd& operator=(const Array& other) {
            m_data = other.m_data;
            return *this;
        };

        ArrayNd& operator=(const Array&& other) {
            m_data = std::move(other.m_data);
            return *this;
        };

        template<typename... Indices>
        Type& operator()(Indices... indices) {
            return m_data[calculateIndex(indices)];
        };

        template<typename... Indices>
        const Type& operator()(Indices... indices) const {
            return m_data[calculateIndex(indices)];
        };

        template<typename... Indices>
        Type& at(Indices... indices) {
            if (!checkBounds(indices)) {
                throw std::out_of_range("Index out of bounds");
            }
            return m_data[calculateIndex(indices)];
        };

        template<typename... Indices>
        const Type& at(Indices... indices) const {
            if (!checkBounds(indices)) {
                throw std::out_of_range("Index out of bounds");
            }
            return m_data[calculateIndex(indices)];
        };

        ArrayData& data() const { return m_data; };

        template<std::size_t i>
        std::size_t size() const {
            static_assert(i >= dimensionAmount, "Dimension index out of bounds");
            return dimensions[i];
        };

        iterator begin() noexcept { return m_data.begin(); }
        iterator end() noexcept { return m_data.end(); }
        const_iterator begin() const noexcept { return m_data.begin(); }
        const_iterator end() const noexcept { return m_data.end(); }
        const_iterator cbegin() const noexcept { return m_data.cbegin(); }
        const_iterator cend() const noexcept { return m_data.cend(); }

        reverse_iterator rbegin() noexcept { return m_data.rbegin(); }
        reverse_iterator rend() noexcept { return m_data.rend(); }
        const_reverse_iterator rbegin() const noexcept { return m_data.rbegin(); }
        const_reverse_iterator rend() const noexcept { return m_data.rend(); }
        const_reverse_iterator crbegin() const noexcept { return m_data.crbegin(); }
        const_reverse_iterator crend() const noexcept { return m_data.crend(); }

        Type& front() { return m_data.front(); }
        const Type& front() const { return m_data.front(); }
        Type& back() { return m_data.back(); }
        const Type& back() const { return m_data.back(); }

        bool empty() const noexcept { return m_data.empty(); }
        size_t size() const noexcept { return m_data.size(); }
        size_t max_size() const noexcept { return m_data.max_size(); }

        void fill(const Type& value) { m_data.fill(value); }
        void swap(Array& other) noexcept { m_data.swap(other.m_data); }
    };
}
#endif
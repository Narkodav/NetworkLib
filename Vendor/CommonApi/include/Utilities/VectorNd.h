#pragma once
#ifndef VECTOR_ND_H
#define VECTOR_ND_H
#include "../Namespaces.h"

#include <vector>
#include <stdexcept>

namespace Utilities
{
    template<typename Type>

    class VectorNd
    {
    private:

        using Vector = VectorNd<Type>;
        using VectorData = std::vector<Type>;

        VectorData m_data;
        std::vector<size_t> m_dimensions;
        std::vector<size_t> m_multipliers;
        std::size_t m_dimensionAmount;
        std::size_t m_totalSize;

        std::vector<size_t> calculateMultipliers() {
            std::vector<size_t> mults(m_dimensions.size());
            mults[0] = 1;
            for (std::size_t i = 0; i < m_dimensionAmount - 1; ++i) {
                mults[i + 1] = mults[i] * m_dimensions[i];
            }
            return mults;
        }

        std::size_t calculateTotalSize()
        {
            std::size_t size = 1;
            for (auto dim : m_dimensions) {
                size *= dim;
            }
            return size;
        }

        template<typename... Indices>
        std::size_t calculateIndex(Indices... indices) const {
            static_assert(sizeof...(indices) == m_dimensionAmount, "Wrong number of indices");
            std::size_t index = 0;
            std::size_t coords[] = { indices... };

            for (std::size_t i = 0; i < m_dimensionAmount; ++i) {
                index += coords[i] * m_multipliers[i];
            }

            return index;
        }

        template<typename... Indices>
        bool checkBounds(Indices... indices) const {
            static_assert(sizeof...(indices) == m_dimensionAmount, "Wrong number of indices");
            std::size_t coords[] = { indices... };

            for (std::size_t i = 0; i < m_dimensionAmount; ++i) {
                if (coords[i] >= m_dimensions[i])
                    return 0;
            }

            return 1;
        }

    public:

        using iterator = typename VectorData::iterator;
        using const_iterator = typename VectorData::const_iterator;
        using reverse_iterator = typename VectorData::reverse_iterator;
        using const_reverse_iterator = typename VectorData::const_reverse_iterator;

        VectorNd() = default;

        template<typename... Sizes>
        VectorNd(Sizes... sizes) : m_dimensions({ sizes... }),
            m_dimensionAmount(sizeof...(sizes)),
            m_totalSize(calculateTotalSize()),
            m_multipliers(calculateMultipliers()),
            m_data(m_totalSize) {};

        VectorNd(const Vector& other) : m_dimensions(other.m_dimensions),
            m_dimensionAmount(other.m_dimensionAmount),
            m_multipliers(other.m_multipliers),
            m_totalSize(other.m_totalSize),
            m_data(other.m_data) {};

        VectorNd(const Vector&& other) : m_dimensions(std::move(other.m_dimensions)),
            m_dimensionAmount(std::move(other.m_dimensionAmount)),
            m_multipliers(std::move(other.m_multipliers)),
            m_totalSize(std::move(other.m_totalSize)),
            m_data(std::move(other.m_data)) {};

        VectorNd& operator=(const Vector& other) {
            m_dimensions = other.m_dimensions;
            m_dimensionAmount = other.m_dimensionAmount;
            m_multipliers = other.m_multipliers;
            m_totalSize = other.m_totalSize;
            m_data = other.m_data;
            return *this;
        };

        VectorNd& operator=(const Vector&& other) {
            m_dimensions = std::move(other.m_dimensions);
            m_dimensionAmount = std::move(other.m_dimensionAmount);
            m_multipliers = std::move(other.m_multipliers);
            m_totalSize = std::move(other.m_totalSize);
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

        VectorData& data() const { return m_data; };

        std::size_t size(size_t i) const {
            if (i >= m_dimensionAmount)
                throw std::out_of_range("Index out of bounds");
            return m_dimensions[i];
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
        void swap(Vector& other) noexcept { m_data.swap(other.m_data); }

        template<typename... Sizes>
        void resize(Sizes... sizes) {
            m_dimensions = { sizes... };
            m_dimensionAmount = sizeof...(sizes);
            m_totalSize = calculateTotalSize();
            m_multipliers = calculateMultipliers();
            m_data.resize(m_totalSize);
        };

        void clear() {
            m_dimensionAmount = 0;
            m_totalSize = 0;
            m_dimensions.clear();
            m_multipliers.clear();
            m_data.clear();
        }

        void reserve(std::size_t new_cap) { m_data.reserve(new_cap); }
        void shrink_to_fit() { m_data.shrink_to_fit(); };

        Type* data() noexcept { return m_data.data(); }
        const Type* data() const noexcept { return m_data.data(); }
    };
}
#endif


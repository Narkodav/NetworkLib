#pragma once
#ifndef VECTOR3D_H
#define VECTOR3D_H
#include "../Namespaces.h"

#include <vector>
#include <stdexcept>

namespace Utilities
{
    template<typename Type>

    class Vector3d
    {
        using Vector = Vector3d<Type>;
        using VectorData = std::vector<Type>;
    private:
        VectorData m_data;
        unsigned int m_sizeX, m_sizeY, m_sizeZ;
    public:

        using iterator = typename VectorData::iterator;
        using const_iterator = typename VectorData::const_iterator;
        using reverse_iterator = typename VectorData::reverse_iterator;
        using const_reverse_iterator = typename VectorData::const_reverse_iterator;

        Vector3d() = default;
        Vector3d(const Vector& other) : m_data(other.m_data) {
            m_sizeX = other.m_sizeX;
            m_sizeY = other.m_sizeY;
            m_sizeZ = other.m_sizeZ;
        };

        Vector3d(const Vector&& other) : m_data(std::move(other.m_data)) {
            m_sizeX = std::move(other.m_sizeX);
            m_sizeY = std::move(other.m_sizeY);
            m_sizeZ = std::move(other.m_sizeZ);
        };

        Vector3d& operator=(const Vector& other) {
            m_data = other.m_data;
            m_sizeX = other.m_sizeX;
            m_sizeY = other.m_sizeY;
            m_sizeZ = other.m_sizeZ;
            return *this;
        };

        Vector3d& operator=(const Vector&& other) noexcept {
            m_data = std::move(other.m_data);
            m_sizeX = std::move(other.m_sizeX);
            m_sizeY = std::move(other.m_sizeY);
            m_sizeZ = std::move(other.m_sizeZ);
            return *this;
        };

        Type& operator()(std::size_t x, std::size_t y, std::size_t z) {
            if (x >= m_sizeX || y >= m_sizeY || z >= m_sizeZ) {
                throw std::out_of_range("Index out of bounds");
            }
            return m_data[x * (m_sizeY * m_sizeZ) + y * m_sizeZ + z];
        };

        const Type& operator()(std::size_t x, std::size_t y, std::size_t z) const {
            if (x >= m_sizeX || y >= m_sizeY || z >= m_sizeZ) {
                throw std::out_of_range("Index out of bounds");
            }
            return m_data[x * (m_sizeY * m_sizeZ) + y * m_sizeZ + z];
        };

        Type& at(std::size_t x, std::size_t y, std::size_t z) {
            if (x >= m_sizeX || y >= m_sizeY || z >= m_sizeZ) {
                throw std::out_of_range("Index out of bounds");
            }
            return m_data[x * (m_sizeY * m_sizeZ) + y * m_sizeZ + z];
        };

        const Type& at(std::size_t x, std::size_t y, std::size_t z) const {
            if (x >= m_sizeX || y >= m_sizeY || z >= m_sizeZ) {
                throw std::out_of_range("Index out of bounds");
            }
            return m_data[x * (m_sizeY * m_sizeZ) + y * m_sizeZ + z];
        };

        void resize(std::size_t x, std::size_t y, std::size_t z) {
            m_data.resize(x * y * z);
            m_sizeX = x;
            m_sizeY = y;
            m_sizeZ = z;
        };

        std::size_t max_size() const noexcept { return m_data.max_size(); }
        void reserve(std::size_t new_cap) { m_data.reserve(new_cap); }
        void shrink_to_fit() { m_data.shrink_to_fit(); };

        Type* data() noexcept { return m_data.data(); }
        const Type* data() const noexcept { return m_data.data(); }

        std::size_t size_x() const { return m_sizeX; };
        std::size_t size_y() const { return m_sizeY; };
        std::size_t size_z() const { return m_sizeZ; };

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
        std::size_t size() const noexcept { return m_data.size(); }
        std::size_t capacity() const noexcept { return m_data.capacity(); }

        void fill(const Type& value) { m_data.fill(value); }
        void swap(Vector& other) noexcept {
            m_data.swap(other.m_data);
            std::swap(m_sizeX, other.m_sizeX);
            std::swap(m_sizeY, other.m_sizeY);
            std::swap(m_sizeZ, other.m_sizeZ);
        }

        void clear() {
            m_data.clear();
            m_sizeX = m_sizeY = m_sizeZ = 0;
        }

        void geometric_resize(std::size_t newX, std::size_t newY, std::size_t newZ) {
            if (newX == 0 || newY == 0 || newZ == 0) {
                clear();
                return;
            }

            Vector temp;
            temp.resize(newX, newY, newZ);

            std::size_t bordx = std::min(m_sizeX, newX);
            std::size_t bordy = std::min(m_sizeY, newY);
            std::size_t bordz = std::min(m_sizeZ, newZ);

            // Map old positions to new positions
            for (std::size_t x = 0; x < bordx; ++x) {
                for (std::size_t y = 0; y < bordy; ++y) {
                    for (std::size_t z = 0; z < bordz; ++z) {
                        temp(x, y, z) = this->operator()(x, y, z);
                    }
                }
            }

            *this = std::move(temp);
        }

        void geometric_resize_uniform(std::size_t newSize) {
            geometric_resize(newSize, newSize, newSize);
        }
    };
}
#endif
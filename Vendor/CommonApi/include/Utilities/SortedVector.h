#pragma once
#include "../Namespaces.h"

#include <vector>
#include <algorithm>

namespace Utilities
{
    template <typename T, typename Compare = std::less<T>>

    class SortedVector
    {
    private:
        std::vector<T> m_elements;
        Compare m_comp;

    public:

        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        explicit SortedVector(Compare comparator = Compare())
            : m_comp(std::move(comparator)) {}

        void insert(const T& element) {
            // Find the position to insert while maintaining sort order
            auto pos = std::lower_bound(m_elements.begin(), m_elements.end(), element, m_comp);
            m_elements.insert(pos, element);
        }

        void remove(const T& element) {
            auto it = std::find(m_elements.begin(), m_elements.end(), element);
            if (it != m_elements.end()) {
                m_elements.erase(it);
            }
        }

        void remove(iterator it) {
            if (it != m_elements.end()) {
                m_elements.erase(it);
            }
        }

        void resort(Compare comp) {
            m_comp = std::move(comp);
            std::sort(m_elements.begin(), m_elements.end(), m_comp);
        }

        bool empty() const { return m_elements.empty(); }
        size_t size() const { return m_elements.size(); }

        T& front() { return m_elements.front(); }
        T& back() { return m_elements.back(); }

        const T& front() const { return m_elements.front(); }
        const T& back() const { return m_elements.back(); }

        void pop_front() { m_elements.erase(m_elements.begin()); }

        // Iterator access
        iterator begin() { return m_elements.begin(); }
        iterator end() { return m_elements.end(); }
        const_iterator begin() const { return m_elements.begin(); }
        const_iterator end() const { return m_elements.end(); }
        const_iterator cbegin() const { return m_elements.cbegin(); }
        const_iterator cend() const { return m_elements.cend(); }

        const T& operator[](size_t index) const { return m_elements[index]; }
    };

}
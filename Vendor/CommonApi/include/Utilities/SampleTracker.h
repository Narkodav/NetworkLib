#pragma once
#include "../Namespaces.h"
#include "RollingWindow.h"

#include <string>

namespace Utilities
{
    template<typename T>

    class SampleTracker
    {
    private:
        static_assert(std::is_arithmetic<T>::value, "SampleTracker only works with numeric types!");

        RollingWindow<T> m_samples;
        std::string m_name;
        T m_max = std::numeric_limits<T>::min();
        T m_min = std::numeric_limits<T>::max();
        T m_sumTotal = 0;

    public:
        SampleTracker()
            : m_samples(0), m_name("") {};

        SampleTracker(const std::string& name)
            : m_samples(0), m_name(name) {};

        SampleTracker(const std::string& name, size_t maxSamples)
            : m_samples(maxSamples, 0), m_name(name) {};

        void rename(const std::string& name)
        {
            m_name = name;
        }

        void resize(size_t maxSamples)
        {
            m_samples.resize(maxSamples);
        }

        void addSample(const T& value) {
            if (m_samples.maxSize() <= m_samples.size())
                m_sumTotal -= m_samples.front();
            m_samples.add(value);
            if (m_max < value)
                m_max = value;
            else if (m_min > value)
                m_min = value;
            m_sumTotal += value;
        }

        T getAverage() const {
            if (m_samples.empty()) return 0;
            return m_sumTotal / static_cast<T>(m_samples.size());
        }

        T getTotal() const {
            return m_sumTotal;
        }

        T getCount() const {
            return m_samples.size();
        }

        T getPeak() const {
            if (m_samples.empty()) return T{};
            return m_max;
        }

        T getMin() const {
            if (m_samples.empty()) return T{};
            return m_min;
        }

        const std::string& getName() const { return m_name; }
        size_t getSampleCount() const { return m_samples.size(); }
        size_t getMaxSamples() const { return m_samples.maxSize(); }
    };
}

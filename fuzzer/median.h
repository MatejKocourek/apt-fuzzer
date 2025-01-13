#pragma once
#include <iostream>
#include <queue>
#include <vector>
#include <mutex>

template <typename T>
class StreamingMedian {
private:
    std::priority_queue<T> maxHeap; // Max-heap for the smaller half
    std::priority_queue<T, std::vector<T>, std::greater<>> minHeap; // Min-heap for the larger half

public:
    void addNumber(T num) {
        // Add to the appropriate heap
        if (maxHeap.empty() || num <= maxHeap.top()) {
            maxHeap.push(num);
        }
        else {
            minHeap.push(num);
        }

        // Balance the heaps
        if (maxHeap.size() > minHeap.size() + 1) {
            minHeap.push(maxHeap.top());
            maxHeap.pop();
        }
        else if (minHeap.size() > maxHeap.size()) {
            maxHeap.push(minHeap.top());
            minHeap.pop();
        }
    }

    double getMedian() const {
        if (maxHeap.size() == 0 && minHeap.size() == 0) [[unlikely]]
            return std::numeric_limits<double>::quiet_NaN();
        if (maxHeap.size() == minHeap.size()) {
            return (maxHeap.top() + minHeap.top()) / 2.0;
        }
        else {
            return maxHeap.top();
        }
    }
};

template<typename T>
inline void update_max(std::atomic<T>& atom, const T& val)
{
    for (T atom_val = atom; atom_val < val && !atom.compare_exchange_weak(atom_val, val););//std::memory_order_relaxed
}

template<typename T>
inline void update_min(std::atomic<T>& atom, const T& val)
{
    for (T atom_val = atom; atom_val > val && !atom.compare_exchange_weak(atom_val, val););//std::memory_order_relaxed
}

template <typename T>
class StatisticsMemory
{
    std::atomic<T> _min = std::numeric_limits<T>::max();
    std::atomic<T> _max = std::numeric_limits<T>::min();
    StreamingMedian<T> _median;
    double currentAverage = 0.0; // Running average
    std::atomic<size_t> _count = 0;
    //mutable std::mutex m;

public:
    void addNumber(T num)
    {
        size_t myCount = ++_count;

        update_max(_max, num);
        update_min(_min, num);

        //std::lock_guard lock(m);
        _median.addNumber(num);
        

        currentAverage += (num - currentAverage) / myCount; // Incremental average
    }
    T min() const
    {
        if (_count == 0) [[unlikely]]
            return 0;
        return _min;
    }
    T max() const
    {
        if (_count == 0) [[unlikely]]
            return 0;
        return _max;
    }
    double avg() const
    {
        if (_count == 0) [[unlikely]]
            return 0;
        //std::lock_guard lock(m);
        return currentAverage;
    }
    T median() const
    {
        if (_count == 0) [[unlikely]]
            return 0;
        //std::lock_guard lock(m);
        return _median.getMedian();
    }
    size_t count() const
    {
        return _count;
    }

};
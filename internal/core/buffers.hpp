#pragma once
#include <vector>
#include <array>

namespace Axiom {

template <typename T, size_t N>
class CircularBuffer {
private:
    std::array<T, N> data;
    size_t head = 0;
    size_t count = 0;
    double running_sum = 0;

public:
    void push(T val) {
        if (count == N) {
            running_sum -= data[head].to_double();
        } else {
            count++;
        }
        data[head] = val;
        running_sum += val.to_double();
        head = (head + 1) % N;
    }

    double avg() const {
        return count == 0 ? 0 : running_sum / count;
    }

    size_t size() const { return count; }
};

} // namespace Axiom

#include <cmath> 

float MoveTowards(float current, float target, float maxDelta) {
    if (std::abs(target - current) <= maxDelta) {
        return target;
    }
    return current + std::copysign(maxDelta, target - current);
}

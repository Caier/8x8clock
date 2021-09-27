template<typename T>
class ClampedInt {
    T min, max, step;

    public:
    T cur;
    ClampedInt(T cur, T min, T max, T step = 1) : cur(cur), min(min), max(max), step(step) {
        if(cur < min)
            cur = min;
        else if(cur > max)
            cur = max;
    };

    ClampedInt operator++(int) {
        if(cur + step > max)
            cur = min;
        else
            cur += step;
        return *this;
    }
};
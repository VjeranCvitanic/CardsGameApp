#pragma once

#include <iostream>

struct Points {
public:
    int punta;
    int bella;

    Points(int p = 0, int b = 0) : punta(p), bella(b) {}

    Points& operator+=(const Points& other) {
        punta += other.punta + (bella + other.bella)/3;
        bella += other.bella;
        bella %= 3;
        return *this;
    }
};

inline Points operator+(Points lhs, const Points& rhs) {
    lhs += rhs;
    return lhs;
}

inline std::ostream& operator<<(std::ostream& os, const Points& p) {
    return os << "(" << p.punta << ", " << p.bella << ")";
}

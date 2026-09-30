#pragma once
#include <string>
#include <cstdio>

struct Date {
    int y = 1970, m = 1, d = 1;

    static bool leap(int y) {
        return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    }

    static int daysInMonth(int y, int m) {
        static const int dm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        return (m == 2 && leap(y)) ? 29 : dm[m - 1];
    }

    // 0 = domingo … 6 = sábado (Sakamoto)
    static int weekday(int y, int m, int d) {
        static const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
        if (m < 3) y -= 1;
        return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    }

    bool operator==(const Date& o) const {
        return y == o.y && m == o.m && d == o.d;
    }

    std::string iso() const {
        char buf[11];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
        return buf;
    }
};
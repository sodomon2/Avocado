#pragma once
#include <string>

namespace disc {
struct Position {
    int mm;
    int ss;
    int ff;

    Position();
    Position(int mm, int ss, int ff);
    static Position fromLba(size_t lba);

    std::string toString() const;
    int toLba() const;
    Position operator+(const Position& p) const;
    Position operator-(const Position& p) const;
    bool operator==(const Position& p) const;
    bool operator>=(const Position& p) const;
    bool operator<(const Position& p) const;
};
}  // namespace utils

#pragma once
#include <iostream>


class Timestamp
{
public:
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    Timestamp();
    static Timestamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;

};


#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <QDateTime>
#include <atomic>

struct Count
{
    std::atomic<long long> countRead{0};
    std::atomic<long long> bytesRead{0};
    QDateTime t;
};

#endif

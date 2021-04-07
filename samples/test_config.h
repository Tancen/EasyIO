#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <QDateTime>
#include <atomic>
#define TEST_DEFAULT_DATA_SIZ_TCP       10240
#define TEST_DEFAULT_DATA_SIZ_UDP       1024

struct Count
{
    std::atomic<long long> countRead{0};
    std::atomic<long long> countWrite{0};
    std::atomic<long long> bytesRead{0};
    std::atomic<long long> bytesWrite{0};
    QDateTime t;
};

#endif

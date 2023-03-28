#ifndef MCSPLITDAL_STATS_H
#define MCSPLITDAL_STATS_H

#include <chrono>

typedef struct Stats{
    unsigned long long nodes{0};
    unsigned long long cutbranches{0};
    unsigned long long conflicts = 0;
    clock_t bestfind;
    unsigned long long bestnodes = 0, bestcount = 0;
    int dl = 0;
    clock_t start;
} Stats;

#endif //MCSPLITDAL_STATS_H

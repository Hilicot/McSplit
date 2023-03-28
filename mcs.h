#ifndef MCSPLIT_MCS_H
#define MCSPLIT_MCS_H
#include <vector>
#include "Graph.h"
#include "args.h"
#include "stats.h"

using namespace std;

using gtype = double;


struct VtxPair
{
    int v;
    int w;
    VtxPair(int v, int w) : v(v), w(w) {}
};

struct Bidomain
{
    int l, r; // start indices of left and right sets
    int left_len, right_len;
    bool is_adjacent;
    Bidomain(int l, int r, int left_len, int right_len, bool is_adjacent) : l(l),
                                                                            r(r),
                                                                            left_len(left_len),
                                                                            right_len(right_len),
                                                                            is_adjacent(is_adjacent){};
};

vector<VtxPair> mcs(const Graph &g0, const Graph &g1, Stats *stats);

#endif
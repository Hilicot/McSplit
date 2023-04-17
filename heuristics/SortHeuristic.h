#ifndef MCSPLITDAL_SORTHEURISTIC_H
#define MCSPLITDAL_SORTHEURISTIC_H

#include <vector>
#include <iostream>
#include "../graph.h"

using namespace std;

namespace SortHeuristic {
    class Base {
    protected:
        int num_threads = 1;
    public:
        string name = "Base";
        virtual vector<int> sort(const Graph &g){std::cout << "Warning: no sort is selected!" << std::endl;}; 
        void set_num_threads(int num_threads) { this->num_threads = num_threads; }
    };

    class Degree : public Base {
    public:
        string name = "Degree";
        vector<int> sort(const Graph &g) override;
    };

    class PageRank : public Base {
    public:
        string name = "PageRank";
        vector<int> sort(const Graph &g) override;
    };

    class BetweennessCentrality : public Base {
    public:
        string name = "BetweennessCentrality";
        vector<int> sort(const Graph &g) override;
    };

}
#endif //MCSPLITDAL_SORTHEURISTIC_H

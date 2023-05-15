#ifndef MCSPLITDAL_SAVE_SEARCH_DATA_H
#define MCSPLITDAL_SAVE_SEARCH_DATA_H

#include "../mcs.h"


struct SearchData {
    int before_size;
    vector<int> left_bidomain;
    vector<int> right_bidomain;

    SearchData(): before_size(0), left_bidomain(vector<int>()), right_bidomain(vector<int>()) {};

    SearchData(int before_size, const Bidomain &bidomain, const vector<int> &left, const vector<int> &right);
};


void save_vertex_v();

#endif //MCSPLITDAL_SAVE_SEARCH_DATA_H

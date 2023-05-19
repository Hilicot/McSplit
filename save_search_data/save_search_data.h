#ifndef MCSPLITDAL_SAVE_SEARCH_DATA_H
#define MCSPLITDAL_SAVE_SEARCH_DATA_H

#include <boost/archive/binary_oarchive.hpp>
#include "../mcs.h"


class SearchData {
public:
    enum search_data_type {
        V, W
    };
    search_data_type type;
    vector<int> left_bidomain;
    vector<int> vertex_scores;

    SearchData() : left_bidomain(vector<int>()), vertex_scores(vector<int>()), type(V) {};

    SearchData(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right, search_data_type type=V);

    void record_score(int vtx, int score);

    virtual void save();

protected:
    int record_score_backend(int vtx, int score, const vector<int> &indices);

    virtual void serialize(ofstream &out);
};

class SearchDataW : public SearchData {
public:
    int v;
    vector<int> right_bidomain;
    vector<int> bounds;

    SearchDataW() : SearchData(), v(-1) {};

    SearchDataW(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right, int v, search_data_type type=W);

    void record_score(int vtx, int score, int bound);

    void save() override;

    void serialize(ofstream &out) override;
};

void save_graph_mappings(const vector<int> &_g0_mapping, const vector<int> &_g1_mapping, const string& g0_name, const string& g1_name);

void save_solution(vector<VtxPair> &solution);

void close_streams();

#endif //MCSPLITDAL_SAVE_SEARCH_DATA_H

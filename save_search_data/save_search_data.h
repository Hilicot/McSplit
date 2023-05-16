#ifndef MCSPLITDAL_SAVE_SEARCH_DATA_H
#define MCSPLITDAL_SAVE_SEARCH_DATA_H

#include "../mcs.h"


class SearchData {
public:
    enum search_data_type {
        V, W
    };
    search_data_type type;
    vector<int> left_bidomain;
    vector<int> right_bidomain;
    vector<int> vertex_scores;

    SearchData() : left_bidomain(vector<int>()), right_bidomain(vector<int>()), vertex_scores(vector<int>()) {};

    SearchData(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right, search_data_type type=V);

    virtual void record_score(int vtx, int score);

    void save();

protected:
    void record_score_backend(int vtx, int score, const vector<int> &indices);
};

class SearchDataW : public SearchData {
    int v;
    search_data_type type;
public:
    SearchDataW() : SearchData(), v(-1) {};

    SearchDataW(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right, int v, search_data_type type=W);

    void record_score(int vtx, int score) override;
};


#endif //MCSPLITDAL_SAVE_SEARCH_DATA_H

#include "save_search_data.h"
#include <algorithm>

SearchData::SearchData(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right, search_data_type type) {
    this->left_bidomain = vector<int>(bidomain.left_len, 0);
    this->vertex_scores = vector<int>(bidomain.left_len, -1);
    this->type = type;

    for (int i = 0; i < bidomain.left_len; ++i)
        this->left_bidomain[i] = left[i + bidomain.l];
}

void SearchData::record_score_backend(int vtx, int score, const vector<int> &indices) {
    auto it = std::find(indices.begin(), indices.end(), vtx);
    if (it != indices.end()) {
        // Calculate the index of the vertex in the vector
        int index = std::distance(indices.begin(), it);

        // Update the score of the element in vector B
        vertex_scores[index] = score;
    } else {
        cout << "Error in type=" << type << ": vertex " << vtx << " not found in bidomain (";
        for (auto &i: (type == V? left_bidomain : right_bidomain))
            cout << i << " ";
        cout << ")" << endl;
        exit(1);
    }
}

void SearchData::record_score(int vtx, int score) {
    record_score_backend(vtx, score, left_bidomain);
}

void SearchData::save() {
    // TODO: save to file. ATTENZIONE: ricordarsi che questi vertici sono rinominati da g_sorted
    // TODO save v data and w data (SearchDataW) in different formats and different files
    // for (auto &i: vertex_scores)
    //     cout << i << " ";
    // cout << endl;
}

SearchDataW::SearchDataW(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right, int v, search_data_type type)
        : SearchData(bidomain, left, right, type) {
    this->v = v;
    this->vertex_scores.resize(bidomain.right_len, -1);
    this->right_bidomain = vector<int>(bidomain.right_len, 0);
    for (int i = 0; i < bidomain.right_len; ++i)
        this->right_bidomain[i] = right[i + bidomain.r];

}

void SearchDataW::record_score(int vtx, int score) {
    record_score_backend(vtx, score, right_bidomain);
}
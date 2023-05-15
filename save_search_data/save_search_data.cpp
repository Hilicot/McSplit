#include "save_search_data.h"

SearchData::SearchData(int before_size, const Bidomain &bidomain, const vector<int> &left, const vector<int> &right){
    this->before_size = before_size;
    this->left_bidomain = vector<int>(bidomain.left_len);
    this->right_bidomain = vector<int>(bidomain.right_len);
    for (int i = 0; i < left.size(); ++i) {
        this->left_bidomain[i] = left[i+bidomain.l];
    }
    for (int i = 0; i < right.size(); ++i) {
        this->right_bidomain[i] = right[i+bidomain.r];
    }
}

void save_vertex_v(){

}
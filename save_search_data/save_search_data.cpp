#include "save_search_data.h"
#include <algorithm>
#include <fstream>
#include <filesystem>

string filepath;
vector<int> g0_mapping, g1_mapping;
ofstream v_file, w_file;
int count_v = 0, count_w = 0;

SearchData::SearchData(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right,
                       search_data_type type) {
    this->left_bidomain = vector<int>(bidomain.left_len, 0);
    this->vertex_scores = vector<int>(bidomain.left_len, -1);
    this->type = type;

    for (int i = 0; i < bidomain.left_len; ++i)
        this->left_bidomain[i] = left[i + bidomain.l];
}

SearchDataW::SearchDataW(const Bidomain &bidomain, const vector<int> &left, const vector<int> &right, int v,
                         search_data_type type)
        : SearchData(bidomain, left, right, type) {
    this->v = v;
    this->vertex_scores.resize(bidomain.right_len, -1);
    this->right_bidomain = vector<int>(bidomain.right_len, 0);
    this->bounds = vector<int>(bidomain.right_len, -1);
    for (int i = 0; i < bidomain.right_len; ++i)
        this->right_bidomain[i] = right[i + bidomain.r];

}

int SearchData::record_score_backend(int vtx, int score, const vector<int> &indices) {
    int index = -1;
    auto it = std::find(indices.begin(), indices.end(), vtx);
    if (it != indices.end()) {
        // Calculate the index of the vertex in the vector
        index = std::distance(indices.begin(), it);

        // Update the score of the element in vector B
        vertex_scores[index] = score;
    } else {
        cout << "Error in type=" << type << ": vertex " << vtx << " not found in bidomain";
        exit(1);
    }
    return index;
}

void SearchData::record_score(int vtx, int score) {
    record_score_backend(vtx, score, left_bidomain);
}

void SearchDataW::record_score(int vtx, int score, int bound) {
    int index = record_score_backend(vtx, score, right_bidomain);
    this->bounds[index] = bound;
}

/**
 * Remap the vertex vector according to the mapping, and destroy the vector in the process (for efficiency)
 */
vector<int> &&remap_vertex_vector(vector<int> &v, const vector<int> &mapping) {
    for (int &i: v)
        i = mapping[i];
    return std::move(v);
}


void SearchData::serialize(ofstream &out) {
    int num_v = (int) left_bidomain.size();
    out.write(reinterpret_cast<char *>(&num_v), sizeof(int));
    int num_s = (int) vertex_scores.size();
    out.write(reinterpret_cast<char *>(&num_s), sizeof(int));
    out.write(reinterpret_cast<char *>(remap_vertex_vector(this->left_bidomain, g0_mapping).data()),
              num_v * (int) sizeof(int));
    out.write(reinterpret_cast<char *>(vertex_scores.data()), num_s * (int) sizeof(int));
}

void SearchDataW::serialize(ofstream &out) {
    SearchData::serialize(out);
    int _v = g0_mapping[v];
    out.write(reinterpret_cast<char *>(&_v), sizeof(int));
    int num_w = (int) right_bidomain.size();
    assert (num_w == (int) vertex_scores.size());
    out.write(reinterpret_cast<char *>(remap_vertex_vector(this->right_bidomain, g1_mapping).data()),
              num_w * (int) sizeof(int));
    out.write(reinterpret_cast<char *>(bounds.data()), num_w * (int) sizeof(int));
}


void SearchData::save() {
    serialize(v_file);
    // ATTENTION: here the bidomain vector is already destroyed! (so don't try to print it)
    count_v++;
}

void SearchDataW::save() {
    serialize(w_file);
    // ATTENTION: here the bidomain vector is already destroyed! (so don't try to print it)
    count_w++;
}


string get_basename(string path) {
    size_t lastindex = path.find_last_of('.');
    path = path.substr(0, lastindex);
    lastindex = path.find_last_of('/');
    return path.substr(lastindex + 1, path.size());
}

void save_graph_mappings(const vector<int> &_g0_mapping, const vector<int> &_g1_mapping, const string& g0_name, const string& g1_name, const Graph &g0, const Graph &g1, const string &save_search_data_folder) {
    filepath = save_search_data_folder+"/";
    string g0_basename = get_basename(g0_name);
    string g1_basename = get_basename(g1_name);
    string folder;
    if (g0_basename == "g1" || g1_basename == "g1") {
        // synthetic pair: need to consider the name of the folder instead
        size_t lastindex = g0_name.find_last_of('/');
        string folder_path = g0_name.substr(0, lastindex);
        folder = get_basename(folder_path);
    } else {
        // small pair: need to consider the name of the file
        g0_basename = get_basename(g0_name);
        g1_basename = get_basename(g1_name);
        folder = g0_basename + "-" + g1_basename;
    }
    filepath = filepath + folder;
    //create folder
    std::filesystem::create_directories(filepath);
    filepath = filepath + "/";

    // open file streams
    v_file = ofstream(filepath + "v_data", ios::binary | ios::out);
    w_file = ofstream(filepath + "w_data", ios::binary | ios::out);

    //save mappings
    g0_mapping = ref(_g0_mapping);
    g1_mapping = ref(_g1_mapping);

    // copy graph files to folder if they are not already there
    if (!std::filesystem::exists(filepath + "g0")){
        if (arguments.ascii)
            std::filesystem::copy(g0_name, filepath + "g0");
        else
            g0.export_to_ascii(filepath + "g0");
    }
    if (!std::filesystem::exists(filepath + "g1")){
        if (arguments.ascii)
            std::filesystem::copy(g1_name, filepath + "g1");
        else
            g1.export_to_ascii(filepath + "g1");
    }
}

void save_solution(vector<VtxPair> &solution){
    ofstream solution_file(filepath + "solution", ios::binary | ios::out);
    int num_v = (int) solution.size();
    solution_file.write(reinterpret_cast<char *>(&num_v), sizeof(int));
    for (VtxPair &p: solution) {
        int _v = p.v;
        int _w = p.w;
        solution_file.write(reinterpret_cast<char *>(&_v), sizeof(int));
        solution_file.write(reinterpret_cast<char *>(&_w), sizeof(int));
    }
    solution_file.close();
}

void close_streams() {
    v_file.close();
    w_file.close();
    cout << "Saved " << count_v << " v data and " << count_w << " w data" << endl;
}
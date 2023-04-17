#ifndef MCSPLITDAL_BRANDES_H
#define MCSPLITDAL_BRANDES_H


#include <atomic>
#include <mutex>
#include <vector>

#include "../graph.h"

class Brandes {

public:
    using fType = double;

    Brandes(const Graph& graph): graph_(graph){
        BC_.resize(graph_.n);
        std::fill(begin(BC_), end(BC_), 0.0);
    }

    void run(size_t thread_num);
    std::vector<int> get_result_vector() const;

private:
    void process(const size_t &vertex_id, std::vector<fType>* BC_local);
    void run_worker(std::atomic<size_t> *idx);

    std::mutex bc_mutex_;
    const Graph &graph_;
    std::vector<fType> BC_;

};

#endif //MCSPLITDAL_BRANDES_H

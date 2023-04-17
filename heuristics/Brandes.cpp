#include "brandes.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <stack>
#include <thread>

void Brandes::process(const size_t& vertex_id, std::vector<fType>* BC_local){
    size_t vec_size = BC_local->size();
    std::stack<size_t> S;
    std::vector<std::vector<size_t> > P(vec_size);
    std::vector<int> sigma(vec_size);
    std::vector<int> d(vec_size);
    std::vector<fType> delta(vec_size);

    for (size_t w = 0; w < graph_.n; w++) {
        sigma[w] = 0;
        d[w] = -1;
        delta[w] = 0;
    }

    sigma[vertex_id] = 1;
    d[vertex_id] = 0;

    std::queue<size_t> Q;
    Q.push(vertex_id);

    while (!Q.empty()) {
        size_t v = Q.front();
        Q.pop();
        S.push(v);

        for (const auto& node : graph_.adjlist[v].adjNodes) {
            size_t w = node.id;
            if (d[w] < 0) {
                Q.push(w);
                d[w] = d[v] + 1;
            }

            if (d[w] == d[v] + 1) {
                sigma[w] += sigma[v];
                P[w].emplace_back(v);
            }
        }
    }

    while (!S.empty()) {
        size_t v = S.top();
        S.pop();

        for (size_t p : P[v]) {
            double result = (fType(sigma[p]) / sigma[v]) * (1.0 + delta[v]);
            delta[p] += result;
        }

        if (v != vertex_id) {
            (*BC_local)[v] += delta[v];
        }
    }
}

void Brandes::run_worker(std::atomic<size_t>* idx){
    std::vector<fType> BC_local(graph_.n);
    std::fill(begin(BC_local), end(BC_local), 0.0);

    while (true) {
        int my_index = (*idx)--;

        if (my_index < 0)
            break;

        process(my_index, &BC_local);
    }

    // Synchronized section
    {
        std::lock_guard<std::mutex> guard(bc_mutex_);
        for (size_t i = 0; i < BC_local.size(); i++) {
            BC_[i] += BC_local[i];
        }
    }
}

void Brandes::run(const size_t thread_num){
    std::vector<std::thread> threads;
    std::atomic<size_t> index;

    index.store(graph_.n - 1);

    // run threads
    for (size_t i = 0; i < thread_num - 1; i++)
        threads.emplace_back(std::thread([this, &index] { run_worker(&index); }));

    // start working
    run_worker(&index);

    // wait for others to finish
    for (auto& thread : threads)
        thread.join();
}

std::vector<int> Brandes::get_result_vector() const{
    std::vector<int> results;
    results.resize(BC_.size());

    for (size_t i = 0; i < BC_.size(); i++) {
        results[i] = static_cast<int>(BC_[i]*100);
    }
    return results;
}
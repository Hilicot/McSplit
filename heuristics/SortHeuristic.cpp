#include "SortHeuristic.h"
#include <algorithm>
#include "Brandes.h"

#define VERBOSE false

namespace SortHeuristic {
    vector<int>  Degree::sort(const Graph &g) {
        if(VERBOSE) std::cout << "Sorting by degree" << std::endl;
        vector<int> degree(g.n, 0);
        for (int v = 0; v < g.n; v++) {
            degree[v] = g.adjlist[v].adjNodes.size() - 1;
        }
        return degree;
    }

    vector<int> PageRank::sort(const Graph &g) {
        if(VERBOSE) std::cout << "Sorting by PageRank" << std::endl;
        constexpr float damping_factor = 0.85f;
        constexpr float epsilon = 0.00001f;
        std::vector<int> out_links = std::vector(g.n, 0);
        for (int i = 0; i < g.n; i++) {
            out_links[i] = g.adjlist[i].adjNodes.size();
        }
        // create a stochastic matrix (inefficient for big/sparse graphs, could be transformed into adj list (or just use the Graph's internal adj_list?))
        std::vector<std::vector<float>> stochastic_g = std::vector(g.n, std::vector(g.n, 0.0f));
        for (int i = 0; i < g.n; i++) {
            if (!out_links[i]) {
                for (int j = 0; j < g.n; j++) {
                    stochastic_g[i][j] = 1.0f / (float) g.n;
                }
            } else {
                for (auto &w: g.adjlist[i].adjNodes) {
                    stochastic_g[i][w.id] = 1.0f / (float) out_links[i];
                }
            }
        }
        std::vector<int> result(g.n, 0);
        std::vector<float> ranks(g.n, 0);
        std::vector<float> p(g.n, 1.0 / g.n);
        std::vector<std::vector<float>> transposed = std::vector(g.n, std::vector(g.n, 0.0f));
        // transpose matrix
        for (int i = 0; i < g.n; i++) {
            for (int j = 0; j < g.n; j++) {
                transposed[i][j] = stochastic_g[j][i];
            }
        }
        while (true) {
            std::fill(ranks.begin(), ranks.end(), 0);
            for (int i = 0; i < g.n; i++) {
                for (int j = 0; j < g.n; j++) {
                    ranks[i] = ranks[i] + transposed[i][j] * p[j];
                }
            }
            for (int i = 0; i < g.n; i++) {
                ranks[i] = damping_factor * ranks[i] + (1.0 - damping_factor) / (float) g.n;
            }
            float error = 0.0f;
            for (int i = 0; i < g.n; i++) {
                error += std::abs(ranks[i] - p[i]);
            }
            if (error < epsilon) {
                break;
            }

            for (int i = 0; i < g.n; i++) {
                p[i] = ranks[i];
            }
        }
        for (int i = 0; i < ranks.size(); i++) {
            result[i] = ranks[i] / epsilon;
        }
        return result;
    }

    // taken from https://github.com/chivay/betweenness-centrality
    vector<int> BetweennessCentrality::sort(const Graph &g){
        if(VERBOSE) std::cout << "Sorting by BetweennessCentrality" << std::endl;

        Brandes brandes(g);
        brandes.run(num_threads);
        vector<int> betweenness = brandes.get_result_vector();
        return betweenness;
    }
}


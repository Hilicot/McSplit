#ifndef GRAPH_H
#define GRAPH_H

#include <limits.h>
#include <stdbool.h>
#include <vector>
#include <string>

struct Node {
    unsigned int id;
    unsigned int original_id;
    unsigned int label;
    std::vector<Node> adjNodes;

    Node(unsigned int id, unsigned int label);
};

struct Graph {
    int n, e;
    std::vector<Node> adjlist;
    std::vector<std::vector<std::pair<std::pair<unsigned int, unsigned int>, std::vector<int>>>> leaves;

    Graph(unsigned int n);

    unsigned int get(const int u, const int v) const;

    void pack_leaves();

    int computeNumEdges();

    float computeDensity();

    void export_to_ascii(std::string filename) const;

    Graph subgraph(const std::vector<int> &arr, int start, int len) const;
};

Graph induced_subgraph(struct Graph &g, std::vector<int> vv);

Graph readGraph(char *filename, char format, bool directed, bool edge_labelled, bool vertex_labelled);

void add_edge(Graph &g, int v, int w, bool directed, unsigned int val);

#endif

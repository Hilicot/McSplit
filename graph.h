#include <limits.h>
#include <stdbool.h>

#include <vector>

struct Node {
    unsigned int id;
    unsigned int original_id;
    unsigned int label;
    std::vector<Node> adjNodes;
    Node(unsigned int id, unsigned int label);
};

struct Graph {
    int n;
    std::vector<Node> adjlist;
    Graph(unsigned int n);
    unsigned int get(const int u, const int v) const;
};

Graph induced_subgraph(struct Graph &g, std::vector<int> vv);
Graph readGraph(char* filename, char format, bool directed, bool edge_labelled, bool vertex_labelled);


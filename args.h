#ifndef MCSPLITDAL_ARGS_H
#define MCSPLITDAL_ARGS_H

enum SwapPolicy {NO_SWAP, McSPLIT_SD, McSPLIT_SO, ADAPTIVE} ;

enum Heuristic
{
    min_max,
    min_product
};

static struct arguments
{
    bool quiet;
    bool verbose;
    bool dimacs;
    bool lad;
    bool ascii;
    bool connected;
    bool directed;
    bool edge_labelled;
    bool vertex_labelled;
    bool big_first;
    Heuristic heuristic;
    char *filename1;
    char *filename2;
    int timeout;
    int arg_num;
    SwapPolicy swap_policy;
} arguments;



#endif //MCSPLITDAL_ARGS_H

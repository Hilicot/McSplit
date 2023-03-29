#include "graph.h"

#include <algorithm>
#include <numeric>
#include <iostream>
#include <set>
#include <string>
#include <mutex>
#include <condition_variable>
#include <argp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include "stats.h"
#include "args.h"
#include "mcs.h"

using namespace std;

#define VSCORE
// #define DEBUG
#define Best
using std::cout;
using std::endl;
using std::vector;

static void fail(std::string msg) {
    std::cerr << msg << std::endl;
    exit(1);
}


/*******************************************************************************
                             Command-line arguments
*******************************************************************************/

static char doc[] = "Find a maximum clique in a graph in DIMACS format\vHEURISTIC can be min_max or min_product";
static char args_doc[] = "HEURISTIC FILENAME1 FILENAME2";
static struct argp_option options[] = {
        {"quiet",                'q', 0,         0, "Quiet output"},
        {"verbose",              'v', 0,         0, "Verbose output"},
        {"dimacs",               'd', 0,         0, "Read DIMACS format"},
        {"lad",                  'l', 0,         0, "Read LAD format"},
        {"ascii",                'A', 0,         0, "Read ASCII format"},
        {"connected",            'c', 0,         0, "Solve max common CONNECTED subgraph problem"},
        {"directed",             'i', 0,         0, "Use directed graphs"},
        {"labelled",             'a', 0,         0, "Use edge and vertex labels"},
        {"vertex-labelled-only", 'x', 0,         0, "Use vertex labels, but not edge labels"},
        {"big-first",            'b', 0,         0, "First try to find an induced subgraph isomorphism, then decrement the target size"},
        {"timeout",              't', "timeout", 0, "Specify a timeout (seconds)"},
        {0}};

void set_default_arguments() {
    arguments.quiet = false;
    arguments.verbose = false;
    arguments.dimacs = false;
    arguments.lad = false;
    arguments.ascii = false;
    arguments.connected = false;
    arguments.directed = false;
    arguments.edge_labelled = false;
    arguments.vertex_labelled = false;
    arguments.big_first = false;
    arguments.filename1 = NULL;
    arguments.filename2 = NULL;
    arguments.timeout = 0;
    arguments.arg_num = 0;
    arguments.swap_policy = McSPLIT_SO;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    switch (key) {
        case 'd':
            if (arguments.lad)
                fail("The -d and -l options cannot be used together.\n");
            arguments.dimacs = true;
            break;
        case 'l':
            if (arguments.dimacs)
                fail("The -d and -l options cannot be used together.\n");
            arguments.lad = true;
            break;
        case 'A':
            if (arguments.dimacs || arguments.lad)
                fail("The -d or -l options cannot be used together with -as.\n");
            arguments.ascii = true;
        case 'q':
            arguments.quiet = true;
            break;
        case 'v':
            arguments.verbose = true;
            break;
        case 'c':
            if (arguments.directed)
                fail("The connected and directed options can't be used together.");
            arguments.connected = true;
            break;
        case 'i':
            if (arguments.connected)
                fail("The connected and directed options can't be used together.");
            arguments.directed = true;
            break;
        case 'a':
            if (arguments.vertex_labelled)
                fail("The -a and -x options can't be used together.");
            arguments.edge_labelled = true;
            arguments.vertex_labelled = true;
            break;
        case 'x':
            if (arguments.edge_labelled)
                fail("The -a and -x options can't be used together.");
            arguments.vertex_labelled = true;
            break;
        case 'b':
            arguments.big_first = true;
            break;
        case 't':
            arguments.timeout = std::stoi(arg);
            break;
        case ARGP_KEY_ARG:
            if (arguments.arg_num == 0) {
                if (std::string(arg) == "min_max")
                    arguments.heuristic = min_max;
                else if (std::string(arg) == "min_product")
                    arguments.heuristic = min_product;
                else
                    fail("Unknown heuristic (try min_max or min_product)");
            } else if (arguments.arg_num == 1) {
                arguments.filename1 = arg;
            } else if (arguments.arg_num == 2) {
                arguments.filename2 = arg;
            } else {
                argp_usage(state);
            }
            arguments.arg_num++;
            break;
        case ARGP_KEY_END:
            if (arguments.arg_num == 0)
                argp_usage(state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

/*******************************************************************************
                                 Main
*******************************************************************************/

bool check_sol(const Graph &g0, const Graph &g1, const vector<VtxPair> &solution) {
    return true;
    vector<bool> used_left(g0.n, false);
    vector<bool> used_right(g1.n, false);
    for (unsigned int i = 0; i < solution.size(); i++) {
        struct VtxPair p0 = solution[i];
        if (used_left[p0.v] || used_right[p0.w])
            return false;
        used_left[p0.v] = true;
        used_right[p0.w] = true;
        if (g0.adjlist[p0.v].label != g1.adjlist[p0.w].label)
            return false;
        for (unsigned int j = i + 1; j < solution.size(); j++) {
            struct VtxPair p1 = solution[j];
            if (g0.get(p0.v, p1.v) != g1.get(p0.w, p1.w))
                return false;
        }
    }
    return true;
}

vector<int> calculate_degrees(const Graph &g) {
    vector<int> degree(g.n, 0);
    for (int v = 0; v < g.n; v++) {
        degree[v] = g.adjlist[v].adjNodes.size() - 1;
    }
    return degree;
}

int sum(const vector<int> &vec) {
    return std::accumulate(std::begin(vec), std::end(vec), 0);
}

/**
 * based on arguments.swap_policy, return true if the graphs needs to be swapped.
 * McSPLIT_SD and McSPLIT_SO are based on Trimble's PHD thesis https://theses.gla.ac.uk/83350/
 */
bool swap_graphs(Graph g0, Graph g1) {
    switch (arguments.swap_policy) {
        case McSPLIT_SD: { // swap if density extremeness of g1 is bigger than that of g0
            // get densities
            float d0 = g0.computeDensity();
            float d1 = g1.computeDensity();
            // compute density extremeness
            double de0 = abs(0.5 - d0);
            double de1 = abs(0.5 - d1);
            return de1 > de0;
        }
        case McSPLIT_SO:
            return g1.n > g0.n;
        default:
            cerr << "swap policy unknown" << endl;
        case NO_SWAP:
            return false;
    }
}

int main(int argc, char **argv) {
    set_default_arguments();
    argp_parse(&argp, argc, argv, 0, 0, 0);

    char format = arguments.dimacs ? 'D' : arguments.lad ? 'L'
                                                         : arguments.ascii ? 'A'
                                                                           : 'B';
    struct Graph g0 = readGraph(arguments.filename1, format, arguments.directed,
                                arguments.edge_labelled, arguments.vertex_labelled);
    struct Graph g1 = readGraph(arguments.filename2, format, arguments.directed,
                                arguments.edge_labelled, arguments.vertex_labelled);

    Stats stats_s;
    Stats *stats = &stats_s;
    std::thread timeout_thread;
    std::mutex timeout_mutex;
    std::condition_variable timeout_cv;
    stats->abort_due_to_timeout.store(false);

    bool aborted = false;
#if 1
    if (0 != arguments.timeout) {
        timeout_thread = std::thread([&] {
            auto abort_time = std::chrono::steady_clock::now() + std::chrono::seconds(arguments.timeout);
            {
                /* Sleep until either we've reached the time limit,
                 * or we've finished all the work. */
                std::unique_lock<std::mutex> guard(timeout_mutex);
                while (!stats->abort_due_to_timeout.load()) {
                    if (std::cv_status::timeout == timeout_cv.wait_until(guard, abort_time)) {
                        /* We've woken up, and it's due to a timeout. */

                        aborted = true;
                        break;
                    }
                }
            }
            stats->abort_due_to_timeout.store(true);
        });
    }
#endif

    // decide wheter to swap the graphs based on swap_policy
    if (swap_graphs(g0, g1)) {
        swap(g0, g1);
        stats->swapped_graphs = true;
    }

    //  auto start = std::chrono::steady_clock::now();
    stats->start = clock();

    vector<int> g0_deg = calculate_degrees(g0);
    vector<int> g1_deg = calculate_degrees(g1);

    // As implemented here, g1_dense and g0_dense are false for all instances
    // in the Experimental Evaluation section of the paper.  Thus,
    // we always sort the vertices in descending order of degree (or total degree,
    // in the case of directed graphs.  Improvements could be made here: it would
    // be nice if the program explored exactly the same search tree if both
    // input graphs were complemented.
    //  #ifdef VSCORE
    //    vector<int> lgrade(g0.n,0);
    //    vector<int> rgrade(g1.n,0);
    //  #endif
    vector<int> vv0(g0.n);
    std::iota(std::begin(vv0), std::end(vv0), 0);
    bool g1_dense = sum(g1_deg) > g1.n * (g1.n - 1);
    std::stable_sort(std::begin(vv0), std::end(vv0),
                     [&](int a, int b) { return g1_dense ? (g0_deg[a] < g0_deg[b]) : (g0_deg[a] > g0_deg[b]); });

    vector<int> vv1(g1.n);
    std::iota(std::begin(vv1), std::end(vv1), 0);
    bool g0_dense = sum(g0_deg) > g0.n * (g0.n - 1);
    std::stable_sort(std::begin(vv1), std::end(vv1),
                     [&](int a, int b) { //????????????????????????????????????????????????????
                         return g0_dense ? (g1_deg[a] < g1_deg[b]) : (g1_deg[a] > g1_deg[b]);
                     });
    std::cout << "Sorting done" << std::endl;

#if 0
    int idx;
    for(idx=0;idx<vv0.size();idx++)
       cout<<vv0[idx]<<"  ";
    cout<<endl;
    for(idx=0;idx<g0_deg.size();idx++)
       cout<<g0_deg[idx]<<"  ";
    cout<<endl;
    for(idx=0;idx<vv1.size();idx++)
       cout<<vv1[idx]<<"  ";
    cout<<endl;
    for(idx=0;idx<g1_deg.size();idx++)
       cout<<"("<<idx<<","<<g1_deg[idx]<<")  ";
    cout<<endl;

#endif
    struct Graph g0_sorted = induced_subgraph(g0, vv0);
    struct Graph g1_sorted = induced_subgraph(g1, vv1);
    clock_t time_elapsed = clock() - stats->start;
    std::cout << "Induced subgraph calculated in " << time_elapsed * 1000 / CLOCKS_PER_SEC << "ms" << endl;
#if 0
    int idx;
    for(idx=0;idx<g0.n;idx++)
        cout<<g0.label[idx]<<" ";
    cout<<endl;
    for(idx=0;idx<g0_sorted.n;idx++)
        cout<<g0_sorted.label[idx]<<" ";
    cout<<endl;
#endif

    g0_sorted.pack_leaves();
    g1_sorted.pack_leaves();

    stats->start = clock();

    vector<VtxPair> solution = mcs(g0_sorted, g1_sorted, stats);

    // Convert to indices from original, unsorted graphs
    for (auto &vtx_pair : solution) {
        vtx_pair.v = vv0[vtx_pair.v];
        vtx_pair.w = vv1[vtx_pair.w];
    }

    // auto stop = std::chrono::steady_clock::now();
    // auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    time_elapsed = clock() - stats->start;
    clock_t time_find = stats->bestfind - stats->start;
    /* Clean up the timeout thread */
#if 1
    if (timeout_thread.joinable()) {
        {
            std::unique_lock<std::mutex> guard(timeout_mutex);
            stats->abort_due_to_timeout.store(true);
            timeout_cv.notify_all();
        }
        timeout_thread.join();
    }
#endif
    if (!check_sol(g0, g1, solution))
        fail("*** Error: Invalid solution\n");

    cout << "Solution size " << solution.size() << std::endl;
    for (int i = 0; i < g0.n; i++)
        for (unsigned int j = 0; j < solution.size(); j++)
            if (solution[j].v == i)
                cout << "(" << solution[j].v << " -> " << solution[j].w << ") ";
    cout << std::endl;

    cout << "Nodes:                      " << stats->nodes << endl;
    cout << "Cut branches:               " << stats->cutbranches << endl;
    cout << "Conflicts:                    " << stats->conflicts << endl;
    printf("CPU time (ms):              %15ld\n", time_elapsed * 1000 / CLOCKS_PER_SEC);
    printf("FindBest time (ms):              %15ld\n", time_find * 1000 / CLOCKS_PER_SEC);
#ifdef Best
    cout << "Best nodes:                 " << stats->bestnodes << endl;
    cout << "Best count:                 " << stats->bestcount << endl;
    cout << "Swapped:                    " << stats->swapped_graphs << endl;
#endif
    if (aborted)
        cout << "TIMEOUT" << endl;
}

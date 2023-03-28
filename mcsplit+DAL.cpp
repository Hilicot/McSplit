#include "graph.h"

#include <algorithm>
#include <numeric>
#include <chrono>
#include <iostream>
#include <set>
#include <string>
#include <utility>
//#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include <argp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "mcs.h"

#define VSCORE
// #define DEBUG
#define Best
using std::cout;
using std::endl;
using std::vector;

static void fail(std::string msg)
{
    std::cerr << msg << std::endl;
    exit(1);
}

enum Heuristic
{
    min_max,
    min_product
};

/*******************************************************************************
                             Command-line arguments
*******************************************************************************/

static char doc[] = "Find a maximum clique in a graph in DIMACS format\vHEURISTIC can be min_max or min_product";
static char args_doc[] = "HEURISTIC FILENAME1 FILENAME2";
static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Quiet output"},
    {"verbose", 'v', 0, 0, "Verbose output"},
    {"dimacs", 'd', 0, 0, "Read DIMACS format"},
    {"lad", 'l', 0, 0, "Read LAD format"},
    {"ascii", 'A', 0, 0, "Read ASCII format"},
    {"connected", 'c', 0, 0, "Solve max common CONNECTED subgraph problem"},
    {"directed", 'i', 0, 0, "Use directed graphs"},
    {"labelled", 'a', 0, 0, "Use edge and vertex labels"},
    {"vertex-labelled-only", 'x', 0, 0, "Use vertex labels, but not edge labels"},
    {"big-first", 'b', 0, 0, "First try to find an induced subgraph isomorphism, then decrement the target size"},
    {"timeout", 't', "timeout", 0, "Specify a timeout (seconds)"},
    {0}};

static struct
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
} arguments;

static std::atomic<bool> abort_due_to_timeout;

void set_default_arguments()
{
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
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    switch (key)
    {
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
        if (arguments.arg_num == 0)
        {
            if (std::string(arg) == "min_max")
                arguments.heuristic = min_max;
            else if (std::string(arg) == "min_product")
                arguments.heuristic = min_product;
            else
                fail("Unknown heuristic (try min_max or min_product)");
        }
        else if (arguments.arg_num == 1)
        {
            arguments.filename1 = arg;
        }
        else if (arguments.arg_num == 2)
        {
            arguments.filename2 = arg;
        }
        else
        {
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
                                     Stats
*******************************************************************************/

unsigned long long nodes{0};
unsigned long long cutbranches{0};
unsigned long long conflicts = 0;
clock_t bestfind;
unsigned long long bestnodes = 0, bestcount = 0;
int dl = 0;
clock_t start;

/*******************************************************************************
                                 MCS functions
*******************************************************************************/

using gtype = double;
const int short_memory_threshold = 1e5;
const int long_memory_threshold = 1e9;
int policy_threshold = 0;
int current_policy = 1;
int policy_switch_counter = 0;

struct VtxPair
{
    int v;
    int w;
    VtxPair(int v, int w) : v(v), w(w) {}
};

struct Bidomain
{
    int l, r; // start indices of left and right sets
    int left_len, right_len;
    bool is_adjacent;
    Bidomain(int l, int r, int left_len, int right_len, bool is_adjacent) : l(l),
                                                                            r(r),
                                                                            left_len(left_len),
                                                                            right_len(right_len),
                                                                            is_adjacent(is_adjacent){};
};

void show(const vector<VtxPair> &current, const vector<Bidomain> &domains,
          const vector<int> &left, const vector<int> &right)
{
    cout << "Nodes: " << nodes << std::endl;
    cout << "Length of current assignment: " << current.size() << std::endl;
    cout << "Current assignment:";
    for (unsigned int i = 0; i < current.size(); i++)
    {
        cout << "  (" << current[i].v << " -> " << current[i].w << ")";
    }
    cout << std::endl;
    for (unsigned int i = 0; i < domains.size(); i++)
    {
        struct Bidomain bd = domains[i];
        cout << "Left  ";
        for (int j = 0; j < bd.left_len; j++)
            cout << left[bd.l + j] << " ";
        cout << std::endl;
        cout << "Right  ";
        for (int j = 0; j < bd.right_len; j++)
            cout << right[bd.r + j] << " ";
        cout << std::endl;
    }
    cout << "\n"
         << std::endl;
}

bool check_sol(const Graph &g0, const Graph &g1, const vector<VtxPair> &solution)
{
    return true;
    vector<bool> used_left(g0.n, false);
    vector<bool> used_right(g1.n, false);
    for (unsigned int i = 0; i < solution.size(); i++)
    {
        struct VtxPair p0 = solution[i];
        if (used_left[p0.v] || used_right[p0.w])
            return false;
        used_left[p0.v] = true;
        used_right[p0.w] = true;
        if (g0.adjlist[p0.v].label != g1.adjlist[p0.w].label)
            return false;
        for (unsigned int j = i + 1; j < solution.size(); j++)
        {
            struct VtxPair p1 = solution[j];
            if (g0.get(p0.v, p1.v) != g1.get(p0.w, p1.w))
                return false;
        }
    }
    return true;
}

int calc_bound(const vector<Bidomain> &domains)
{
    int bound = 0;
    for (const Bidomain &bd : domains)
    {
        bound += std::min(bd.left_len, bd.right_len);
    }
    return bound;
}

int selectV_index(const vector<int> &arr, const vector<gtype> &lgrade, int start_idx, int len)
{
    int idx = -1;
    gtype max_g = -1;
    int vtx, best_vtx = INT_MAX;
    for (int i = 0; i < len; i++)
    {
        vtx = arr[start_idx + i];
        if (lgrade[vtx] > max_g)
        {
            idx = i;
            best_vtx = vtx;
            max_g = lgrade[vtx];
        }
        else if (lgrade[vtx] == max_g)
        {
            if (vtx < best_vtx)
            {
                idx = i;
                best_vtx = vtx;
            }
        }
    }
    return idx;
}

int select_bidomain(const vector<Bidomain> &domains, const vector<int> &left, const vector<gtype> &lgrade,
                    int current_matching_size)
{
    // Select the bidomain with the smallest max(leftsize, rightsize), breaking
    // ties on the smallest vertex index in the left set
    int min_size = INT_MAX;
    int min_tie_breaker = INT_MAX;
    int tie_breaker;
    unsigned int i;
    int len;
    int best = -1;
    for (i = 0; i < domains.size(); i++)
    {
        const Bidomain &bd = domains[i];
        if (arguments.connected && current_matching_size > 0 && !bd.is_adjacent)
            continue;
        len = arguments.heuristic == min_max ? std::max(bd.left_len, bd.right_len) : bd.left_len * bd.right_len; // 最多有这么多种连线
        if (len < min_size)
        {                                                                                  // 优先计算可能连线少的情况,当domain中可能配对数最小时，选择domain中含有最大度的点那个domain
            min_size = len;                                                                // 此时进行修改将度最大的点进行优化
            min_tie_breaker = left[bd.l + selectV_index(left, lgrade, bd.l, bd.left_len)]; // 序号越小度越大
            best = i;
        }
        else if (len == min_size)
        {
            tie_breaker = left[bd.l + selectV_index(left, lgrade, bd.l, bd.left_len)];
            if (tie_breaker < min_tie_breaker)
            {
                min_tie_breaker = tie_breaker;
                best = i;
            }
        }
    }
    return best;
}

// Returns length of left half of array
int partition(vector<int> &all_vv, int start, int len, const Graph &g, int index)
{
    int i = 0;
    for (int j = 0; j < len; j++)
    {
        if (g.get(index, all_vv[start + j]))
        {
            std::swap(all_vv[start + i], all_vv[start + j]);
            i++;
        }
    }
    return i;
}

int remove_matched_vertex(vector<int> &arr, int start, int len, const vector<int> &matched)
{
    int p = 0;
    for (int i = 0; i < len; i++)
    {
        if (!matched[arr[start + i]])
        {
            std::swap(arr[start + i], arr[start + p]);
            p++;
        }
    }
    return p;
}

// multiway is for directed and/or labelled graphs
vector<Bidomain> rewardfeed(const vector<Bidomain> &d, int bd_idx, vector<VtxPair> &current, vector<int> &g0_matched, vector<int> &g1_matched,
                            vector<int> &left, vector<int> &right, vector<vector<gtype>> &lgrades, vector<vector<gtype>> &rgrades,
                            const Graph &g0, const Graph &g1, int v, int w,
                            bool multiway)
{ // 每个domain均分成2个new_domain分别是与当前对应点相连或者不相连
    //    assert(g0_matched[v] == 0);
    //    assert(g1_matched[w] == 0);
    current.push_back(VtxPair(v, w));
    g0_matched[v] = 1;
    g1_matched[w] = 1;

    int leaves_match_size = 0, v_leaf, w_leaf;
    for (unsigned int i = 0, j = 0; i < g0.leaves[v].size() && j < g1.leaves[w].size();)
    {
        if (g0.leaves[v][i].first < g1.leaves[w][j].first)
            i++;
        else if (g0.leaves[v][i].first > g1.leaves[w][j].first)
            j++;
        else
        {
            const vector<int> &leaf0 = g0.leaves[v][i].second;
            const vector<int> &leaf1 = g1.leaves[w][j].second;
            for (unsigned int p = 0, q = 0; p < leaf0.size() && q < leaf1.size();)
            {
                if (g0_matched[leaf0[p]])
                    p++;
                else if (g1_matched[leaf1[q]])
                    q++;
                else
                {
                    v_leaf = leaf0[p], w_leaf = leaf1[q];
                    p++, q++;
                    current.push_back(VtxPair(v_leaf, w_leaf));
                    g0_matched[v_leaf] = 1;
                    g1_matched[w_leaf] = 1;
                    leaves_match_size++;
                }
            }
            i++, j++;
        }
    }

    vector<Bidomain> new_d;
    new_d.reserve(d.size());
    // unsigned int old_bound = current.size() + calc_bound(d)+1;//这里的domain是已经去掉选了的点，但是该点还没纳入current所以+1
    // unsigned int new_bound(0);
    int l, r, j = -1;
    int temp = 0, total = 0;
    int unmatched_left_len, unmatched_right_len;
    for (const Bidomain &old_bd : d)
    {
        j++;
        l = old_bd.l;
        r = old_bd.r;
        if (leaves_match_size > 0 && old_bd.is_adjacent == false)
        {
            unmatched_left_len = remove_matched_vertex(left, l, old_bd.left_len, g0_matched);
            unmatched_right_len = remove_matched_vertex(right, r, old_bd.right_len, g1_matched);
        }
        else
        {
            unmatched_left_len = old_bd.left_len;
            unmatched_right_len = old_bd.right_len;
        }
        // After these two partitions, left_len and right_len are the lengths of the
        // arrays of vertices with edges from v or w (int the directed case, edges
        // either from or to v or w)
        int left_len = partition(left, l, unmatched_left_len, g0, v);    // 将与选出来的顶点相连顶点数返回，
        int right_len = partition(right, r, unmatched_right_len, g1, w); // 并且将相连顶点依次交换到数组前面
        int left_len_noedge = unmatched_left_len - left_len;
        int right_len_noedge = unmatched_right_len - right_len;
        // 这里传递v,w选取的下标到bd_idx，j用来计算当前所分domain的下标，这样就能将bound更精确地计算
        temp = std::min(old_bd.left_len, old_bd.right_len) - std::min(left_len, right_len) - std::min(left_len_noedge, right_len_noedge);
        total += temp;

#ifdef DEBUG

        printf("adj=%d ,noadj=%d ,old=%d ,temp=%d \n", std::min(left_len, right_len),
               std::min(left_len_noedge, right_len_noedge), std::min(old_bd.left_len, old_bd.right_len), temp);
        cout << "j=" << j << "  idx=" << bd_idx << endl;
        cout << "gl=" << lgrade[v] << " gr=" << rgrade[w] << endl;
#endif
        if (left_len_noedge && right_len_noedge) // new_domain存在的条件是同一domain内需要存在与该对应点同时相连，或者同时不相连的点的点
            new_d.push_back({l + left_len, r + right_len, left_len_noedge, right_len_noedge, old_bd.is_adjacent});
        if (multiway && left_len && right_len)
        { // 不与顶点相连的顶点
            auto l_begin = std::begin(left) + l;
            auto r_begin = std::begin(right) + r;
            std::sort(l_begin, l_begin + left_len,
                      [&](int a, int b)
                      { return g0.get(v, a) < g0.get(v, b); });
            std::sort(r_begin, r_begin + right_len,
                      [&](int a, int b)
                      { return g1.get(w, a) < g1.get(w, b); });
            int l_top = l + left_len;
            int r_top = r + right_len;
            while (l < l_top && r < r_top)
            {
                unsigned int left_label = g0.get(v, left[l]);
                unsigned int right_label = g1.get(w, right[r]);
                if (left_label < right_label)
                {
                    l++;
                }
                else if (left_label > right_label)
                {
                    r++;
                }
                else
                {
                    int lmin = l;
                    int rmin = r;
                    do
                    {
                        l++;
                    } while (l < l_top && g0.get(v, left[l]) == left_label);
                    do
                    {
                        r++;
                    } while (r < r_top && g1.get(w, right[r]) == left_label);
                    new_d.push_back({lmin, rmin, l - lmin, r - rmin, true});
                }
            }
        }
        else if (left_len && right_len)
        {
            new_d.push_back({l, r, left_len, right_len, true}); // 与顶点相连的顶点 标志为Ture
        }
    }
    for (int p = 0; p < (int)lgrades.size(); ++p)
    {
        int to_add = p == 0 ? (int)new_d.size() : 0;
        if (total + to_add > 0)
        {
            conflicts++;

            lgrades[p][v] += total + to_add;
            rgrades[p][w] += total + to_add;

            if (lgrades[p][v] > short_memory_threshold)
                for (int i = 0; i < g0.n; i++)
                    lgrades[p][i] = lgrades[p][i] / 2;
            if (rgrades[p][w] > long_memory_threshold)
                for (int i = 0; i < g1.n; i++)
                    rgrades[p][i] = rgrades[p][i] / 2;
        }
    }
#ifdef DEBUG
    cout << "new domains are " << endl;
    for (const Bidomain &testd : new_d)
    {
        l = testd.l;
        r = testd.r;
        for (j = 0; j < testd.left_len; j++)
            cout << left[l + j] << " ";
        cout << " ; ";
        for (j = 0; j < testd.right_len; j++)
            cout << right[r + j] << " ";
        cout << endl;
    }
#endif
    return new_d;
}

int selectW_index(const vector<int> &arr, const vector<gtype> &rgrade, int start_idx, int len, const vector<int> &wselected)
{
    int idx = -1;
    gtype max_g = -1;
    int vtx, best_vtx = INT_MAX;
    for (int i = 0; i < len; i++)
    {
        vtx = arr[start_idx + i];
        if (wselected[vtx] == 0)
        {
            if (rgrade[vtx] > max_g)
            {
                idx = i;
                best_vtx = vtx;
                max_g = rgrade[vtx];
            }
            else if (rgrade[vtx] == max_g)
            {
                if (vtx < best_vtx)
                {
                    idx = i;
                    best_vtx = vtx;
                }
            }
        }
    }
    return idx;
}

void remove_vtx_from_array(vector<int> &arr, int start_idx, int &len, int remove_idx)
{
    len--;
    std::swap(arr[start_idx + remove_idx], arr[start_idx + len]);
}

void update_policy_counter(bool reset)
{
    if (reset)
        policy_switch_counter = 0;
    else
    {
        policy_switch_counter += 1;
        if (policy_switch_counter > policy_threshold)
            current_policy = 1 - current_policy;
    }
}

void remove_bidomain(vector<Bidomain> &domains, int idx)
{
    domains[idx] = domains[domains.size() - 1];
    domains.pop_back();
}



int main(int argc, char **argv)
{
    set_default_arguments();
    argp_parse(&argp, argc, argv, 0, 0, 0);

    char format = arguments.dimacs ? 'D' : arguments.lad ? 'L'
                                       : arguments.ascii ? 'A'
                                                         : 'B';
    struct Graph g0 = readGraph(arguments.filename1, format, arguments.directed,
                                arguments.edge_labelled, arguments.vertex_labelled);
    struct Graph g1 = readGraph(arguments.filename2, format, arguments.directed,
                                arguments.edge_labelled, arguments.vertex_labelled);

    policy_threshold = 2 * std::min(g0.n, g1.n);

    //  std::thread timeout_thread;
    //  std::mutex timeout_mutex;
    std::condition_variable timeout_cv;
    abort_due_to_timeout.store(false);
    bool aborted = false;
#if 0
    if (0 != arguments.timeout) {
        timeout_thread = std::thread([&] {
                auto abort_time = std::chrono::steady_clock::now() + std::chrono::seconds(arguments.timeout);
                {
                    /* Sleep until either we've reached the time limit,
                     * or we've finished all the work. */
                    std::unique_lock<std::mutex> guard(timeout_mutex);
                    while (! abort_due_to_timeout.load()) {
                        if (std::cv_status::timeout == timeout_cv.wait_until(guard, abort_time)) {
                            /* We've woken up, and it's due to a timeout. */
                            aborted = true;
                            break;
                        }
                    }
                }
                abort_due_to_timeout.store(true);
                });
    }
#endif
    //  auto start = std::chrono::steady_clock::now();
    start = clock();

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
    std::stable_sort(std::begin(vv0), std::end(vv0), [&](int a, int b)
                     { return g1_dense ? (g0_deg[a] < g0_deg[b]) : (g0_deg[a] > g0_deg[b]); });

    vector<int> vv1(g1.n);
    std::iota(std::begin(vv1), std::end(vv1), 0);
    bool g0_dense = sum(g0_deg) > g0.n * (g0.n - 1);
    std::stable_sort(std::begin(vv1), std::end(vv1), [&](int a, int b) { //????????????????????????????????????????????????????
        return g0_dense ? (g1_deg[a] < g1_deg[b]) : (g1_deg[a] > g1_deg[b]);
    });
    cout << "Sorting done" << endl;

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
    clock_t time_elapsed = clock() - start;
    cout << "Induced subgraph calculated in " << time_elapsed * 1000 / CLOCKS_PER_SEC << "ms" << endl;
#if 0
    int idx;
    for(idx=0;idx<g0.n;idx++)
        cout<<g0.label[idx]<<" ";
    cout<<endl;
    for(idx=0;idx<g0_sorted.n;idx++)
        cout<<g0_sorted.label[idx]<<" ";
    cout<<endl;
#endif

    pack_leaves(g0_sorted);
    pack_leaves(g1_sorted);

    start = clock();

    vector<VtxPair> solution = mcs(g0_sorted, g1_sorted);

    // Convert to indices from original, unsorted graphs
    for (auto &vtx_pair : solution)
    {
        vtx_pair.v = vv0[vtx_pair.v];
        vtx_pair.w = vv1[vtx_pair.w];
    }

    // auto stop = std::chrono::steady_clock::now();
    // auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    time_elapsed = clock() - start;
    clock_t time_find = bestfind - start;
    /* Clean up the timeout thread */
#if 0
    if (timeout_thread.joinable()) {
        {
            std::unique_lock<std::mutex> guard(timeout_mutex);
            abort_due_to_timeout.store(true);
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

    cout << "Nodes:                      " << nodes << endl;
    cout << "Cut branches:               " << cutbranches << endl;
    cout << "Conflicts:                    " << conflicts << endl;
    printf("CPU time (ms):              %15ld\n", time_elapsed * 1000 / CLOCKS_PER_SEC);
    printf("FindBest time (ms):              %15ld\n", time_find * 1000 / CLOCKS_PER_SEC);
#ifdef Best
    cout << "Best nodes:                 " << bestnodes << endl;
    cout << "Best count:                 " << bestcount << endl;
#endif
    if (aborted)
        cout << "TIMEOUT" << endl;
}

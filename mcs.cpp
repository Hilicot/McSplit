#include <iostream>
#include <algorithm>
#include <set>
#include "mcs.h"

using namespace std;

const int short_memory_threshold = 1e5;
const int long_memory_threshold = 1e9;
int policy_threshold = 0;
int current_policy = 1;
int policy_switch_counter = 0;


void show(const vector<VtxPair> &current, const vector<Bidomain> &domains,
          const vector<int> &left, const vector<int> &right, Stats *stats) {
    cout << "Nodes: " << stats->nodes << std::endl;
    cout << "Length of current assignment: " << current.size() << std::endl;
    cout << "Current assignment:";
    for (unsigned int i = 0; i < current.size(); i++) {
        cout << "  (" << current[i].v << " -> " << current[i].w << ")";
    }
    cout << std::endl;
    for (unsigned int i = 0; i < domains.size(); i++) {
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

int calc_bound(const vector<Bidomain> &domains) {
    int bound = 0;
    for (const Bidomain &bd : domains) {
        bound += std::min(bd.left_len, bd.right_len);
    }
    return bound;
}

int selectV_index(const vector<int> &arr, const vector<gtype> &lgrade, int start_idx, int len) {
    int idx = -1;
    gtype max_g = -1;
    int vtx, best_vtx = INT_MAX;
    for (int i = 0; i < len; i++) {
        vtx = arr[start_idx + i];
        if (lgrade[vtx] > max_g) {
            idx = i;
            best_vtx = vtx;
            max_g = lgrade[vtx];
        } else if (lgrade[vtx] == max_g) {
            if (vtx < best_vtx) {
                idx = i;
                best_vtx = vtx;
            }
        }
    }
    return idx;
}

int select_bidomain(const vector<Bidomain> &domains, const vector<int> &left, const vector<gtype> &lgrade,
                    int current_matching_size) {
    // Select the bidomain with the smallest max(leftsize, rightsize), breaking
    // ties on the smallest vertex index in the left set
    int min_size = INT_MAX;
    int min_tie_breaker = INT_MAX;
    int tie_breaker;
    unsigned int i;
    int len;
    int best = -1;
    //
    for (i = 0; i < domains.size(); i++) {
        const Bidomain &bd = domains[i];
        if (arguments.connected && current_matching_size > 0 && !bd.is_adjacent)
            continue;
        len = arguments.heuristic == min_max ? std::max(bd.left_len, bd.right_len) : bd.left_len * bd.right_len;
        if (len < min_size) {
            min_size = len;
            min_tie_breaker = left[bd.l + selectV_index(left, lgrade, bd.l, bd.left_len)];
            best = i;
        } else if (len == min_size) {
            tie_breaker = left[bd.l + selectV_index(left, lgrade, bd.l, bd.left_len)];
            if (tie_breaker < min_tie_breaker) {
                min_tie_breaker = tie_breaker;
                best = i;
            }
        }
    }
    return best;
}

// Returns length of left half of array
int partition(vector<int> &all_vv, int start, int len, const Graph &g, int index) {
    int i = 0;
    for (int j = 0; j < len; j++) {
        if (g.get(index, all_vv[start + j])) {
            std::swap(all_vv[start + i], all_vv[start + j]);
            i++;
        }
    }
    return i;
}

int remove_matched_vertex(vector<int> &arr, int start, int len, const vector<int> &matched) {
    int p = 0;
    for (int i = 0; i < len; i++) {
        if (!matched[arr[start + i]]) {
            std::swap(arr[start + i], arr[start + p]);
            p++;
        }
    }
    return p;
}

// multiway is for directed and/or labelled graphs
vector<Bidomain> rewardfeed(const vector<Bidomain> &d, int bd_idx, vector<VtxPair> &current, vector<int> &g0_matched,
                            vector<int> &g1_matched,
                            vector<int> &left, vector<int> &right, vector<vector<gtype>> &lgrades,
                            vector<vector<gtype>> &rgrades,
                            const Graph &g0, const Graph &g1, int v, int w,
                            bool multiway, Stats *stats) { // 每个domain均分成2个new_domain分别是与当前对应点相连或者不相连
    //    assert(g0_matched[v] == 0);
    //    assert(g1_matched[w] == 0);
    current.push_back(VtxPair(v, w));
    g0_matched[v] = 1;
    g1_matched[w] = 1;

    int leaves_match_size = 0, v_leaf, w_leaf;
    for (unsigned int i = 0, j = 0; i < g0.leaves[v].size() && j < g1.leaves[w].size();) {
        if (g0.leaves[v][i].first < g1.leaves[w][j].first)
            i++;
        else if (g0.leaves[v][i].first > g1.leaves[w][j].first)
            j++;
        else {
            const vector<int> &leaf0 = g0.leaves[v][i].second;
            const vector<int> &leaf1 = g1.leaves[w][j].second;
            for (unsigned int p = 0, q = 0; p < leaf0.size() && q < leaf1.size();) {
                if (g0_matched[leaf0[p]])
                    p++;
                else if (g1_matched[leaf1[q]])
                    q++;
                else {
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
    for (const Bidomain &old_bd : d) {
        j++;
        l = old_bd.l;
        r = old_bd.r;
        if (leaves_match_size > 0 && old_bd.is_adjacent == false) {
            unmatched_left_len = remove_matched_vertex(left, l, old_bd.left_len, g0_matched);
            unmatched_right_len = remove_matched_vertex(right, r, old_bd.right_len, g1_matched);
        } else {
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
        temp = std::min(old_bd.left_len, old_bd.right_len) - std::min(left_len, right_len) -
               std::min(left_len_noedge, right_len_noedge);
        total += temp;

#ifdef DEBUG

        printf("adj=%d ,noadj=%d ,old=%d ,temp=%d \n", std::min(left_len, right_len),
               std::min(left_len_noedge, right_len_noedge), std::min(old_bd.left_len, old_bd.right_len), temp);
        cout << "j=" << j << "  idx=" << bd_idx << endl;
        cout << "gl=" << lgrade[v] << " gr=" << rgrade[w] << endl;
#endif
        if (left_len_noedge && right_len_noedge) // new_domain存在的条件是同一domain内需要存在与该对应点同时相连，或者同时不相连的点的点
            new_d.push_back({l + left_len, r + right_len, left_len_noedge, right_len_noedge, old_bd.is_adjacent});
        if (multiway && left_len && right_len) { // 不与顶点相连的顶点
            auto l_begin = std::begin(left) + l;
            auto r_begin = std::begin(right) + r;
            std::sort(l_begin, l_begin + left_len,
                      [&](int a, int b) { return g0.get(v, a) < g0.get(v, b); });
            std::sort(r_begin, r_begin + right_len,
                      [&](int a, int b) { return g1.get(w, a) < g1.get(w, b); });
            int l_top = l + left_len;
            int r_top = r + right_len;
            while (l < l_top && r < r_top) {
                unsigned int left_label = g0.get(v, left[l]);
                unsigned int right_label = g1.get(w, right[r]);
                if (left_label < right_label) {
                    l++;
                } else if (left_label > right_label) {
                    r++;
                } else {
                    int lmin = l;
                    int rmin = r;
                    do {
                        l++;
                    } while (l < l_top && g0.get(v, left[l]) == left_label);
                    do {
                        r++;
                    } while (r < r_top && g1.get(w, right[r]) == left_label);
                    new_d.push_back({lmin, rmin, l - lmin, r - rmin, true});
                }
            }
        } else if (left_len && right_len) {
            new_d.push_back({l, r, left_len, right_len, true}); // 与顶点相连的顶点 标志为Ture
        }
    }
    for (int p = 0; p < (int) lgrades.size(); ++p) {
        int to_add = p == 0 ? (int) new_d.size() : 0;
        if (total + to_add > 0) {
            stats->conflicts++;

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

int selectW_index(const vector<int> &arr, const vector<gtype> &rgrade, int start_idx, int len,
                  const vector<int> &wselected) {
    int idx = -1;
    gtype max_g = -1;
    int vtx, best_vtx = INT_MAX;
    for (int i = 0; i < len; i++) {
        vtx = arr[start_idx + i];
        if (wselected[vtx] == 0) {
            if (rgrade[vtx] > max_g) {
                idx = i;
                best_vtx = vtx;
                max_g = rgrade[vtx];
            } else if (rgrade[vtx] == max_g) {
                if (vtx < best_vtx) {
                    idx = i;
                    best_vtx = vtx;
                }
            }
        }
    }
    return idx;
}

void remove_vtx_from_array(vector<int> &arr, int start_idx, int &len, int remove_idx) {
    len--;
    std::swap(arr[start_idx + remove_idx], arr[start_idx + len]);
}

void update_policy_counter(bool reset) {
    if (reset)
        policy_switch_counter = 0;
    else {
        policy_switch_counter += 1;
        if (policy_switch_counter > policy_threshold) {
            policy_switch_counter = 0;
            current_policy = 1 - current_policy;
        }
    }
}

void remove_bidomain(vector<Bidomain> &domains, int idx) {
    domains[idx] = domains[domains.size() - 1];
    domains.pop_back();
}


void solve(const Graph &g0, const Graph &g1, vector<vector<gtype>> &V, vector<vector<vector<gtype>>> &Q,
           vector<VtxPair> &incumbent,
           vector<VtxPair> &current, vector<int> &g0_matched, vector<int> &g1_matched,
           vector<Bidomain> &domains, vector<int> &left, vector<int> &right, unsigned int matching_size_goal,
           Stats *stats) {
    // FIXME we have 2 timeout systems, remove one of them (the first seems to not work...)
    if (arguments.timeout && double(clock() - stats->start) / CLOCKS_PER_SEC > arguments.timeout) {
        return;
    }
    if (stats->abort_due_to_timeout)
        return;
    stats->nodes++;
    // if (arguments.verbose) show(current, domains, left, right, stats);

    if (current.size() > incumbent.size()) { // incumbent 现任的
        incumbent = current;
        stats->bestcount = stats->cutbranches + 1;
        stats->bestnodes = stats->nodes;
        // avoid printing size of each iteration if they are too close in time (save log space)
        if (!arguments.quiet && clock() - stats->bestfind >1000)
            cout << "Incumbent size: " << incumbent.size() << endl;
        stats->bestfind = clock();

        update_policy_counter(true);
    }

    // prune branch if upper bound is too small
    unsigned int bound = current.size() + calc_bound(domains);
    if (bound <= incumbent.size() || bound < matching_size_goal) {
        stats->cutbranches++;
        return;
    }
    // exit branch if goal already reached in big_first policy
    if (arguments.big_first && incumbent.size() == matching_size_goal)
        return;

    // select bidomain based on heuristic
    int bd_idx = select_bidomain(domains, left, V[current_policy], current.size());
    if (bd_idx == -1) { // In the MCCS case, there may be nothing we can branch on
        //  cout<<endl;
        //  cout<<endl;
        //  cout<<"big"<<endl;
        return;
    }
    Bidomain &bd = domains[bd_idx];

    int v, w;
    int tmp_idx;

    // select vertex v (vertex with max reward)
    tmp_idx = selectV_index(left, V[current_policy], bd.l, bd.left_len);
    v = left[bd.l + tmp_idx];
    remove_vtx_from_array(left, bd.l, bd.left_len, tmp_idx); // remove v from bidomain
    update_policy_counter(false);   //

    // Try assigning v to each vertex w in the colour class beginning at bd.r, in turn
    vector<int> wselected(g1.n, 0);
    bd.right_len--; //
    for (int i = 0; i <= bd.right_len; i++) {
        tmp_idx = selectW_index(right, Q[v][current_policy], bd.r, bd.right_len + 1, wselected);
        w = right[bd.r + tmp_idx];
        wselected[w] = 1;
        std::swap(right[bd.r + tmp_idx], right[bd.r + bd.right_len]);
        update_policy_counter(false);

#ifdef DEBUG
        cout << "v= " << v << " w= " << w << endl;
        unsigned int m;
        for (m = 0; m < left.size(); m++)
            cout << left[m] << " ";
        cout << endl;
        for (m = 0; m < right.size(); m++)
            cout << right[m] << " ";
        cout << endl;
#endif
        unsigned int cur_len = current.size();
        auto new_domains = rewardfeed(domains, bd_idx, current, g0_matched, g1_matched, left, right, V, Q[v], g0, g1, v,
                                      w,
                                      arguments.directed || arguments.edge_labelled, stats);

        stats->dl++;
        solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, new_domains, left, right, matching_size_goal,
              stats);
        while (current.size() > cur_len) {
            VtxPair pr = current.back();
            current.pop_back();
            g0_matched[pr.v] = 0;
            g1_matched[pr.w] = 0;
        }
    }
    bd.right_len++;
    if (bd.left_len == 0)
        remove_bidomain(domains, bd_idx);
    solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, domains, left, right, matching_size_goal, stats);
}

vector<VtxPair> mcs(const Graph &g0, const Graph &g1, Stats *stats) {
    policy_threshold = 2 * std::min(g0.n, g1.n);

    vector<int> left;  // the buffer of vertex indices for the left partitions
    vector<int> right; // the buffer of vertex indices for the right partitions

    vector<int> g0_matched(g0.n, 0);
    vector<int> g1_matched(g1.n, 0);

    vector<vector<gtype>> V(2, vector<gtype>(g0.n, 0));
    vector<vector<vector<gtype>>> Q(g0.n, vector<vector<gtype>>(2, vector<gtype>(g1.n, 0)));

    auto domains = vector<Bidomain>{};

    std::set<unsigned int> left_labels;
    std::set<unsigned int> right_labels;
    for (auto node : g0.adjlist)
        left_labels.insert(node.label);
    for (auto node : g1.adjlist)
        right_labels.insert(node.label);
    std::set<unsigned int> labels; // labels that appear in both graphs
    std::set_intersection(std::begin(left_labels),
                          std::end(left_labels),
                          std::begin(right_labels),
                          std::end(right_labels),
                          std::inserter(labels, std::begin(labels)));

    // Create a bidomain for each label that appears in both graphs (only one at the start)
    for (unsigned int label : labels) {
        int start_l = left.size();
        int start_r = right.size();

        for (int i = 0; i < g0.n; i++)
            if (g0.adjlist[i].label == label)
                left.push_back(i);
        for (int i = 0; i < g1.n; i++)
            if (g1.adjlist[i].label == label)
                right.push_back(i);

        int left_len = left.size() - start_l;
        int right_len = right.size() - start_r;
        domains.push_back({start_l, start_r, left_len, right_len, false});
    }

    vector<VtxPair> incumbent;

    if (arguments.big_first) {
        for (int k = 0; k < g0.n; k++) {
            unsigned int goal = g0.n - k;
            auto left_copy = left;
            auto right_copy = right;
            auto domains_copy = domains;
            vector<VtxPair> current;
            solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, domains_copy, left_copy, right_copy, goal,
                  stats);
            if (incumbent.size() == goal || stats->abort_due_to_timeout)
                break;
            if (!arguments.quiet)
                cout << "Upper bound: " << goal - 1 << std::endl;
        }
    } else {
        vector<VtxPair> current;
        solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, domains, left, right, 1, stats);
    }

    if (arguments.timeout && double(clock() - stats->start) / CLOCKS_PER_SEC > arguments.timeout) {
        cout << "time out" << endl;
    }

    return incumbent;
}




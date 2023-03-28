#include "mcs.h"

void solve(const Graph &g0, const Graph &g1, vector<vector<gtype>> &V, vector<vector<vector<gtype>>> &Q, vector<VtxPair> &incumbent,
           vector<VtxPair> &current, vector<int> &g0_matched, vector<int> &g1_matched,
           vector<Bidomain> &domains, vector<int> &left, vector<int> &right, unsigned int matching_size_goal)
{
    if (arguments.timeout && double(clock() - start) / CLOCKS_PER_SEC > arguments.timeout)
    {
        return;
    }
    //  if (abort_due_to_timeout)
    //     return;
    nodes++;
    // if (arguments.verbose) show(current, domains, left, right);

    if (current.size() > incumbent.size())
    { // incumbent 现任的
        incumbent = current;
        bestcount = cutbranches + 1;
        bestnodes = nodes;
        bestfind = clock();
        if (!arguments.quiet)
            cout << "Incumbent size: " << incumbent.size() << endl;

        update_policy_counter(true);
    }

    unsigned int bound = current.size() + calc_bound(domains); // 计算相连和不相连同构数的最大可能加上当前已经同构的点数
    if (bound <= incumbent.size() || bound < matching_size_goal)
    { // 剪枝
        cutbranches++;
        return;
    }
    if (arguments.big_first && incumbent.size() == matching_size_goal)
        return;

    int bd_idx = select_bidomain(domains, left, V[current_policy], current.size()); // 选出domain中可能情况最少的下标
    if (bd_idx == -1)
    { // In the MCCS case, there may be nothing we can branch on
        //  cout<<endl;
        //  cout<<endl;
        //  cout<<"big"<<endl;
        return;
    }
    Bidomain &bd = domains[bd_idx];

    int v, w;
    int tmp_idx;

    tmp_idx = selectV_index(left, V[current_policy], bd.l, bd.left_len);
    v = left[bd.l + tmp_idx];
    remove_vtx_from_array(left, bd.l, bd.left_len, tmp_idx);
    update_policy_counter(false);

    // Try assigning v to each vertex w in the colour class beginning at bd.r, in turn
    vector<int> wselected(g1.n, 0);
    bd.right_len--; //
    for (int i = 0; i <= bd.right_len; i++)
    {
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
        auto new_domains = rewardfeed(domains, bd_idx, current, g0_matched, g1_matched, left, right, V, Q[v], g0, g1, v, w,
                                      arguments.directed || arguments.edge_labelled);

        dl++;
        solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, new_domains, left, right, matching_size_goal);
        while (current.size() > cur_len)
        {
            VtxPair pr = current.back();
            current.pop_back();
            g0_matched[pr.v] = 0;
            g1_matched[pr.w] = 0;
        }
    }
    bd.right_len++;
    if (bd.left_len == 0)
        remove_bidomain(domains, bd_idx);
    solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, domains, left, right, matching_size_goal);
}

vector<VtxPair> mcs(const Graph &g0, const Graph &g1)
{
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

    // Create a bidomain for each label that appears in both graphs
    for (unsigned int label : labels)
    {
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

    if (arguments.big_first)
    {
        for (int k = 0; k < g0.n; k++)
        {
            unsigned int goal = g0.n - k;
            auto left_copy = left;
            auto right_copy = right;
            auto domains_copy = domains;
            vector<VtxPair> current;
            solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, domains_copy, left_copy, right_copy, goal);
            if (incumbent.size() == goal || abort_due_to_timeout)
                break;
            if (!arguments.quiet)
                cout << "Upper bound: " << goal - 1 << std::endl;
        }
    }
    else
    {
        vector<VtxPair> current;
        solve(g0, g1, V, Q, incumbent, current, g0_matched, g1_matched, domains, left, right, 1);
    }

    if (arguments.timeout && double(clock() - start) / CLOCKS_PER_SEC > arguments.timeout)
    {
        cout << "time out" << endl;
    }

    return incumbent;
}

vector<int> calculate_degrees(const Graph &g)
{
    vector<int> degree(g.n, 0);
    for (int v = 0; v < g.n; v++)
    {
        degree[v] = g.adjlist[v].adjNodes.size() - 1;
    }
    return degree;
}

int sum(const vector<int> &vec)
{
    return std::accumulate(std::begin(vec), std::end(vec), 0);
}


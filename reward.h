#ifndef MCSPLIT_REWARD_H
#define MCSPLIT_REWARD_H

#include "mcs.h"

struct Reward{
    // TODO save as float instead of double? to save space and keep rewards in cache as much as possible
    // rl and ll components must be separated in RL_DAL (bc DAL policy is always made by LL+DAL)
private:
    gtype rl_component;
    gtype ll_component;
    gtype dal_component;
    gtype normalized_reward; // TODO remove
public:
    Reward() : rl_component(0), ll_component(0), dal_component(0), normalized_reward(0.0) {}
    void normalize(int rl_max, int dal_max, double factor);
    gtype get_reward(bool normalized) const;
    void reset();
    void decay();
    void update(gtype ll_reward, gtype dal_reward);
};

struct Rewards{
    virtual vector<Reward> get_left_rewards() = 0;
    virtual vector<Reward> get_right_rewards(int v) = 0;
    virtual void update_policy_counter(bool restart_counter) = 0;
    virtual gtype get_vertex_reward(int v, bool normalized) const = 0;
    virtual gtype get_pair_reward(int v, int w, bool normalized) const = 0;
    virtual void update_rewards(const NewBidomainResult &new_domains_result, int v, int w, Stats *stats) = 0;
    Rewards(int n, int m) {};
};
/*
struct SingleQRewards : Rewards{
    vector<Reward> V;
    vector<Reward> Q;

    SingleQRewards(int n, int m) : V(n), Q(m) {}
    vector<Reward> get_left_rewards();
    vector<Reward> get_right_rewards(int v);
};
*/
struct DoubleQRewards : Rewards{
    vector<Reward> V;
    vector<vector<Reward>> Q;

    DoubleQRewards(int n, int m) : Rewards(n,m), V(n), Q(n , vector<Reward>(m)) {}
    vector<Reward> get_left_rewards() override;
    vector<Reward> get_right_rewards(int v) override;
    void update_policy_counter(bool restart_counter) override;
    gtype get_vertex_reward(int v, bool normalized) const override;
    gtype get_pair_reward(int v, int w, bool normalized) const override;
    void empty_rewards();
    void randomize_rewards();
    void update_rewards(const NewBidomainResult &new_domains_result, int v, int w, Stats *stats) override;
};

#endif
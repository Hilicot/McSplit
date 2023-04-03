#ifndef MCSPLIT_REWARD_H
#define MCSPLIT_REWARD_H

#include "mcs.h"

struct Reward
{
    int rl_component;
    int dal_component;
    double normalized_reward;

    Reward() : rl_component(0), dal_component(0), normalized_reward(0.0) {}
    void normalize(int rl_max, int dal_max, double factor);
};

struct Rewards 
{
    virtual vector<Reward> get_left_rewards();
    virtual vector<Reward> get_right_rewards(int v);
};

struct SingleQRewards : Rewards
{
    vector<Reward> V;
    vector<Reward> Q;

    SingleQRewards(int n, int m) : V(n), Q(m) {}
    vector<Reward> get_left_rewards();
    vector<Reward> get_right_rewards(int v);   
};

struct DoubleQRewards : Rewards
{
    vector<Reward> V;
    vector<vector<Reward>> Q;

    DoubleQRewards(int n, int m) : V(n), Q(n , vector<Reward>(m)) {}
    vector<Reward> get_left_rewards();
    vector<Reward> get_right_rewards(int v);
};

#endif
#ifndef MCSPLITDAL_ARGS_H
#define MCSPLITDAL_ARGS_H

enum SwapPolicy
{
    NO_SWAP,
    McSPLIT_SD,
    McSPLIT_SO,
    ADAPTIVE
};

enum Heuristic
{
    min_max,
    min_product
};

enum RewardSwitchPolicy
{
    CHANGE,
    RESET,
    RANDOM
};

struct RewardPolicy
{
    RewardSwitchPolicy switch_policy;
    float reward_coefficient;
    int reward_switch_policy_threshold;
    int reward_policies_num;
    int current_reward_policy;
    int policy_switch_counter;
    RewardPolicy() : reward_coefficient(1.0), reward_switch_policy_threshold(0), reward_policies_num(2), current_reward_policy(0), policy_switch_counter(0) {}
};

static struct arguments{
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
    RewardPolicy reward_policy;
} arguments;

#endif // MCSPLITDAL_ARGS_H

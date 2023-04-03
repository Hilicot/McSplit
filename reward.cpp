#include "reward.h"

void Reward::normalize(int rl_max, int dal_max, double factor)
{
    this->normalized_reward = factor * this->rl_component / rl_max + (1 - factor) * this->dal_component / dal_max;
}

vector<Reward> SingleQRewards::get_left_rewards()
{
    return this->V;
}

vector<Reward> SingleQRewards::get_right_rewards(int v)
{
    return this->Q;
}

vector<Reward> DoubleQRewards::get_left_rewards()
{
    return this->V;
}

vector<Reward> DoubleQRewards::get_right_rewards(int v)
{
    return this->Q[v];
}

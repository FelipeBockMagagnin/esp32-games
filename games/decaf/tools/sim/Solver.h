// Finds a way through a floor with the real Stage: a beam search over short held inputs, scored
// by how many cells of open floor lie between the player and the coworker, plus the boss's hits.
// A route that never dies proves the floor can be finished; film.cpp plays it to take pictures.
#ifndef SIM_SOLVER_H
#define SIM_SOLVER_H

#include <algorithm>
#include <cstdio>
#include <deque>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "../../src/game/Stage.h"

struct SolverAction
{
    int8_t dir;
    bool jump;
    bool dash;
};

static const SolverAction SOLVER_ACTIONS[] = {
    {1, false, false}, {1, true, false}, {0, false, false}, {-1, false, false},
    {-1, true, false}, {0, true, false}, {1, false, true},  {-1, false, true},
};
static const int SOLVER_ACTION_TICKS = 6;

// Steps through open cells (gates open) from the coworker's cell back to every cell
static std::vector<int> solverDistances(const Stage &stage, uint8_t level)
{
    int w = stage.getWidth();
    std::vector<int> dist((size_t)w * MAP_ROWS, -1);
    std::deque<std::pair<int, int>> queue;
    const char *const *map = LEVELS[level].map;
    for (int r = 0; r < MAP_ROWS; r++)
    {
        for (int c = 0; c < w && map[r][c] != '\0'; c++)
        {
            if (map[r][c] == 'F')
            {
                dist[r * w + c] = 0;
                queue.push_back({c, r});
            }
        }
    }
    auto open = [&](int c, int r)
    {
        Tile t = stage.tileAt(c, r);
        return t == Tile::EMPTY || t == Tile::DESK || t == Tile::ENTRY_GATE || t == Tile::EXIT_GATE;
    };
    while (!queue.empty())
    {
        auto [c, r] = queue.front();
        queue.pop_front();
        const int steps[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (auto &s : steps)
        {
            int nc = c + s[0], nr = r + s[1];
            if (nc < 0 || nc >= w || nr < 0 || nr >= MAP_ROWS || dist[nr * w + nc] >= 0 || !open(nc, nr))
            {
                continue;
            }
            dist[nr * w + nc] = dist[r * w + c] + 1;
            queue.push_back({nc, nr});
        }
    }
    return dist;
}

struct SolverNode
{
    std::shared_ptr<Stage> stage;
    std::shared_ptr<const std::vector<uint8_t>> actions;
    double score;
};

static double solverScore(const Stage &s, const std::vector<int> &dist)
{
    int w = s.getWidth();
    int c = (int)((s.getPlayerX() + 6) / TILE);
    int r = (int)((s.getPlayerY() + 11) / TILE);
    double d = 1e6;
    if (c >= 0 && c < w && r >= 0 && r < MAP_ROWS && dist[r * w + c] >= 0)
    {
        d = dist[r * w + c];
    }
    double score = -d * 16;
    score += (3 - s.getBossHitsLeft()) * 4000.0 + (s.isBossDefeated() ? 20000.0 : 0);
    if (s.getPhase() != Stage::Phase::PLAYING)
    {
        score += 1e7;
    }
    return score;
}

static std::vector<Input> solverInputs(const std::vector<uint8_t> &actions)
{
    std::vector<Input> inputs;
    bool held = false;
    for (uint8_t index : actions)
    {
        const SolverAction &a = SOLVER_ACTIONS[index];
        for (int t = 0; t < SOLVER_ACTION_TICKS; t++)
        {
            Input in = {a.dir < 0, a.dir > 0, a.jump, a.jump && !held, a.dash && t == 0};
            held = a.jump;
            inputs.push_back(in);
        }
    }
    return inputs;
}

// The inputs of a deathless run to the coworker, one per step; empty if none was found
static std::vector<Input> solve(uint8_t level, bool verbose, size_t beam = 160, size_t maxDepth = 4000)
{
    auto root = std::make_shared<Stage>();
    root->load(level);
    std::vector<int> dist = solverDistances(*root, level);
    std::vector<SolverNode> frontier = {{root, std::make_shared<std::vector<uint8_t>>(), 0}};
    const size_t actionCount = sizeof(SOLVER_ACTIONS) / sizeof(SOLVER_ACTIONS[0]);

    for (size_t depth = 0; depth < maxDepth && !frontier.empty(); depth++)
    {
        std::map<std::tuple<int, int, int, int, int>, SolverNode> best;
        for (const SolverNode &node : frontier)
        {
            bool held = !node.actions->empty() && SOLVER_ACTIONS[node.actions->back()].jump;
            for (size_t a = 0; a < actionCount; a++)
            {
                auto stage = std::make_shared<Stage>(*node.stage);
                const SolverAction &act = SOLVER_ACTIONS[a];
                bool finished = false;
                for (int t = 0; t < SOLVER_ACTION_TICKS && stage->getDeaths() == 0; t++)
                {
                    Input in = {act.dir < 0, act.dir > 0, act.jump, act.jump && !held && t == 0, act.dash && t == 0};
                    stage->step(in);
                    if (stage->getPhase() != Stage::Phase::PLAYING)
                    {
                        finished = true;
                        break;
                    }
                }
                if (stage->getDeaths() > 0)
                {
                    continue;
                }
                auto actions = std::make_shared<std::vector<uint8_t>>(*node.actions);
                actions->push_back((uint8_t)a);
                if (finished)
                {
                    if (verbose)
                    {
                        printf("solved in %zu actions, %.1f s\n", actions->size(),
                               stage->getTicks() / (double)STEPS_PER_SECOND);
                    }
                    return solverInputs(*actions);
                }
                double score = solverScore(*stage, dist);
                auto key = std::make_tuple((int)stage->getPlayerX() / 3, (int)stage->getPlayerY() / 3,
                                           (int)stage->getBossHitsLeft(), (int)stage->isBossDizzy(),
                                           (int)stage->aliveEnemies());
                auto found = best.find(key);
                if (found == best.end() || found->second.score < score)
                {
                    best[key] = {stage, actions, score};
                }
            }
        }
        frontier.clear();
        for (auto &entry : best)
        {
            frontier.push_back(entry.second);
        }
        std::sort(frontier.begin(), frontier.end(),
                  [](const SolverNode &a, const SolverNode &b) { return a.score > b.score; });
        if (frontier.size() > beam)
        {
            frontier.resize(beam);
        }
        if (verbose && depth % 50 == 0 && !frontier.empty())
        {
            const Stage &s = *frontier.front().stage;
            printf("depth %zu: x %.0f y %.0f score %.0f boss %d\n", depth, s.getPlayerX(), s.getPlayerY(),
                   frontier.front().score, s.getBossHitsLeft());
        }
    }
    return {};
}

#endif // SIM_SOLVER_H

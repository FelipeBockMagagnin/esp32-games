#include "WaterSim.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Tuned on the PC (tools/test.sh measures the settled water): a rest density that spaces the
// particles about 6 px apart, stiff enough that a full box squeezes less than a third at the bottom
static const float REST = 3.0f;
static const float STIFF = 0.5f;
static const float NEAR_STIFF = 1.6f;
// Damps neighbours closing on each other, which stills the surface's jitter
static const float VISCOSITY = 0.3f;
static const float MAX_STEP = WaterSim::RADIUS * 0.5f;
static const float WALL = 2.0f;

bool WaterSim::begin()
{
    if (ready())
    {
        return true;
    }
    float **arrays[] = {&x, &y, &vx, &vy, &ox, &oy, &rho};
    for (float **a : arrays)
    {
        *a = (float *)malloc(MAX_PARTICLES * sizeof(float));
        if (*a == nullptr)
        {
            end();
            return false;
        }
    }
    order = (uint16_t *)malloc(MAX_PARTICLES * sizeof(uint16_t));
    if (order == nullptr)
    {
        end();
        return false;
    }
    fill();
    return true;
}

void WaterSim::end()
{
    float **arrays[] = {&x, &y, &vx, &vy, &ox, &oy, &rho};
    for (float **a : arrays)
    {
        free(*a);
        *a = nullptr;
    }
    free(order);
    order = nullptr;
    n = 0;
}

void WaterSim::fill(int count)
{
    n = count > MAX_PARTICLES ? MAX_PARTICLES : count;
    const float spacing = RADIUS * 0.42f;
    const int perRow = (int)((W - 2 * WALL) / spacing);
    for (int i = 0; i < n; i++)
    {
        int row = i / perRow, col = i % perRow;
        x[i] = WALL + spacing * (col + 0.5f + (row & 1) * 0.5f);
        y[i] = H - WALL - spacing * (row * 0.87f + 0.5f);
        vx[i] = vy[i] = 0;
        rho[i] = 1;
    }
    gx = 0;
    gy = 0.15f;
}

void WaterSim::setGravity(float x_, float y_)
{
    gx = x_;
    gy = y_;
}

void WaterSim::shake(float dx, float dy, uint32_t seed)
{
    for (int i = 0; i < n; i++)
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        float jitter = 0.7f + (seed & 255) / 255.0f * 0.6f;
        vx[i] += dx * jitter;
        vy[i] += dy * jitter;
    }
}

float WaterSim::speed(int i) const
{
    return sqrtf(vx[i] * vx[i] + vy[i] * vy[i]);
}

float WaterSim::density(int i) const
{
    return rho[i] / REST;
}

int WaterSim::cellOf(float px, float py) const
{
    int cx = (int)(px * (1.0f / RADIUS)), cy = (int)(py * (1.0f / RADIUS));
    cx = cx < 0 ? 0 : (cx >= GRID_W ? GRID_W - 1 : cx);
    cy = cy < 0 ? 0 : (cy >= GRID_H ? GRID_H - 1 : cy);
    return cx + cy * GRID_W;
}

// A counting sort: the particles of cell c are order[cellStart[c]..cellStart[c + 1])
void WaterSim::sortIntoCells()
{
    memset(cellStart, 0, sizeof(cellStart));
    for (int i = 0; i < n; i++)
    {
        cellStart[cellOf(x[i], y[i]) + 1]++;
    }
    for (int c = 0; c < GRID_W * GRID_H; c++)
    {
        cellStart[c + 1] += cellStart[c];
    }
    uint16_t fillAt[GRID_W * GRID_H];
    memcpy(fillAt, cellStart, sizeof(fillAt));
    for (int i = 0; i < n; i++)
    {
        order[fillAt[cellOf(x[i], y[i])]++] = (uint16_t)i;
    }
}

void WaterSim::relax(float dt)
{
    const float inv = 1.0f / RADIUS, r2 = RADIUS * RADIUS, dt2 = dt * dt;
    for (int c = 0; c < GRID_W * GRID_H; c++)
    {
        int cx = c % GRID_W, cy = c / GRID_W;
        int x0 = cx > 0 ? cx - 1 : 0, x1 = cx < GRID_W - 1 ? cx + 1 : cx;
        int y0 = cy > 0 ? cy - 1 : 0, y1 = cy < GRID_H - 1 ? cy + 1 : cy;
        for (int s = cellStart[c]; s < cellStart[c + 1]; s++)
        {
            int i = order[s];
            float d = 0, dn = 0;
            for (int ny = y0; ny <= y1; ny++)
            {
                for (int nc = ny * GRID_W + x0; nc <= ny * GRID_W + x1; nc++)
                {
                    for (int t = cellStart[nc]; t < cellStart[nc + 1]; t++)
                    {
                        int j = order[t];
                        float dx = x[j] - x[i], dy = y[j] - y[i];
                        float rr = dx * dx + dy * dy;
                        if (j == i || rr >= r2)
                        {
                            continue;
                        }
                        float q = 1.0f - sqrtf(rr) * inv;
                        d += q * q;
                        dn += q * q * q;
                    }
                }
            }
            rho[i] = d;
            float pressure = STIFF * (d - REST), nearPressure = NEAR_STIFF * dn;
            float mx = 0, my = 0;
            for (int ny = y0; ny <= y1; ny++)
            {
                for (int nc = ny * GRID_W + x0; nc <= ny * GRID_W + x1; nc++)
                {
                    for (int t = cellStart[nc]; t < cellStart[nc + 1]; t++)
                    {
                        int j = order[t];
                        float dx = x[j] - x[i], dy = y[j] - y[i];
                        float rr = dx * dx + dy * dy;
                        if (j == i || rr >= r2)
                        {
                            continue;
                        }
                        float r = sqrtf(rr);
                        if (r < 1e-4f)
                        {
                            // Two particles on the same spot: part them along any fixed line
                            dx = (i < j) ? 0.5f : -0.5f;
                            r = 0.5f;
                        }
                        float q = 1.0f - r * inv;
                        float closing = ((vx[i] - vx[j]) * dx + (vy[i] - vy[j]) * dy) / r;
                        float damp = closing > 0 ? dt * q * VISCOSITY * closing * 0.5f : 0;
                        float push = (dt2 * (pressure * q + nearPressure * q * q) * 0.5f + damp) / r;
                        float px = dx * push, py = dy * push;
                        x[j] += px;
                        y[j] += py;
                        mx -= px;
                        my -= py;
                    }
                }
            }
            x[i] += mx;
            y[i] += my;
        }
    }
}

void WaterSim::keepInside()
{
    for (int i = 0; i < n; i++)
    {
        if (x[i] < WALL)
        {
            x[i] = WALL;
        }
        else if (x[i] > W - WALL)
        {
            x[i] = W - WALL;
        }
        if (y[i] < WALL)
        {
            y[i] = WALL;
        }
        else if (y[i] > H - WALL)
        {
            y[i] = H - WALL;
        }
    }
}

void WaterSim::step()
{
    const float dt = 1.0f;
    for (int i = 0; i < n; i++)
    {
        vx[i] += gx * dt;
        vy[i] += gy * dt;
        float s = vx[i] * vx[i] + vy[i] * vy[i];
        if (s > MAX_STEP * MAX_STEP)
        {
            float k = MAX_STEP / sqrtf(s);
            vx[i] *= k;
            vy[i] *= k;
        }
        ox[i] = x[i];
        oy[i] = y[i];
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
    }
    keepInside();
    sortIntoCells();
    relax(dt);
    keepInside();
    for (int i = 0; i < n; i++)
    {
        vx[i] = (x[i] - ox[i]) / dt;
        vy[i] = (y[i] - oy[i]) / dt;
    }
}

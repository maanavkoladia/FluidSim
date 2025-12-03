#include "Advection.h"
#include "Assert_Common.h"
#include "Controller.h"
#include "ForLoop.h"
#include "LOG.h"
#include "SimTypes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>




// ---------------------------------------------------------
// Basic math helpers / types
// ---------------------------------------------------------

typedef struct {
    double x;
    double y;
} Vector2;

static inline Vector2 vec2_add(Vector2 a, Vector2 b) {
    Vector2 r = { a.x + b.x, a.y + b.y };
    return r;
}

static inline Vector2 vec2_sub(Vector2 a, Vector2 b) {
    Vector2 r = { a.x - b.x, a.y - b.y };
    return r;
}

static inline Vector2 vec2_scale(Vector2 v, double s) {
    Vector2 r = { v.x * s, v.y * s };
    return r;
}

static inline double clamp_double(double v, double minVal, double maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

static inline int clamp_int(int v, int minVal, int maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

static inline double clamp01(double v) {
    return clamp_double(v, 0.0f, 1.0f);
}

static inline double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}


// ---------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------


static bool FluidGrid_IsSolid(const SimState_t* g, int x, int y) {
    int cx = clamp_int(x, 0, g->nx - 1);
    int cy = clamp_int(y, 0, g->ny - 1);
    return g->CellBufs_Arr[g->cellBufInUse][cx][cy].type == SOLID;
}
static Vector2 FluidGrid_CellCentre(const SimState_t* g, int x, int y) {
    double boundsSizeX = (double)g->nx * g->w;
    double boundsSizeY = (double)g->ny * g->w;

    double bottomleftX = -boundsSizeX * 0.5;
    double bottomlefty = -boundsSizeY * 0.5;

    Vector2 base = {bottomleftX,bottomlefty};
    Vector2 offset = { (x + 0.5f) * g->w, (y + 0.5f) * g->w };
    return vec2_add(base, offset);
}

static Vector2 FluidGrid_LeftEdgeCentre(const SimState_t* g, int x, int y) {
    Vector2 c = FluidGrid_CellCentre(g, x, y);
    Vector2 off = { g->w*0.5, 0.0 };
    return vec2_sub(c, off);
}

static Vector2 FluidGrid_BottomEdgeCentre(const SimState_t* g, int x, int y) {
    Vector2 c = FluidGrid_CellCentre(g, x, y);
    Vector2 off = { 0.0, g->w*0.5 };
    return vec2_sub(c, off);
}


static double FluidGrid_SampleBilinearEdgesVertical(
    Cell_t** edgeValues,
    int edgeCountX,
    int edgeCountY,
    double cellSize,
    Vector2 worldPos)
{
    double width  = (double)(edgeCountX - 1) * cellSize;
    double height = (double)(edgeCountY - 1) * cellSize;

    double px = (worldPos.x + width * 0.5f) / cellSize;  // [0, countX]
    double py = (worldPos.y + height * 0.5f) / cellSize; // [0, countY]

    int left   = clamp_int((int)px, 0, edgeCountX - 2);
    int bottom = clamp_int((int)py, 0, edgeCountY - 2);
    int right  = left + 1;
    int top    = bottom + 1;

    double xFrac = clamp01(px - (double)left);
    double yFrac = clamp01(py - (double)bottom);

    double valueTop    = lerp(edgeValues[left][top].uy,   edgeValues[right][top].uy,   xFrac);
    double valueBottom = lerp(edgeValues[left][bottom].uy, edgeValues[right][bottom].uy, xFrac);
    return lerp(valueBottom, valueTop, yFrac);
}
static double FluidGrid_SampleBilinearEdgesHorizontal(
    Cell_t** edgeValues,
    int edgeCountX,
    int edgeCountY,
    double cellSize,
    Vector2 worldPos)
{
    double width  = (double)(edgeCountX - 1) * cellSize;
    double height = (double)(edgeCountY - 1) * cellSize;

    double px = (worldPos.x + width * 0.5f) / cellSize;  // [0, countX]
    double py = (worldPos.y + height * 0.5f) / cellSize; // [0, countY]

    int left   = clamp_int((int)px, 0, edgeCountX - 2);
    int bottom = clamp_int((int)py, 0, edgeCountY - 2);
    int right  = left + 1;
    int top    = bottom + 1;

    double xFrac = clamp01(px - (double)left);
    double yFrac = clamp01(py - (double)bottom);

    double valueTop    = lerp(edgeValues[left][top].ux,   edgeValues[right][top].ux,   xFrac);
    double valueBottom = lerp(edgeValues[left][bottom].ux, edgeValues[right][bottom].ux, xFrac);
    return lerp(valueBottom, valueTop, yFrac);
}

static Vector2 FluidGrid_GetVelocityAtWorldPos(const SimState_t* g, Vector2 worldPos) {
    int vxWidth  = g->nx + 1;
    int vxHeight = g->ny;
    int vyWidth  = g->nx;
    int vyHeight = g->ny + 1;

    double velX = FluidGrid_SampleBilinearEdgesHorizontal(
        g->CellBufs_Arr[g->cellBufInUse], vxWidth, vxHeight, g->w, worldPos);
    double velY = FluidGrid_SampleBilinearEdgesVertical(
        g->CellBufs_Arr[g->cellBufInUse], vyWidth, vyHeight, g->w, worldPos);

    Vector2 v = { velX, velY };
    return v;
}



sim_err_t AdvectVelocity(SimState_t* g) {
    double dt = g->dt;

    int vxWidth  = g->nx ;
    int vxHeight = g->ny;
    int vyWidth  = g->nx ;
    int vyHeight = g->ny;
    Cell_t** currCells = GetCellsInUse(g);
    Cell_t** nextCell = GetCellNextInUse(g);
    // Horizontal
    for (int x = 0; x < vxWidth; x++) {
        for (int y = 0; y < vxHeight; y++) {
            if (FluidGrid_IsSolid(g, x - 1, y) || FluidGrid_IsSolid(g, x, y)) {
                nextCell[x][y].ux = currCells[x][y].ux;
                continue;
            }

            Vector2 pos    = FluidGrid_LeftEdgeCentre(g, x, y);
            Vector2 vel    = FluidGrid_GetVelocityAtWorldPos(g, pos);
            Vector2 posPrev = vec2_sub(pos, vec2_scale(vel, dt));
            //Vector newUx
            double newUX = FluidGrid_GetVelocityAtWorldPos(g, posPrev).x;
            nextCell[x][y].ux = newUX;
        }
    }

    // Vertical
    for (int x = 0; x < vyWidth; x++) {
        for (int y = 0; y < vyHeight; y++) {
            if (FluidGrid_IsSolid(g, x, y - 1) || FluidGrid_IsSolid(g, x, y)) {
                nextCell[x][y].uy = currCells[x][y].uy;
                continue;
            }

            Vector2 pos     = FluidGrid_BottomEdgeCentre(g, x, y);
            Vector2 vel     = FluidGrid_GetVelocityAtWorldPos(g, pos);
            Vector2 posPrev = vec2_sub(pos, vec2_scale(vel, dt));

            double newUY = FluidGrid_GetVelocityAtWorldPos(g, posPrev).y;
            nextCell[x][y].uy = newUY;
        }
    }

    Sim_State_SwapCellsInUse(g);
    return SIM_SUCCESS;
   // FluidGrid_UpdateVelocitiesFromTemporary(g);
}
#include "Advection.h"
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>



// Clamp a value between min and max
double clamp(double value, double minVal, double maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// Clamp a value between 0 and 1
double clamp01(double value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

// Linear interpolation: mix a and b by t
double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

// public Vector2 LeftEdgeCentre(int x, int y)
// {
//     float worldX = -Width / 2 + x * CellSize;
//     float worldY = -Height / 2 + y * CellSize + CellSize * 0.5f;

//     return new Vector2(worldX, worldY);
// }

// public Vector2 BottomEdgeCentre(int x, int y)
// {
//     float worldX = -Width  / 2 + x * CellSize + CellSize * 0.5f;   // horizontal midpoint
//     float worldY = -Height / 2 + y * CellSize;                     // bottom edge

//     return new Vector2(worldX, worldY);
// }


double InterpolateVelocityX(SimState_t* sim_state, double world_pos_x, double world_pos_y){
    uint nx = sim_state->nx;
    uint ny = sim_state->ny;
    uint cell_size = sim_state->w;

    uint width = (nx - 1) * cell_size;
    uint height = (ny - 1) * cell_size;

    double posx = ((world_pos_x + width) / 2) * cell_size;
    double posy = ((world_pos_y + height)/ 2) * cell_size;

    uint left = clamp(posx, 0, nx - 2);
    uint bottom = clamp(posy, 0, ny - 2);
    uint right = left + 1;
    uint top = bottom + 1;

    double x_fraction = clamp01(posx - left);
    double y_fraction = clamp01(posy - bottom);

    Cell_t** cells = sim_state->using_cells1 ? sim_state->cells1 : sim_state->cells2;

    double top_valueX = lerp(cells[left][top].ux, cells[right][top].ux, x_fraction);
    double bottom_valueX = lerp(cells[left][bottom].ux, cells[right][bottom].ux, x_fraction);
    double newX_velocity = lerp(bottom_valueX, top_valueX, y_fraction);

    return newX_velocity;
}

double InterpolateVelocityY(SimState_t* sim_state, double world_pos_x, double world_pos_y){
    uint nx = sim_state->nx;
    uint ny = sim_state->ny;
    uint cell_size = sim_state->w;

    uint width = (nx - 1) * cell_size;
    uint height = (ny - 1) * cell_size;

    double posx = ((world_pos_x + width) / 2) * cell_size;
    double posy = ((world_pos_y + height)/ 2) * cell_size;

    uint left = clamp(posx, 0, nx - 2);
    uint bottom = clamp(posy, 0, ny - 2);
    uint right = left + 1;
    uint top = bottom + 1;

    double x_fraction = clamp01(posx - left);
    double y_fraction = clamp01(posy - bottom);

    Cell_t** cells = sim_state->using_cells1 ? sim_state->cells1 : sim_state->cells2;

    double top_valueY = lerp(cells[left][top].uy, cells[right][top].uy, x_fraction);
    double bottom_valueY = lerp(cells[left][bottom].uy, cells[right][bottom].uy, x_fraction);
    double newX_velocitY = lerp(bottom_valueY, top_valueY, y_fraction);

    return newX_velocitY;
}


sim_err_t AdvectVelocity(SimState_t* state){
    double cell_width = (state->nx -1) * state->w;
    double cell_height = (state->ny - 1) * state->w;
    double cell_size = state->w;
    Cell_t** new_cell = state->using_cells1 ? state->cells2 : state->cells1;


    for(uint i = 0; i < state->nx; i++){
        for(uint j = 0; j < state->ny; j++){
            //get world position x velocity
            double horizontal_worldX = cell_width / 2 + i * cell_size;
            double horizontal_worldY = cell_height / 2 + j * cell_size + cell_size/2;
            //Biliner Interpol x velocity
            double horizontal_velX = InterpolateVelocityX(state,horizontal_worldX,horizontal_worldY);
            double horizontal_velY = InterpolateVelocityY(state,horizontal_worldX, horizontal_worldY);
            // calculate where velociy came from
            double prev_horizontal_worldX = horizontal_worldX - horizontal_velX * state->dt;
            double prev_horizontal_worldY = horizontal_worldY - horizontal_velY * state->dt;
            //Biliner Interpol the previous velocity as the new velocity 
            new_cell[i][j].ux = InterpolateVelocityX(state,prev_horizontal_worldX, prev_horizontal_worldY);


            //get world position y velocity
            double vertical_worldX = cell_width/2 + i*cell_size + cell_size/2;
            double vertical_worldY = cell_height/2 + j*cell_size;
            //Biliner Interpol y
            double vertical_velX = InterpolateVelocityX(state,vertical_worldX,vertical_worldY);
            double vertical_velY = InterpolateVelocityY(state,vertical_worldX,vertical_worldY);
            // calculate prev
            double prev_vertical_worldX = vertical_worldX - vertical_velX * state->dt;
            double prev_vertical_worldY = vertical_worldY - vertical_velY * state->dt;
            //Biliner interpol prev velocity as new 
            new_cell[i][j].uy = InterpolateVelocityY(state,prev_vertical_worldX, prev_vertical_worldY);            
        }
    }
        state->using_cells1 = !state->using_cells1;
    return SIM_SUCCESS;
}

// }

// void advenction(SimState_t* sim){

//     // int xtemp[sim->nx][sim->ny];

//     // int ytemp[sim->nx][sim->ny];

//     // for(int i = 0; i < sim->nx; i++){
//     //     for(int j = 0; j < sim->ny; j++){

//     //         int xposition = i * sim->w;
//     //         int yposition = j * sim->w;
//     //         int pos = xposition +yposition;

//     //         int xvel = sim->cells[i][j].u;
//     //         int yvel = sim->cells[i][j].v;
//     //         int vel = xvel + yvel;

//     //         int prevPos = pos - vel * sim->dt;





        
//     //     }
//     // }
// }


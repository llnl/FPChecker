#include <iostream>
using namespace std;

typedef float Real_t;

FPC_CALCULATE_ERROR

void compute_motion(float start, float end, float initial_velocity, int time, float &velocity, float &acceleration) {
    velocity = (start - end) / time;
    acceleration = (velocity - initial_velocity) / time;
}

int main(int argc, char* argv[]) {
    float initial_position = atof(argv[1]);
    float final_position   = atof(argv[2]);
    float initial_velocity = atof(argv[3]);
    int  time = atoi(argv[4]);
    float velocity, acceleration;

    compute_motion(initial_position, final_position, initial_velocity, time, velocity, acceleration);
    printf("Velocity      = %.7f\n", velocity);
    printf("Acceleration  = %.7f\n", acceleration);

    return 0;
}
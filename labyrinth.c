#include "labyrinth.h"
#include "common.h"

bool testVerticalWall(Labyrinth* labyrinth, int i, int j) {
    return labyrinth->verticalWalls[i][j];
}
bool testHorizontalWall(Labyrinth* labyrinth, int i, int j) {
    return labyrinth->horizontalWalls[j][i];
}
void Labyrinth_random(Labyrinth* labyrinth) {
    for (int i = 0; i < WIDTH; ++i) {
        for (int j = 0; j < HEIGHT; ++j) {
            labyrinth->verticalWalls[i][j] = (rand() % 3 == 0);
            labyrinth->horizontalWalls[j][i] = (rand() % 3 == 0);
        }
    }
}

bool isOutOfBounds(double x, double y) {
    return (x < 0.0 || x > (double) WIDTH || y < 0.0 || y > (double) HEIGHT);
}
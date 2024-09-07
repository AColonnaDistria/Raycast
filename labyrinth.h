#ifndef LABYRINTH_H
#define LABYRINTH_H

#include "common.h"

#define WIDTH 25
#define HEIGHT 25

typedef struct {
    bool verticalWalls[WIDTH][HEIGHT];
    bool horizontalWalls[HEIGHT][WIDTH];
} Labyrinth;

typedef struct {
    double x;
    double y;
    double angle;
} PlayerPosition;

bool testVerticalWall(Labyrinth* labyrinth, int i, int j);
bool testHorizontalWall(Labyrinth* labyrinth, int i, int j);
bool isOutOfBounds(double x, double y);
void Labyrinth_random(Labyrinth* labyrinth);

#endif
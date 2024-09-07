#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "common.h"
#include "labyrinth.h"

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

#define WALL_THICKNESS 0.05

#define PLAYER_POINT_SIZE 4
#define BUMP_POINT_SIZE 8

#define BUMP_MAX_COUNT 16

#define FOV (RAYCAST_PI / 3)
#define NUMBER_OF_RAYS 128

#define REAL_WALL_HEIGHT 2.0

#define FPS 30
#define FRAME_TARGET_TIME (1000 / FPS)

static double factor_camera;

static SDL_Texture* wallTexture;

typedef enum {
    INITIALIZE_WINDOW_OK = 0,
    INITIALIZE_WINDOW_ERORR = 1,
} InitializeWindowResult;

typedef struct {
    double x;
    double y;

    double x2;
    double y2;

    double distance;

    bool verticalWall;
} Ray;

typedef struct {
    bool isGameRunning;
    int lastFrameTime;

    Labyrinth labyrinth;

    SDL_Rect verticalWalls[WIDTH][HEIGHT];
    SDL_Rect horizontalWalls[HEIGHT][WIDTH];

    PlayerPosition playerPosition;
    Ray rayPosition[NUMBER_OF_RAYS];

    //double xRayBumps[BUMP_MAX_COUNT];
    //double yRayBumps[BUMP_MAX_COUNT];

    SDL_Rect playerRect;

    //SDL_Rect bumpsRect[BUMP_MAX_COUNT];
    int bumpsCount;

    bool shouldRefresh;

    SDL_Window* window;
    SDL_Renderer* renderer;
} GameState;

static void report_error(const char* message);
static InitializeWindowResult initialize_window();
static void destroy_window();
static void process_input();
static void setup();
static void update();
static void render();

static GameState gameState = {
    .isGameRunning = false,
    .window = NULL,
    .renderer = NULL,
    .shouldRefresh = true
};

static void report_error(const char* message) {
    fprintf(stderr, "%s", message);
}

static InitializeWindowResult initialize_window() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        report_error(SDL_GetError());
        return INITIALIZE_WINDOW_ERORR;
    }

    gameState.window = SDL_CreateWindow(
        "",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH * 2, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (gameState.window == NULL) {
        report_error(SDL_GetError());
        return INITIALIZE_WINDOW_ERORR;
    }

    gameState.renderer = SDL_CreateRenderer(gameState.window, -1, 0);
    if (gameState.renderer == NULL) {
        report_error(SDL_GetError());
        return INITIALIZE_WINDOW_ERORR;
    }

    return INITIALIZE_WINDOW_OK;
}

static void destroy_window() {
    SDL_DestroyRenderer(gameState.renderer);
    SDL_DestroyWindow(gameState.window);
    SDL_Quit();
}

static void setup() {
    // Create a labyrinth

    Labyrinth_random(&gameState.labyrinth);

    // Create rects associated to labyrinth
    for (int i = 0; i < WIDTH; ++i) {
        for (int j = 0; j < HEIGHT; ++j) {
            int index = j * WIDTH + i;

            SDL_Rect verticalWall = {
                .x = (WINDOW_WIDTH / WIDTH) * i,
                .y = (WINDOW_HEIGHT / HEIGHT) * j,
                .w = (int) (WALL_THICKNESS / (double) WIDTH *(double)WINDOW_WIDTH),
                .h = WINDOW_HEIGHT / HEIGHT
            };
            SDL_Rect horizontalWall = {
                .x = (WINDOW_WIDTH / WIDTH) * i,
                .y = (WINDOW_HEIGHT / HEIGHT) * j,
                .w = WINDOW_WIDTH / WIDTH,
                .h = (int) (WALL_THICKNESS / (double) HEIGHT *(double)WINDOW_WIDTH),
            };

            gameState.verticalWalls[i][j] = verticalWall;
            gameState.horizontalWalls[j][i] = horizontalWall;
        }
    }

    // Create player
    gameState.playerPosition.x = WIDTH * 0.5 + 0.5;
    gameState.playerPosition.y = HEIGHT * 0.5 + 0.5;
    gameState.playerPosition.angle = RAYCAST_PI;

    gameState.playerRect.x = (gameState.playerPosition.x) * ((double) (WINDOW_WIDTH / WIDTH)) - PLAYER_POINT_SIZE * 0.5;
    gameState.playerRect.y = (gameState.playerPosition.y) * ((double) (WINDOW_HEIGHT / HEIGHT)) - PLAYER_POINT_SIZE * 0.5;
    gameState.playerRect.w = PLAYER_POINT_SIZE;
    gameState.playerRect.h = PLAYER_POINT_SIZE;

    factor_camera = REAL_WALL_HEIGHT * ((double) WINDOW_WIDTH * 0.5) / tan(FOV * 0.5);

    wallTexture = IMG_LoadTexture(gameState.renderer, "./textures/wall.png");
}

static void process_input() {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_QUIT:
            gameState.isGameRunning = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                gameState.isGameRunning = false;
            
            int x_wall = (int) gameState.playerPosition.x;
            int y_wall = (int) gameState.playerPosition.y;

            int index = x_wall + y_wall * WIDTH;
            int index2 = (x_wall + 1) + (y_wall + 1) * WIDTH;

            if (event.key.keysym.sym == SDLK_z) {
                gameState.playerPosition.x += 0.05 * cos(gameState.playerPosition.angle); 
                gameState.playerPosition.y -= 0.05 * sin(gameState.playerPosition.angle);

                if (isOutOfBounds(gameState.playerPosition.x, gameState.playerPosition.y)) {
                    // undo
                    gameState.playerPosition.x -= 0.05 * cos(gameState.playerPosition.angle); 
                    gameState.playerPosition.y += 0.05 * sin(gameState.playerPosition.angle);
                }

                gameState.shouldRefresh = true;
            }
            if (event.key.keysym.sym == SDLK_s) {
                gameState.playerPosition.x -= 0.05 * cos(gameState.playerPosition.angle); 
                gameState.playerPosition.y += 0.05 * sin(gameState.playerPosition.angle);

                if (isOutOfBounds(gameState.playerPosition.x, gameState.playerPosition.y)) {
                    // undo
                    gameState.playerPosition.x += 0.05 * cos(gameState.playerPosition.angle); 
                    gameState.playerPosition.y -= 0.05 * sin(gameState.playerPosition.angle);
                }

                gameState.shouldRefresh = true;
            }
            if (event.key.keysym.sym == SDLK_q) {
                gameState.playerPosition.x += 0.05 * cos(gameState.playerPosition.angle + RAYCAST_PI / 2); 
                gameState.playerPosition.y += 0.05 * sin(gameState.playerPosition.angle + RAYCAST_PI / 2);

                if (isOutOfBounds(gameState.playerPosition.x, gameState.playerPosition.y)) {
                    // undo
                    gameState.playerPosition.x -= 0.05 * cos(gameState.playerPosition.angle + RAYCAST_PI / 2); 
                    gameState.playerPosition.y -= 0.05 * sin(gameState.playerPosition.angle + RAYCAST_PI / 2); 
                }

                gameState.shouldRefresh = true;
            }
            if (event.key.keysym.sym == SDLK_d) {
                gameState.playerPosition.x += 0.05 * cos(gameState.playerPosition.angle - RAYCAST_PI / 2); 
                gameState.playerPosition.y += 0.05 * sin(gameState.playerPosition.angle - RAYCAST_PI / 2);

                if (isOutOfBounds(gameState.playerPosition.x, gameState.playerPosition.y)) {
                    // undo
                    gameState.playerPosition.x -= 0.05 * cos(gameState.playerPosition.angle - RAYCAST_PI / 2); 
                    gameState.playerPosition.y -= 0.05 * sin(gameState.playerPosition.angle - RAYCAST_PI / 2); 
                }

                gameState.shouldRefresh = true;
            }
            if (event.key.keysym.sym == SDLK_w) {
                gameState.playerPosition.angle += 0.05;

                gameState.shouldRefresh = true;
            }
            if (event.key.keysym.sym == SDLK_x) {
                gameState.playerPosition.angle -= 0.05;

                gameState.shouldRefresh = true;
            }



            break;
    }
}

static int sign(double x) {
    if (x < 0.0) {
        return -1;
    }
    else {
        return +1;
    }
}

/*
typedef struct {
    double x;
    double y;

    int i;
    int j;
} Bump;
*/

static void swap(int* x, int* y) {
    int swapTemp = *y;

    *x = *y;
    *y = swapTemp;
}

static bool hitWall_X(int x_i, int y_i) {
    if (x_i < 0 || x_i >= WIDTH || y_i < 0 || y_i >= HEIGHT)
        return true;

    return testVerticalWall(&gameState.labyrinth, x_i, y_i);
}

static bool hitWall_Y(int x_i, int y_i) {
    if (x_i < 0 || x_i >= WIDTH || y_i < 0 || y_i >= HEIGHT)
        return true;

    return testHorizontalWall(&gameState.labyrinth, x_i, y_i);
}

static void computeRay(Ray* ray, double x_ray_ini, double y_ray_ini, double angle, double beta) {
    // starts from start
    //double x_ray_ini = gameState.playerPosition.x;
    //double y_ray_ini = gameState.playerPosition.y;

    //double angle = gameState.playerPosition.angle;

    int sign_x = sign(cos(angle));
    int sign_y = -sign(sin(angle));

    //Bump bumps[BUMP_MAX_COUNT];
    double last_bump_x;
    double last_bump_y;

    int index = 0;

    int range_x_start = (int) x_ray_ini + (sign_x == 1 ? 1 : 0);
    int range_x_end = range_x_start + sign_x * BUMP_MAX_COUNT;

    int y_i_last = (int) y_ray_ini;
    bool hitWall = false;

    bool isVertical = false;

    for (int x_i = range_x_start; (x_i * sign_x <= range_x_end * sign_x) && (index < BUMP_MAX_COUNT) && !hitWall; x_i += sign_x) {
        double x_ray = x_i;
        double y_ray = (x_ray_ini - x_ray) * tan(angle) + y_ray_ini;
        int y_i = (int) y_ray;

        if (y_i != y_i_last) {
            int range_y_start = y_i_last + (sign_y == 1 ? 1 : 0);
            int range_y_end = y_i + (sign_y == -1 ? 1 : 0);

            for (int y_i_2 = range_y_start; (y_i_2 * sign_y <= range_y_end * sign_y) && (index < BUMP_MAX_COUNT) && !hitWall; y_i_2 += sign_y) {
                double y_ray_2 = y_i_2;
                double x_ray_2 = -(y_ray_2 - y_ray_ini) / tan(angle) + x_ray_ini;

                //bumps[index].x = x_ray_2;
                //bumps[index].y = y_ray_2;
                //bumps[index].i = (int) x_ray_2;
                //bumps[index].j = y_i_2;

                last_bump_x = x_ray_2;
                last_bump_y = y_ray_2;

                isVertical = false;

                if (hitWall_Y((int) x_ray_2, y_i_2))
                    hitWall = true;
                
                ++index;
            }
        }

        if (index < BUMP_MAX_COUNT && !hitWall) {
            //bumps[index].x = x_ray;
            //bumps[index].y = y_ray;
            //bumps[index].i = x_i;
            //bumps[index].j = y_i;

            last_bump_x = x_ray;
            last_bump_y = y_ray;

            y_i_last = y_i;

            if (hitWall_X(x_i, y_i))
                hitWall = true;
            
            isVertical = true;

            ++index;
        }
    }

    //for (int i = 0; i < index; ++i) {
    //    gameState.xRayBumps[i] = bumps[i].x;
    //    gameState.yRayBumps[i] = bumps[i].y;
    //}

    gameState.bumpsCount = index;

    ray->x = x_ray_ini;
    ray->y = y_ray_ini;
    ray->x2 = last_bump_x;
    ray->y2 = last_bump_y;

    ray->distance = sqrt((ray->x2 - ray->x) * (ray->x2 - ray->x) + (ray->y2 - ray->y) * (ray->y2 - ray->y)) * cos(beta);
    ray->verticalWall = isVertical;
}

static void update() {
    //double deltaTime = (SDL_GetTicks() - gameState.lastFrameTime) / 1000.0;
    //gameState.lastFrameTime = SDL_GetTicks();

    gameState.playerRect.x = (gameState.playerPosition.x) * ((double) (WINDOW_WIDTH / WIDTH)) - PLAYER_POINT_SIZE * 0.5;
    gameState.playerRect.y = (gameState.playerPosition.y) * ((double) (WINDOW_HEIGHT / HEIGHT)) - PLAYER_POINT_SIZE * 0.5;
    gameState.playerRect.w = PLAYER_POINT_SIZE;
    gameState.playerRect.h = PLAYER_POINT_SIZE;

    /*
    for (int i = 0; i < gameState.bumpsCount && i < BUMP_MAX_COUNT; ++i) {
        gameState.bumpsRect[i].x = gameState.xRayBumps[i] * ((double) (WINDOW_WIDTH / WIDTH));
        gameState.bumpsRect[i].y = gameState.yRayBumps[i] * ((double) (WINDOW_HEIGHT / HEIGHT));
        gameState.bumpsRect[i].w = BUMP_POINT_SIZE;
        gameState.bumpsRect[i].h = BUMP_POINT_SIZE;
    }
    */
    
    // compute ray
    for (int i = 0; i < NUMBER_OF_RAYS; ++i) {
        double angle = (gameState.playerPosition.angle - FOV * 0.5) + FOV * ((double) i / (double) NUMBER_OF_RAYS);
    
        computeRay(&gameState.rayPosition[i], gameState.playerPosition.x, gameState.playerPosition.y, angle, (angle - gameState.playerPosition.angle));
    }
}

static void render() {
    SDL_SetRenderDrawColor(gameState.renderer, 0, 0, 0, 255);
    SDL_RenderClear(gameState.renderer);

    SDL_SetRenderDrawColor(gameState.renderer, 255, 255, 255, 255);
    for (int i = 0; i < WIDTH; ++i) {
        for (int j = 0; j < HEIGHT; ++j) {
            if (gameState.labyrinth.verticalWalls[i][j])
                SDL_RenderFillRect(gameState.renderer, &gameState.verticalWalls[i][j]);

            if  (gameState.labyrinth.horizontalWalls[i][j])
                SDL_RenderFillRect(gameState.renderer, &gameState.horizontalWalls[i][j]);
        }
    }

    SDL_SetRenderDrawColor(gameState.renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(gameState.renderer, &gameState.playerRect);

    SDL_SetRenderDrawColor(gameState.renderer, 0, 255, 0, 255);

    for (int i = 0; i < NUMBER_OF_RAYS; ++i) {
        SDL_SetRenderDrawColor(gameState.renderer, 0, 255, 0, 255);
        SDL_RenderDrawLine(gameState.renderer, 
                        gameState.rayPosition[i].x / (double)WIDTH * (double)WINDOW_WIDTH, gameState.rayPosition[i].y / (double)HEIGHT * (double)WINDOW_HEIGHT, 
                        gameState.rayPosition[i].x2 / (double)WIDTH * (double)WINDOW_WIDTH, gameState.rayPosition[i].y2 / (double)HEIGHT * WINDOW_HEIGHT);

        int sliceHeight = factor_camera / gameState.rayPosition[i].distance;
        int sliceWidth = (1.0 / ((double) NUMBER_OF_RAYS) * (double) WINDOW_WIDTH);

        //if (sliceHeight > WINDOW_HEIGHT) {
        //    sliceHeight = WINDOW_HEIGHT;
        //}

        double x;
        if (gameState.rayPosition[i].verticalWall) {
            x = (gameState.rayPosition[i].y2 - floor(gameState.rayPosition[i].y2)) * 510.0;
        }
        else {
            x = (gameState.rayPosition[i].x2 - floor(gameState.rayPosition[i].x2)) * 510.0;
        }

        SDL_Rect srcRect = {
            .x = (int) (round(x)),
            .y = 0,
            .w = 1,
            .h = 510,
        };
        SDL_Rect destRect = {
            .x = WINDOW_WIDTH + i * sliceWidth,
            .y = WINDOW_HEIGHT / 2 - sliceHeight / 2,
            .w = sliceWidth,
            .h = sliceHeight,
        };

        double luminosity = ((double) sliceHeight > (double) WINDOW_HEIGHT ? (double) WINDOW_HEIGHT : (double) sliceHeight) / ((double) WINDOW_HEIGHT);

        SDL_SetTextureColorMod(wallTexture, 255.0 * luminosity, 255.0 * luminosity, 255.0 * luminosity);
        SDL_RenderCopy(gameState.renderer, wallTexture, &srcRect, &destRect);    
    }   
    SDL_RenderPresent(gameState.renderer);
}

// Game loop
int main(int argc, char** argv) {
    srand(time(NULL));

    gameState.isGameRunning = (initialize_window() == INITIALIZE_WINDOW_OK);

    setup();
    while (gameState.isGameRunning) {
        process_input();
        if (gameState.shouldRefresh) {
            update();
        }
        render();

        gameState.shouldRefresh = false;
        SDL_Delay(1);
    }

    destroy_window();
    return 0;
}
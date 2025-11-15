#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROCK 1
#define RABBIT 2
#define FOX 3

typedef struct {
    int type;   // ROCK, RABBIT, FOX, or 0
    int age;    // generations since last procreation
    int hunger; // generations since last meal (for foxes)
    int moved;  // flag for conflict resolution
} Cell;

typedef struct {
    int GEN_PROC_RABBITS;
    int GEN_PROC_FOXES;
    int GEN_FOOD_FOXES;
    int N_GEN;
    int R;
    int C;
    int N;
} Config;

typedef struct {
    int x, y;
} Position;

int dx[4] = {-1, 0, 1, 0}; // N, E, S, W
int dy[4] = {0, 1, 0, -1};

Cell **allocate_grid(int R, int C) {
    Cell **grid = (Cell **)malloc(R * sizeof(Cell *));
    for (int i = 0; i < R; i++) {
        grid[i] = (Cell *)malloc(C * sizeof(Cell));
        for (int j = 0; j < C; j++) {
            grid[i][j].type = 0;
            grid[i][j].age = 0;
            grid[i][j].hunger = 0;
            grid[i][j].moved = 0;
        }
    }
    return grid;
}

void free_grid(Cell **grid, int R) {
    for (int i = 0; i < R; i++) free(grid[i]);
    free(grid);
}

int is_valid(Config *conf, int x, int y) {
    return x >= 0 && x < conf->R && y >= 0 && y < conf->C;
}

// Count empty neighbors for rabbits or fox movement
int get_neighbors(Cell **grid, Config *conf, int x, int y, Position *pos, int target_type) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (is_valid(conf, nx, ny)) {
            if (target_type == 0 && grid[nx][ny].type == 0) {
                pos[count].x = nx;
                pos[count].y = ny;
                count++;
            }
            else if (target_type == RABBIT && grid[nx][ny].type == RABBIT) {
                pos[count].x = nx;
                pos[count].y = ny;
                count++;
            }
        }
    }
    return count;
}

// Conflict resolution helper
void try_move(Cell **next, int nx, int ny, Cell c) {
    if (next[nx][ny].type == 0) {
        next[nx][ny] = c;
    } else if (c.type == RABBIT && next[nx][ny].type == RABBIT) {
        // Keep the older rabbit
        if (c.age > next[nx][ny].age) next[nx][ny] = c;
    } else if (c.type == FOX && next[nx][ny].type == FOX) {
        if (c.age > next[nx][ny].age) next[nx][ny] = c;
        else if (c.age == next[nx][ny].age && c.hunger < next[nx][ny].hunger) next[nx][ny] = c;
    }
}

void move_rabbits(Cell **grid, Cell **next, Config *conf, int generation) {
    // Copy rocks first
    for (int i = 0; i < conf->R; i++)
        for (int j = 0; j < conf->C; j++)
            if (grid[i][j].type == ROCK)
                next[i][j] = grid[i][j];

    for (int i = 0; i < conf->R; i++) {
        for (int j = 0; j < conf->C; j++) {
            if (grid[i][j].type == RABBIT && !grid[i][j].moved) {
                Position pos[4];
                int count = get_neighbors(grid, conf, i, j, pos, 0);
                Cell c = grid[i][j];
                c.age++;
                if (count > 0) {
                    int idx = (generation + i + j) % count;
                    try_move(next, pos[idx].x, pos[idx].y, c);

                    // Procreation
                    if (c.age >= conf->GEN_PROC_RABBITS) {
                        Cell baby = {RABBIT, 0, 0, 1};
                        try_move(next, i, j, baby);
                        next[pos[idx].x][pos[idx].y].age = 0;
                    }
                } else {
                    try_move(next, i, j, c);
                }
            }
        }
    }
}

void move_foxes(Cell **grid, Cell **next, Config *conf, int generation) {
    // Copy rocks first
    for (int i = 0; i < conf->R; i++)
        for (int j = 0; j < conf->C; j++)
            if (grid[i][j].type == ROCK)
                next[i][j] = grid[i][j];

    for (int i = 0; i < conf->R; i++) {
        for (int j = 0; j < conf->C; j++) {
            if (grid[i][j].type == FOX && !grid[i][j].moved) {
                Cell c = grid[i][j];
                c.age++;
                c.hunger++;

                Position rabbit_pos[4];
                int r_count = get_neighbors(grid, conf, i, j, rabbit_pos, RABBIT);
                if (r_count > 0) {
                    int idx = (generation + i + j) % r_count;
                    c.hunger = 0; // ate a rabbit
                    try_move(next, rabbit_pos[idx].x, rabbit_pos[idx].y, c);

                    // Procreation
                    if (c.age >= conf->GEN_PROC_FOXES) {
                        Cell baby = {FOX, 0, 0, 1};
                        try_move(next, i, j, baby);
                        next[rabbit_pos[idx].x][rabbit_pos[idx].y].age = 0;
                    }
                } else {
                    Position empty_pos[4];
                    int e_count = get_neighbors(grid, conf, i, j, empty_pos, 0);
                    if (e_count > 0) {
                        int idx = (generation + i + j) % e_count;
                        try_move(next, empty_pos[idx].x, empty_pos[idx].y, c);
                    } else {
                        try_move(next, i, j, c);
                    }
                }

                // Starvation
                for (int x = 0; x < conf->R; x++)
                    for (int y = 0; y < conf->C; y++)
                        if (next[x][y].type == FOX && next[x][y].hunger >= conf->GEN_FOOD_FOXES)
                            next[x][y].type = 0;
            }
        }
    }
}

void print_grid(Cell **grid, Config *conf) {
    int count = 0;
    for (int i = 0; i < conf->R; i++)
        for (int j = 0; j < conf->C; j++)
            if (grid[i][j].type != 0)
                count++;
    printf("%d %d %d %d %d %d %d\n", conf->GEN_PROC_RABBITS, conf->GEN_PROC_FOXES,
           conf->GEN_FOOD_FOXES, 0, conf->R, conf->C, count);
    for (int i = 0; i < conf->R; i++)
        for (int j = 0; j < conf->C; j++) {
            if (grid[i][j].type != 0) {
                if (grid[i][j].type == ROCK) printf("ROCK %d %d\n", i, j);
                else if (grid[i][j].type == RABBIT) printf("RABBIT %d %d\n", i, j);
                else if (grid[i][j].type == FOX) printf("FOX %d %d\n", i, j);
            }
        }
}

int main() {
    Config conf;
    scanf("%d %d %d %d %d %d %d",
          &conf.GEN_PROC_RABBITS, &conf.GEN_PROC_FOXES,
          &conf.GEN_FOOD_FOXES, &conf.N_GEN,
          &conf.R, &conf.C, &conf.N);

    Cell **grid = allocate_grid(conf.R, conf.C);
    Cell **next = allocate_grid(conf.R, conf.C);

    for (int i = 0; i < conf.N; i++) {
        char obj[10];
        int x, y;
        scanf("%s %d %d", obj, &x, &y);
        if (strcmp(obj, "ROCK") == 0) grid[x][y].type = ROCK;
        else if (strcmp(obj, "RABBIT") == 0) grid[x][y].type = RABBIT;
        else if (strcmp(obj, "FOX") == 0) grid[x][y].type = FOX;
    }

    for (int gen = 0; gen < conf.N_GEN; gen++) {
        // Clear next grid
        for (int i = 0; i < conf.R; i++)
            for (int j = 0; j < conf.C; j++)
                next[i][j].type = 0;

        move_rabbits(grid, next, &conf, gen);

        Cell **tmp = grid;
        grid = next;
        next = tmp;

        // Clear next for foxes
        for (int i = 0; i < conf.R; i++)
            for (int j = 0; j < conf.C; j++)
                next[i][j].type = 0;

        move_foxes(grid, next, &conf, gen);

        tmp = grid;
        grid = next;
        next = tmp;
    }

    print_grid(grid, &conf);

    free_grid(grid, conf.R);
    free_grid(next, conf.R);
    return 0;
}

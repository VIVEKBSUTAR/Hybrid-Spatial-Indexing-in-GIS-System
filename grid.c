#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "grid.h"
#include "octree.h"

/* ── Global definitions (declared extern in types.h) ─────────────── */
GridCell3D    grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
SpatialObject all_objects[MAX_TOTAL_OBJECTS];
int           total_objects = 0;
long          visited_nodes = 0;

/* ── Grid helpers ─────────────────────────────────────────────────── */

static double cellWidth(void)  { return WORLD_SIZE / GRID_SIZE; }
static double cellHeight(void) { return WORLD_SIZE / GRID_SIZE; }
static double cellDepth(void)  { return WORLD_SIZE / GRID_SIZE; }

static void clampCell3D(int* cx, int* cy, int* cz) {
    if (*cx < 0)          *cx = 0;
    if (*cx >= GRID_SIZE) *cx = GRID_SIZE - 1;
    if (*cy < 0)          *cy = 0;
    if (*cy >= GRID_SIZE) *cy = GRID_SIZE - 1;
    if (*cz < 0)          *cz = 0;
    if (*cz >= GRID_SIZE) *cz = GRID_SIZE - 1;
}

/* ── Initialise the 10x10x10 3D grid, one Octree root per cell ──── */

void initGrid(void) {
    double cw = cellWidth(), ch = cellHeight(), cd = cellDepth();
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            for (int k = 0; k < GRID_SIZE; k++) {
                BoundingBox3D b = {
                    i * cw, j * ch, k * cd,
                    (i + 1) * cw, (j + 1) * ch, (k + 1) * cd
                };
                grid[i][j][k].root = createOctreeNode(b);
            }
        }
    }
}

/* ── Insert one point into the correct grid cell's octree ────────── */

void insertIntoGrid(SpatialObject* obj) {
    int cx = (int)(obj->x / cellWidth());
    int cy = (int)(obj->y / cellHeight());
    int cz = (int)(obj->z / cellDepth());
    clampCell3D(&cx, &cy, &cz);
    insertOctree(grid[cx][cy][cz].root, obj, 0);
}

/* ── Load points from points.txt ────────────────────────────────── */

void loadData(void) {
    FILE* f = fopen("points.txt", "r");
    if (!f) return;
    double x, y, z;
    while (total_objects < MAX_TOTAL_OBJECTS) {
        /* Try to read 3 values (3D format) */
        int result = fscanf(f, "%lf %lf %lf", &x, &y, &z);
        if (result == 3) {
            /* 3D format: x y z */
            all_objects[total_objects].id = total_objects + 1;
            all_objects[total_objects].x  = x;
            all_objects[total_objects].y  = y;
            all_objects[total_objects].z  = z;
            insertIntoGrid(&all_objects[total_objects]);
            total_objects++;
        } else if (result == 2) {
            /* Old 2D format: x y (fallback z to 0) */
            all_objects[total_objects].id = total_objects + 1;
            all_objects[total_objects].x  = x;
            all_objects[total_objects].y  = y;
            all_objects[total_objects].z  = 0.0;
            insertIntoGrid(&all_objects[total_objects]);
            total_objects++;
        } else {
            break; /* EOF or parse error */
        }
    }
    fclose(f);
}

/* ── Generate N random points and write to points.txt ────────────── */

void generateData(int n) {
    if (n <= 0 || n > MAX_TOTAL_OBJECTS) {
        fprintf(stderr, "N must be between 1 and %d\n", MAX_TOTAL_OBJECTS);
        return;
    }
    srand((unsigned int)time(NULL));
    FILE* f = fopen("points.txt", "w");
    if (!f) { fprintf(stderr, "Cannot open points.txt\n"); return; }
    for (int i = 0; i < n; i++) {
        double x = ((double)rand() / RAND_MAX) * WORLD_SIZE;
        double y = ((double)rand() / RAND_MAX) * WORLD_SIZE;
        double z = ((double)rand() / RAND_MAX) * WORLD_SIZE;
        fprintf(f, "%.6f %.6f %.6f\n", x, y, z);
    }
    fclose(f);
    fprintf(stderr, "Generated %d points into points.txt\n", n);
}

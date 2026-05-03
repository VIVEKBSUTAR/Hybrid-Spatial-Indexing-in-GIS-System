#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define MAX_OBJECTS       8          /* octree holds more per leaf */
#define MAX_DEPTH         10
#define GRID_SIZE         10         /* 10×10×10 grid */
#define WORLD_SIZE        1000.0
#define MAX_TOTAL_OBJECTS 100000

typedef struct {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;
} BoundingBox3D;

typedef struct {
    int    id;
    double x, y, z;                  /* 3-D point */
} SpatialObject;

/* 8-children octree node */
typedef struct OctreeNode {
    BoundingBox3D      boundary;
    SpatialObject*     objects[MAX_OBJECTS];
    int                object_count;
    struct OctreeNode* children[8];  /* 0=TNW 1=TNE 2=TSW 3=TSE 4=BNW 5=BNE 6=BSW 7=BSE */
    bool               is_divided;
} OctreeNode;

typedef struct {
    OctreeNode* root;
} GridCell3D;

/* Globals shared across modules */
extern GridCell3D    grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
extern SpatialObject all_objects[MAX_TOTAL_OBJECTS];
extern int           total_objects;
extern long          visited_nodes;

#endif

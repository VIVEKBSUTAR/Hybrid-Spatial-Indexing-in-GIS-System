#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define MAX_OBJECTS       8
#define MAX_DEPTH         10
#define GRID_SIZE         10
#define WORLD_SIZE        1000.0
#define MAX_TOTAL_OBJECTS 100000

/* ───────── 3D (Octree) ───────── */

typedef struct {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;
} BoundingBox3D;

typedef struct {
    int id;
    double x, y, z;
} SpatialObject;

typedef struct OctreeNode {
    BoundingBox3D boundary;
    SpatialObject* objects[MAX_OBJECTS];
    int object_count;

    struct OctreeNode* children[8];   // 8 children for octree

    bool is_divided;
} OctreeNode;

typedef struct {
    OctreeNode* root;
} GridCell3D;

/* ───────── 2D (Quadtree) ───────── */

typedef struct {
    double minX, minY;
    double maxX, maxY;
} BoundingBox2D;

typedef struct {
    int id;
    double x, y;
} SpatialObject2D;

typedef struct QuadtreeNode {
    BoundingBox2D boundary;
    SpatialObject2D* objects[MAX_OBJECTS];
    int object_count;

    struct QuadtreeNode* children[4];  // 4 children

    bool is_divided;
} QuadtreeNode;

/* ───────── GLOBALS ───────── */

/* Octree (3D) */
extern GridCell3D grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
extern SpatialObject all_objects[MAX_TOTAL_OBJECTS];

/* Quadtree (2D) */
extern SpatialObject2D all_objects_2d[MAX_TOTAL_OBJECTS];

extern int total_objects;
extern long visited_nodes;

#endif
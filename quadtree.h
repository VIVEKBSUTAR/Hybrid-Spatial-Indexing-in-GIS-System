#ifndef QUADTREE_H
#define QUADTREE_H

#include "types.h"
#include <stdbool.h>

/* Geometry */
bool contains2D(BoundingBox2D b, SpatialObject2D* obj);
bool intersects2D(BoundingBox2D a, BoundingBox2D b);

/* Core tree */
QuadtreeNode* createNode(BoundingBox2D boundary);
void subdivide(QuadtreeNode* node);
bool insertQuadtree(QuadtreeNode* node, SpatialObject2D* obj, int depth);

void getQuadTreeStats(QuadtreeNode* node, int depth,
                      int* total_nodes, int* max_depth, int* obj_count);
/* Queries */
void rangeQuery2D(QuadtreeNode* node,
                  BoundingBox2D range,
                  SpatialObject2D* results[],
                  int* count);

/* Memory */
void freeQuadtree(QuadtreeNode* node);

#endif
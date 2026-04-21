#ifndef OCTREE_H
#define OCTREE_H

#include "types.h"

/* Geometry helpers */
bool        containsPoint3D(BoundingBox3D box, double x, double y, double z);
bool        intersectsBox3D(BoundingBox3D a, BoundingBox3D b);
double      pointDistance3D(double x1, double y1, double z1,
                            double x2, double y2, double z2);

/* Node lifecycle */
OctreeNode* createOctreeNode(BoundingBox3D boundary);
void        subdivideOctreeNode(OctreeNode* node);
bool        insertOctree(OctreeNode* node, SpatialObject* obj, int depth);

/* Statistics */
void getOctreeStats(OctreeNode* node, int depth,
                    int* total_nodes, int* max_depth, int* obj_count);
void printGridStats(void);

#endif

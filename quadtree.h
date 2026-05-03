#ifndef QUADTREE_H
#define QUADTREE_H

#include "types.h"

bool         containsPoint(BoundingBox box, double x, double y);
bool         intersectsBox(BoundingBox a, BoundingBox b);
double       pointDistance(double x1, double y1, double x2, double y2);

QuadTreeNode* createNode(BoundingBox boundary);
void          subdivideNode(QuadTreeNode* node);
bool          insertQuadTree(QuadTreeNode* node, SpatialObject* obj, int depth);

/* Statistics */
void  getQuadTreeStats(QuadTreeNode* node, int depth,
                       int* total_nodes, int* max_depth, int* obj_count);
void  printGridStats(void);

#endif

#include <vector>

#ifndef COMMONSPATIAL_H
#define	COMMONSPATIAL_H

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* COMMONSPATIAL_H */

using namespace std;


class SpatialObject {
public:
    double left, right, top, bottom;
    SpatialObject(double left, double right, double top, double bottom);
};

class BinarySplitNode {
public:
    double left;
    double right;
    double top;
    double bottom;
    int level;
    bool isLeaf;
    bool canBeSplit;
    int size;
    BinarySplitNode* first;
    BinarySplitNode* second;
    vector<SpatialObject*> objectList;
    
    BinarySplitNode(double left, double right, double top, 
        double bottom, int level);
    
    ~BinarySplitNode();
    
    bool addObject(SpatialObject *object);
    bool intersects(SpatialObject *object);
    bool addObjectIgnore(SpatialObject *object);    
};

// proposed by Jensen
#include "nanoflann.hpp"
#include "Image.h"
#include <queue>

using namespace nanoflann;

struct Photon
{
    float x, y, z;
    Vector3f _position;
    Vector3f _direction;

    // not sure if power is best stored as a vector?
    Vector3f _power;

    // Representation from Jensen
    // char p[4];     // power packed as 4 chars
    // char phi, theta; // direction, compressed; unnecessary?
    // float d_x, d_y, d_z; // direction
    // short flag;          // flag used in kdtree
};
#pragma once
struct PointCloud
{

    std::vector<Photon> pts;

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return pts.size(); }

    // Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
    inline float kdtree_distance(const float *p1, const size_t idx_p2, size_t /*size*/) const
    {
        const float d0 = p1[0] - pts[idx_p2].x;
        const float d1 = p1[1] - pts[idx_p2].y;
        const float d2 = p1[2] - pts[idx_p2].z;
        return std::sqrt(d0 * d0 + d1 * d1 + d2 * d2);
    }

    // Returns the dim'th component of the idx'th point in the class:
    // Since this is inlined and the "dim" argument is typically an immediate value, the
    //  "if/else's" are actually solved at compile time.
    inline float kdtree_get_pt(const size_t idx, int dim) const
    {
        if (dim == 0)
            return pts[idx].x;
        else if (dim == 1)
            return pts[idx].y;
        else
            return pts[idx].z;
    }

    template <class BBOX>
    bool kdtree_get_bbox(BBOX & /*bb*/) const { return false; }
};

// construct a kd-tree index:
typedef KDTreeSingleIndexAdaptor<
    L2_Simple_Adaptor<float, PointCloud>,
    PointCloud,
    3 /* dim */
    >
    my_kd_tree_t;

class PhotonMap
{
  public:
    // hardcoded for now, can pass as param later.
    size_t _numberOfPhotons;
    float _searchRadius;
    size_t _maxBounces;
    SceneParser _scene; // need _scene to get lights.
    PointCloud cloud;
	my_kd_tree_t   index;
    size_t _getNearest; // how many nearest photon to get for radiance calculation?

    // NOTE: will also need some structure to store our photons
    // efficiently to find nearest neighbors (kdtree)
    // for irradiance :)
    // for now, just using a vector until we add kdtree functionality

    std::vector<Photon> _photons;

    PhotonMap(const ArgParser &_args, size_t numberOfPhotons);
    Vector3f tracePhoton(Photon &p, int bounces);
    void generateMap();
    void storePhoton(Photon &p);
    Vector3f findRadiance(Vector3f hitPoint, Vector3f normal);

    // todo:
    // scatter photons (emit photons from light sources)
    // store them in photon map when they hit nonspecular objects
    // calculate radiance via local density

    // constructor needs num of light sources
};
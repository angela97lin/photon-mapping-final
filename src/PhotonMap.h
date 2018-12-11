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
    Vector3f _powerVector;
    float _power;

    Photon copy()
    {
        Photon newP;
        newP.x = x;
        newP.y = y;
        newP.z = z;
        newP._direction = _direction;
        newP._powerVector = _powerVector;
        newP._power = _power;
        newP._position = _position;

        return newP;
    }

    void setPosition(Vector3f pos)
    {
        x = pos.x();
        y = pos.y();
        z = pos.z();
        _position = pos;
    }
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
    kd_tree_t;

class PhotonMap
{

  public:
    enum Channel
    {
        red = 0,
        green = 1,
        blue = 2,
        size
    };

    size_t _numberOfPhotons;
    float _searchRadius;
    size_t _maxBounces;
    SceneParser _scene;
    PointCloud cloud;
    kd_tree_t index;
    size_t _getNearest; // how many nearest photons to get (for radiance calculation)

    std::vector<Photon> getPhotons();
    void generateMap(std::vector<Photon> photonList);

    // void generateMap(Channel channel);
    void generatePhoton(Light *l, Channel channel, std::vector<Photon> &v);


    PhotonMap(const ArgParser &_args, size_t numberOfPhotons);
    void tracePhoton(Photon &p, int bounces, Channel channel, std::vector<Photon> &v);

    void storePreprocessPhoton(Photon &p, Channel channel);

    void generateMap();
    void storePhoton(Photon &p);
    Vector3f findRadiance(Hit h, Vector3f hitPoint);
    void scalePhotonPower(const float scale);
    // std::vector<size_t> getNearestNeighbors(Hit h, Vector3f hitPoint);
};
// proposed by Jensen
#include "nanoflann.hpp"
#include "Image.h"
#include <queue>


struct Photon
{
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





class PhotonMap
{
public:

    // hardcoded for now, can pass as param later.
    size_t _numberOfPhotons;
    float _radius;
    size_t _maxBounces;
    SceneParser _scene; // need _scene to get lights.


    size_t _getNearest; // how many nearest photon to get for radiance calculation?

    // NOTE: will also need some structure to store our photons
    // efficiently to find nearest neighbors (kdtree)
    // for irradiance :)
    // for now, just using a vector until we add kdtree functionality

    std::vector<Photon> _photons;

    PhotonMap(const ArgParser &_args, size_t numberOfPhotons);
    Vector3f tracePhoton(Photon &p, int bounces);
    void generateMap();
    void storePhoton(Photon& p);
    Vector3f findRadiance(Vector3f hitPoint, Vector3f normal);

    // todo:
    // scatter photons (emit photons from light sources)
    // store them in photon map when they hit nonspecular objects
    // calculate radiance via local density

    // constructor needs num of light sources
};
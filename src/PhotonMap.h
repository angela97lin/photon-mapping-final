// proposed by Jensen
#include "nanoflann.hpp"
#include "Image.h"

struct Photon
{
    Vector3f position;
    Vector3f direction;
    Vector3f power;

    // char p[4];     // power packed as 4 chars
    // char phi, theta; // direction, compressed; unnecessary?
    // float d_x, d_y, d_z; // direction
    // short flag;          // flag used in kdtree
};


class PhotonMap
{

public:

    // hardcoded for now, can pass as param later.
    size_t numberOfPhotons = 100;
    float radius = 0.03;
    SceneParser _scene;
    // need _scene for lights.



    PhotonMap(const ArgParser &_args);
    Vector3f
    traceRay(const Ray &r,
                   float tmin,
                   int bounces,
                   Hit &h) const;
    void generateMap();
    void storePhoton(Photon& p);

    // todo:
    // scatter photons (emit photons from light sources)
    // store them in photon map when they hit nonspecular objects
    // calculate radiance via local density

    // constructor needs num of light sources
};
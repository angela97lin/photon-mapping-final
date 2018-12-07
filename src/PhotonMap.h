// proposed by Jensen
#include "nanoflann.hpp"
#include "Image.h"

struct Photon
{
    float x, y, z; // position
    char p[4];     // power packed as 4 chars
    // char phi, theta; // direction, compressed; unnecessary?
    float d_x, d_y, d_z; // direction
    short flag;          // flag used in kdtree
};

class PhotonMapping
{

    size_t numberOfPhotons = 1000;
    float radius = 0.03;
    SceneParser _scene;

    // need _scene for lights.
    PhotonMapping(const ArgParser &_args) : _scene(_args.input_file)
    {
        int w = _args.width;
        int h = _args.height;

        Image image(w, h);
        int numLights = _scene.getNumLights();
        for (int i = 0; i < numLights; ++i)
        {
            Light *l = _scene.getLight(i);
            if (l->_type == 0) // point light, scatter photons randomly
            {
                image.savePNG("test_photonmap");
            }
        }
    }

    // todo:
    // scatter photons
    // calculate radiance via local density

    // constructor needs num of light sources
};
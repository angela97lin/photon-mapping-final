// proposed by Jensen
#include "nanoflann.hpp"

struct Photon
{
    float x, y, z;   // position
    char p[4];       // power packed as 4 chars
    // char phi, theta; // direction
    float d_x, d_y, d_z;   // direction
    short flag;      // flag used in kdtree
};

class PhotonMapping 
{

};
#include "Renderer.h"
#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"
#include "PhotonMap.h"
#include "nanoflann.hpp"
#include <limits>
#include <random>

#define EPSILON 0.01
#define WEAK_LIGHT 0.001

/* 

Photon Mapping: 2 Pass Algorithm

1. Create photon map
    - Loop through each of the light sources 
        (for simplicity, we will only use one point light source to begin)
    - Emit photons:
        Until we have the number of photons we have specified,
        we will emit photons randomly, then trace the photon's path (just as we might for rays in ray tracing)
            if photon doesn't hit anything:
                - discard

            if photon hits diffuse surface:
                - store the photon
                - then, use russian roulette in order to determine 
                whether or not we want to trace again
                    - for simplicity, we may want to limit the number of bounces as well
                    but not sure if this will impact our results

            if photon hits reflective (later, refractive as well) surface:
                - don't store photon (? unsure)
                -


    Current simplifications / scoping to remove:
        - Only one light (point light)


Once we've found our photons, we can render the scene as follows:

2. As we render each pixel, we will take into consideration
the intensity of the light values for a certain number of nearby photons.
    - we can easily compute and determine the closest photons using our kdtree structure
    - then, we can add this value to the indirect illumination term...




How to VISUALIZE photon map?
    - We need a way to go from 3D (positions in our scene) to 2D (image coordinates)
    --> add spheres to scene and visualize!
*/

// Get luminance value from color
float PhotonMap::getLuminance(Photon &p)
{
    Vector3f color = p._power;
    float R = color.x();
    float G = color.y();
    float B = color.z();
    return 0.2126f * R + 0.7152f * G + 0.0722f * B;
}

PhotonMap::PhotonMap(const ArgParser &_args, size_t numberOfPhotons) : _scene(_args.input_file),
                                                                       _numberOfPhotons(numberOfPhotons),
                                                                       index(3 /*dim*/, cloud, KDTreeSingleIndexAdaptorParams(10 /* max leaf */))

{
    _maxBounces = 3;    // also hardcoded for now; used to make sure if RR doesn't terminate, we set a bound
    _searchRadius = 10; // hardcoded for now, tbd when we calculate irradiance
    _getNearest = 10;
}

// Used for Russian Roulette,
// just generates a random float from 0 to max.
static float generateRandom(float min, float max)
{
    // std::default_random_engine generator;
    // std::uniform_real_distribution<double> distribution(0, max);
    // return (float)distribution(generator);
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(min, max);
    return (float)dis(gen);
}

void PhotonMap::generateMap()
{
    // used to get random direction
    // in reality, we need to calculate the
    // number of photons we want to emit per light
    // depending on the total number of lights in our scene,
    // and then dividing up our desired number of photons
    // by this number (wattage).
    // however, not relevant right now since we only want to handle one light source
    // all of our photons will be generated from this one light
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(-1, 1); //doubles from -1 to 1

    size_t currentNumPhotons = 0;
    int numLights = _scene.getNumLights();
    for (int i = 0; i < numLights; ++i)
    {
        Light *l = _scene.getLight(i);

        if (l->_type == 0) // point light, scatter photons randomly
        {
            while (currentNumPhotons < _numberOfPhotons)
            {
                // simple rejection algorithm from Jensen
                double x, y, z;
                do
                {
                    x = ((float(rand()) / float(RAND_MAX)) * (2.0f)) - 1.0f;

                    y = ((float(rand()) / float(RAND_MAX)) * (2.0f)) - 1.0f;
                    z = ((float(rand()) / float(RAND_MAX)) * (2.0f)) - 1.0f;
                } while ((x * x + y * y + z * z) > 1.0);

                Vector3f dir = Vector3f(x, y, z).normalized();
                // dir.print();
                Photon p;
                p.x = dir.x();
                p.y = dir.y();
                p.z = dir.z();
                p._position = l->_position;
                p._direction = dir;
                p._power = l->_power;

                Ray r = Ray(p._position, p._direction);
                Hit h;
                Camera *cam = _scene.getCamera();
                float tmin = cam->getTMin();
                if (_scene.getGroup()->intersect(r, tmin, h))
                {

                    // Use diffuse and specular values to determine
                    // what kind of material we have.
                    // We also need this for our Russian Roulette to determine
                    // whether we should store the photon
                    // and with what probability we should absorb, reflect, transmit (?)

                    // Update values and store:
                    Vector3f hitPoint = r.pointAtParameter(h.getT());
                    p.x = hitPoint.x();
                    p.y = hitPoint.y();
                    p.z = hitPoint.z();
                    p._position = hitPoint;
                    tracePhoton(p, _maxBounces);

                    // Tracing photons may have produced a number
                    // of photons, so update to determine if we should continue.
                    currentNumPhotons = _photons.size();
                    // printf("current number of photons stored: %d\n", currentNumPhotons);
                }
            }
        }

        printf("size of cloud after init: %d\n", cloud.pts.size());
    }
    // Build KD tree and place all photons in tree.
    index.buildIndex();
    printf("Done building map!\n");
}

// currently, just store in vector list
// we can later use that list to build our tree.
void PhotonMap::storePhoton(Photon &photon)
{
    _photons.push_back(photon);
    cloud.pts.push_back(photon);
}

void PhotonMap::tracePhoton(Photon &p, int bounces)
{
    // if a photon is extremely weak, we won't even consider it since
    // its contribution will be minimal

    // printf("number of bounces left: %d\n", bounces);
    //if (getLuminance instead? < WEAK_LIGHT)

    if (p._power.abs() < WEAK_LIGHT)
    {
        return;
    }

    Camera *cam = _scene.getCamera();
    float tmin = cam->getTMin();

    Ray r = Ray(p._position, p._direction);
    Hit h;
    if (_scene.getGroup()->intersect(r, tmin, h))
    {

        Vector3f hitPoint = r.pointAtParameter(h.getT());
        // Use diffuse and specular values to determine
        // what kind of material we have.
        // We also need this for our Russian Roulette to determine
        // whether we should store the photon
        // and with what probability we should absorb, reflect, transmit (?)
        if (h.getMaterial()->isDiffusive())
        {
            // Update values and store:
            Vector3f hitPoint = r.pointAtParameter(h.getT());
            p.x = hitPoint.x();
            p.y = hitPoint.y();
            p.z = hitPoint.z();
            p._position = hitPoint;
            // p._direction --> stays the same (?)
            storePhoton(p);
        }

        Vector3f diffuse = h.getMaterial()->getDiffuseColor();
        float diffuseMagnitude = diffuse.abs();
        Vector3f specular = h.getMaterial()->getSpecularColor();
        float specularMagnitude = specular.abs();
        float absorptionMagnitude = 0.4;
        float probabilitySum = diffuseMagnitude + specularMagnitude + absorptionMagnitude;
        float randNum = generateRandom(0.0f, probabilitySum);

        // hardcoded for now, should later augment materials to save
        // indices referring to diffuse and specular indicies

        // if reflective / refractive, don't store.
        // power and direction get modified, continue to bounce until limit

        // if diffuse, store power, position, direction in map
        // then, modify power and direction and bounce again until limit reached

        // float randNum = generateRandom(0.0, 1.0);
        if (randNum < absorptionMagnitude) // absorb
        {
            // "Absorption is the general termination condition upon which
            // we then store it in the photon map."
            storePhoton(p);
            return;
        }

        else
        {
            // might also want to check number of photons
            // here to see if we even have space for more
            if (bounces > 0)
            {
                // use BRDF of surface in order to calculate new direction.
                Vector3f k_s = h.getMaterial()->getSpecularColor();
                Vector3f eye = r.getDirection();
                Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
                Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
                p.x = newRayDir.x();
                p.y = newRayDir.y();
                p.z = newRayDir.z();

                p._direction = newRayDir;
                p._position = newRayOrigin;
                p._power = 0.6 * p._power;
                tracePhoton(p, bounces - 1);
            }
            return;
        }
    }
    return;
}

// This will primarily be used in rendering.
// TODO:
// find the radiance of a point by
// first finding the N nearest photons,
// and then using radiance estimate algorithm
Vector3f PhotonMap::findRadiance(Hit h, Vector3f hitPoint, Vector3f normal)
{
    // for now, lets say we have a vector that
    // contains our N nearest points
    // in reality, need kdtree
    const float query_pt[3] = {hitPoint.x(), hitPoint.y(), hitPoint.z()};
    // find N closest points
    std::vector<std::pair<size_t, float>> ret_matches;
    nanoflann::SearchParams params;
    params.sorted = true;
    size_t num_results = index.radiusSearch(&query_pt[0], _searchRadius, ret_matches, params);
    float radius_2; // used to determine our radiance via spherical SA.
    if (num_results > _getNearest)
    {
        num_results = _getNearest;
        radius_2 = ret_matches[num_results - 1].second * ret_matches[num_results - 1].second;
    }
    else
    {
        radius_2 = _searchRadius * _searchRadius;
    }

    // Calculate radiance:
    float surfaceArea = 4.0f * M_PI * radius_2;
    Vector3f I = Vector3f(0.0f);
    for (int i = 0; i < num_results; ++i)
    {
        size_t index = ret_matches[i].first;
        Photon photon = cloud.pts[index];
        Vector3f dir = photon._direction;
        Vector3f power = photon._power;
        float angle = Vector3f::dot(normal, dir);
        Vector3f k_d = h.getMaterial()->getDiffuseColor();

    float LN_angle = Vector3f::dot(normal.normalized(), dir);
    float clamped_d = std::max(LN_angle, 0.0f);

    float di_x = clamped_d * power.x() * k_d.x();
    float di_y = clamped_d * power.y() * k_d.y();
    float di_z = clamped_d * power.z() * k_d.z();
    Vector3f d_i = Vector3f(di_x, di_y, di_z);

        if (angle > 0)
        {
            // intensity × (d · N) × diffuse-factor
            I += d_i / radius_2;
        }
        else
        {
            // facing inwards to surface and thus, don't consider
        }
        // cloud.pts[ret_matches[i].first]._direction.print();
    }

    float intensity = I.abs() * (1.0f / surfaceArea);
   // if radiance too small --> ignore
    // if radiance > 1 --> must normalize
    if (intensity < WEAK_LIGHT)
    {
        return Vector3f(0.0f);
    }
    else
    {
        if (intensity > 1.0f)
        {
            I.normalize();
        }
        return I;
    }
 
}

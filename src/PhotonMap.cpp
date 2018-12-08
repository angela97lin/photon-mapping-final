#include "Renderer.h"
#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"
#include "PhotonMap.h"
#include <limits>
#include <random>

#define EPSILON 0.01

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
    -->
*/

PhotonMap::PhotonMap(const ArgParser &_args, size_t numberOfPhotons) : _scene(_args.input_file),
                                                                       _numberOfPhotons(numberOfPhotons)
{
    _maxBounces = 3; // also hardcoded for now; used to make sure if RR doesn't terminate, we set a bound
    _radius = 0.03;  // hardcoded for now, tbd when we calculate irradiance
}

// Used for Russian Roulette
static float generateRandom(float max)
{
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0, max);
    return (float)distribution(generator);
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
    printf("number of lights in scene: %d\n", numLights);
    for (int i = 0; i < numLights; ++i)
    {
        Light *l = _scene.getLight(i);

        if (l->_type == 0) // point light, scatter photons randomly
        {
            printf("found point light!\n");
            while (currentNumPhotons < _numberOfPhotons)
            {
                // simple rejection algorithm from Jensen
                double x, y, z;
                do
                {
                    x = distribution(generator);
                    y = distribution(generator);
                    z = distribution(generator);
                } while ((x * x + y * y + z * z) > 1);
                
                Vector3f dir = Vector3f(x, y, z);

                Photon p;
                p._position = l->_position; //position as just from the light (?)
                p._direction = dir;
                p._power = Vector3f(l->_wattage, l->_wattage, l->_wattage);
                tracePhoton(p, _maxBounces);

                // Tracing photons may have produced a number
                // of photons, so update to determine if we should continue.
                currentNumPhotons = _photons.size();
                printf("current number of photons stored: %d\n", currentNumPhotons);
            }
        }
    }
}

// currently, just store in vector list
void PhotonMap::storePhoton(Photon &p)
{
    _photons.push_back(p);
}



Vector3f PhotonMap::tracePhoton(Photon &p, int bounces)
{
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
        // Q: is diffuse the same as diffuse const?
        // Vector3f diffuse = h.getMaterial()->getDiffuseColor();
        // float diffuseMagnitude = diffuse.abs();
        // Vector3f specular = h.getMaterial()->getSpecularColor();
        // float specularMagnitude = specular.abs();

        // float probabilitySum = diffuseMagnitude + specularMagnitude;
        // float r = generateRandom(probabilitySum);

        // hardcoded for now, should later augment materials to save
        // indices referring to diffuse and specular indicies

        // if reflective / refractive, don't store.
        // power and direction get modified, continue to bounce until limit

        // if diffuse, store power, position, direction in map
        // then, modify power and direction and bounce again until limit reached

        float randNum = generateRandom(1.0);
        if (randNum < 0.5) // absorb
        {
            // "Absorption is the general termination condition upon which
            // we then store it in the photon map."
            storePhoton(p);
            return p._power;
        }
        else
        {
            // might also want to check number of photons 
            // here to see if we even have space for more
            if (bounces > 0)
            {
                Photon *secondaryPhoton;

                // use BRDF of surface in order to calculate new direction.
                Vector3f k_s = h.getMaterial()->getSpecularColor();
                Vector3f eye = r.getDirection();
                Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
                Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
                secondaryPhoton->_direction = newRayDir;
                secondaryPhoton->_position = newRayOrigin;
                secondaryPhoton->_power = k_s * p._power;
                return tracePhoton(*secondaryPhoton, bounces - 1);
                
            }
        }
    }
}


// This will primarily be used in rendering.
// TODO:
// find the irradiance of a point by
// first finding the N nearest photons,
// and then using radiance estimate algorithm
Vector3f PhotonMap::findRadiance(Vector3f hitPoint, Vector3f normal)
{
    // for now, lets say we have a vector that
    // contains our N nearest points 
    // in reality, need kdtree
    float surfaceArea = M_PI * _radius * _radius;
    std::vector<Photon> nearestPhotons;
    for (int i = 0; i < nearestPhotons.size(); ++i)
    {
        Vector3f dir = nearestPhotons[i]._direction;
        float angle = Vector3f::dot(normal, dir);
        if (angle > 0)
        {
            // intensity × (d · N) × diffuse-factor
        }
        else 
        {
        // facing inwards to surface and thus, don't consider

        }

    }
    return Vector3f(0.0f);

    // if radiance too small --> ignore
    // if radiance > 1 --> must normalize
}
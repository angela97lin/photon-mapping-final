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
// Probs don't need?
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
    _maxBounces = 4;      // also hardcoded for now; used to make sure if RR doesn't terminate, we set a bound
    _searchRadius = .10f; // hardcoded for now, tbd when we calculate irradiance
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

// utility function to generate random unit vectors for direction
inline Vector3f RandomUnitVector()
{
    double x, y, z;
    do
    {
        x = ((float(rand()) / float(RAND_MAX)) * (2.0f)) - 1.0f;
        y = ((float(rand()) / float(RAND_MAX)) * (2.0f)) - 1.0f;
        z = ((float(rand()) / float(RAND_MAX)) * (2.0f)) - 1.0f;
    } while ((x * x + y * y + z * z) > 1.0);

    Vector3f dir = Vector3f(x, y, z).normalized();
    return dir;
}

// compute a random diffuse direction
// (not the same as a uniform random direction on the hemisphere)
inline Vector3f randomDiffuseDirection(const Vector3f &normal)
{
    Vector3f dir = normal + RandomUnitVector();
    dir.normalize();
    return dir;
}

void PhotonMap::generatePhoton(Light *l)
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

    // these are just to visualize photons for debugging
    Material m = Material(p._power);
    p._sphere = new Sphere(p._position, 0.03f, &m);

    Ray r = Ray(p._position, p._direction);
    Hit h;
    Camera *cam = _scene.getCamera();
    float tmin = cam->getTMin();
    if (_scene.getGroup()->intersect(r, tmin, h))
    {
        // Update values and store:
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        p.x = hitPoint.x();
        p.y = hitPoint.y();
        p.z = hitPoint.z();
        p._position = hitPoint;
        p._sphere = new Sphere(p._position, 0.03f, &m);

        tracePhoton(p, _maxBounces);
    }
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
                generatePhoton(l);
                // Tracing photons may have produced a number
                // of photons, so update to determine if we should continue.
                currentNumPhotons = cloud.pts.size();
            }
        }
    }
    // Build KD tree and place all photons in tree.
    // scalePhotonPower(1.0f / _numberOfPhotons);
    index.buildIndex();
    printf("Done building map...\n");
    printf("Size of cloud: %d\n", cloud.pts.size());
}

// currently, just store in vector list
// we can later use that list to build our tree.
void PhotonMap::storePhoton(Photon &photon)
{
    Photon photonToStore;
    photonToStore.x = photon.x;
    photonToStore.y = photon.y;
    photonToStore.z = photon.z;
    photonToStore._position = Vector3f(photon._position);
    photonToStore._direction = Vector3f(photon._direction);
    photonToStore._sphere = new Sphere(*photon._sphere);
    photonToStore._power = Vector3f(photon._power);
    cloud.pts.push_back(photonToStore);
}

void PhotonMap::tracePhoton(Photon &p, int bounces)
{
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
        // determine whether to specularly reflect
        // or diffusely reflect or just get absorbed
        // for each color component?
        for (int color = 0; color < 3; ++color)
        {
            float diffuseComponent = h.getMaterial()->getDiffuseColor()[color];
            // for later, but only working with strictly diffuse surfaces right now.
            float specularComponent = h.getMaterial()->getSpecularColor()[color];
            // NOTE: assuming that diffuseComponent + specularComponent < 1.0f

            float randNum = generateRandom(0.0f, 1.0f);
            if (randNum < diffuseComponent)
            {
                // diffusely reflect (uniformly choose random direction to reflect to)
                // should be same except direction chosen randomly rather than based on specular
                if (bounces > 0)
                {
                    Vector3f hitPoint = r.pointAtParameter(h.getT());
                    p.x = hitPoint.x();
                    p.y = hitPoint.y();
                    p.z = hitPoint.z();
                    p._position = hitPoint;
                    Material m = Material(p._power);
                    p._sphere = new Sphere(p._position, 0.03f, &m);
                    storePhoton(p);

                    // Generate new photon for bouncing:
                    // Photon newP;
                    // newP.x = hitPoint.x();
                    // newP.y = hitPoint.y();
                    // newP.z = hitPoint.z();
                    // newP._position = hitPoint;
                    // newP._power = Vector3f(0.0f);
                    // newP._power[color] = diffuseComponent;
                    // Vector3f newDir = randomDiffuseDirection(h.getNormal());
                    // Material newM = Material(newP._power);
                    // newP._sphere = new Sphere(newP._position, 0.03f, &m);
                    // newP._direction = newDir;
                    // newP._power.print();

                    p._power = Vector3f(0.0f);
                    p._power[color] = diffuseComponent;
                    Vector3f newDir = randomDiffuseDirection(h.getNormal());
                    p._direction = newDir;
                    Material newM = Material(p._power);
                    p._sphere = new Sphere(p._position, 0.03f, &m);
                    tracePhoton(p, bounces - 1);
                }
                return;
            }

            // ignore for now
            else if (randNum < (diffuseComponent + specularComponent))
            {
                // specularly reflect
                if (bounces > 0)
                {
                    Vector3f k_s = h.getMaterial()->getSpecularColor();
                    Vector3f eye = r.getDirection();
                    Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
                    Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
                    p.x = newRayDir.x();
                    p.y = newRayDir.y();
                    p.z = newRayDir.z();

                    p._direction = newRayDir;
                    p._position = newRayOrigin;
                    Material m = Material(p._power);
                    p._sphere = new Sphere(p._position, 0.03f, &m);
                    tracePhoton(p, bounces - 1);
                }
            }

            else
            {
                // absorb photon by storing
                // "Absorption is the general termination condition upon which
                // we then store it in the photon map."
                storePhoton(p);
                return;
            }
        }



        // // determine whether to specularly reflect or diffusely reflect or just get absorbed.
        // for (int color = 0; color < 3; ++color)
        // {
        //     float diffuseComponent = h.getMaterial()->getDiffuseColor()[color];
        //     // for later, but only working with strictly diffuse surfaces right now.
        //     float specularComponent = h.getMaterial()->getSpecularColor()[color];
        //     // NOTE: assuming that diffuseComponent + specularComponent < 1.0f

        //     float randNum = generateRandom(0.0f, 1.0f);
        //     if (randNum < diffuseComponent)
        //     {
        //         // diffusely reflect (uniformly choose random direction to reflect to)
        //         // should be same except direction chosen randomly rather than based on specular
        //         if (bounces > 0)
        //         {
        //             Vector3f hitPoint = r.pointAtParameter(h.getT());
        //             p.x = hitPoint.x();
        //             p.y = hitPoint.y();
        //             p.z = hitPoint.z();
        //             p._position = hitPoint;
        //             Material m = Material(p._power);
        //             p._sphere = new Sphere(p._position, 0.03f, &m);
        //             storePhoton(p);

        //             // Generate new photon for bouncing:
        //             // Photon newP;
        //             // newP.x = hitPoint.x();
        //             // newP.y = hitPoint.y();
        //             // newP.z = hitPoint.z();
        //             // newP._position = hitPoint;
        //             // newP._power = Vector3f(0.0f);
        //             // newP._power[color] = diffuseComponent;
        //             // Vector3f newDir = randomDiffuseDirection(h.getNormal());
        //             // Material newM = Material(newP._power);
        //             // newP._sphere = new Sphere(newP._position, 0.03f, &m);
        //             // newP._direction = newDir;
        //             // newP._power.print();

        //             p._power = Vector3f(0.0f);
        //             p._power[color] = diffuseComponent;
        //             Vector3f newDir = randomDiffuseDirection(h.getNormal());
        //             Material newM = Material(p._power);
        //             p._sphere = new Sphere(p._position, 0.03f, &m);
        //             tracePhoton(p, bounces - 1);
        //         }
        //     }

        //     // ignore for now
        //     else if (randNum < (diffuseComponent + specularComponent))
        //     {
        //         // specularly reflect
        //         if (bounces > 0)
        //         {
        //             Vector3f k_s = h.getMaterial()->getSpecularColor();
        //             Vector3f eye = r.getDirection();
        //             Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
        //             Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
        //             p.x = newRayDir.x();
        //             p.y = newRayDir.y();
        //             p.z = newRayDir.z();

        //             p._direction = newRayDir;
        //             p._position = newRayOrigin;
        //             Material m = Material(p._power);
        //             p._sphere = new Sphere(p._position, 0.03f, &m);
        //             tracePhoton(p, bounces - 1);
        //         }
        //     }

        //     else
        //     {
        //         // absorb photon by storing
        //         // "Absorption is the general termination condition upon which
        //         // we then store it in the photon map."
        //         storePhoton(p);
        //         return;
        //     }
        // }
    }
    return;
}

void PhotonMap::scalePhotonPower(const float scale)
{
    for (int i = 0; i < cloud.pts.size(); i++)
    {
        cloud.pts[i]._power *= scale;
    }
}

std::vector<size_t> PhotonMap::getNearestNeighbors(Hit h, Vector3f hitPoint)
{
    std::vector<size_t> photonList;
    Vector3f normal = h.getNormal();
    const float query_pt[3] = {hitPoint.x(), hitPoint.y(), hitPoint.z()};
    // find N closest points
    std::vector<std::pair<size_t, float>> ret_matches;
    nanoflann::SearchParams params;
    params.sorted = true;
    size_t num_results = index.radiusSearch(&query_pt[0], _searchRadius, ret_matches, params);

    for (int i = 0; i < num_results; ++i)
    {
        size_t index = ret_matches[i].first;
        photonList.push_back(index);
    }
    // printf("Nearest neighbor list size: %d\n", num_results);
    return photonList;
}

// This will primarily be used in rendering.
// TODO:
// find the radiance of a point by
// first finding the N nearest photons,
// and then using radiance estimate algorithm
Vector3f PhotonMap::findRadiance(Hit h, Vector3f hitPoint)
{
    // for now, lets say we have a vector that
    // contains our N nearest points
    // in reality, need kdtree
    Vector3f normal = h.getNormal();
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
        Vector3f d_i = k_d * Vector3f(di_x, di_y, di_z);

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
    // if (intensity > 0.0f)
    // {
    //     printf("This photon is something!\n");
    // }
    // else
    // {
    //             printf("This photon is NOT something!\n");

    // }
    if (intensity > 1.0f)
    {
        I.normalize();
    }

    return I;
}
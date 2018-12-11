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
    // dir.print();
    return dir;
}

void PhotonMap::generatePhoton(Light *l, Channel channel, std::vector<Photon> &v)
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

    Ray r = Ray(l->_position, dir);
    Hit h;
    Camera *cam = _scene.getCamera();
    float tmin = cam->getTMin();
    if (_scene.getGroup()->intersect(r, tmin, h))
    {
        // Update values and store:
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        Photon p;
        p.x = hitPoint.x();
        p.y = hitPoint.y();
        p.z = hitPoint.z();
        p._direction = dir;
        p._power = l->_power[channel];
        p._position = hitPoint;
        tracePhoton(p, _maxBounces, channel, v);
    }
}


void PhotonMap::generateMap(std::vector<Photon> photonList)
{
    for (int i = 0; i < photonList.size(); ++i)
    {
        cloud.pts.push_back(photonList[i]);
    }
    // Build KD tree and place all photons in tree.
    scalePhotonPower(1.0f / _numberOfPhotons);
    index.buildIndex();
    printf("Done building map...\n");
    printf("Size of cloud: %d\n", cloud.pts.size());
}

std::vector<Photon> PhotonMap::getPhotons()
{
    std::vector<Photon> master;
    for (int channel = 0; channel < Channel::size; ++channel)
    {
        std::vector<Photon> s_photons;
        int numLights = _scene.getNumLights();
        for (int i = 0; i < numLights; ++i)
        {
            Light *l = _scene.getLight(i);

            if (l->_type == 0) // point light, scatter photons randomly
            {
                for (int j = 0; j < _numberOfPhotons; ++j)
                {
                    generatePhoton(l, (Channel)channel, s_photons);
                }
            }
        }
        for (int k = 0; k < s_photons.size(); ++k)
        {
            Photon newP = s_photons[k].copy();
            newP._powerVector = Vector3f::ZERO;
            newP._powerVector[channel] = newP._power;
            // newP._powerVector.print();
            master.push_back(newP);
        }
    }

    return master;
}

void PhotonMap::tracePhoton(Photon &p, int bounces, Channel channel, std::vector<Photon> &v)
{
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
        float diffuseComponent = h.getMaterial()->getDiffuseColor()[channel];
        // h.getMaterial()->getDiffuseColor().print();
        // for later, but only working with strictly diffuse surfaces right now.
        // float specularComponent = h.getMaterial()->getSpecularColor()[color];
        // NOTE: assuming that diffuseComponent + specularComponent < 1.0f

        float randNum = generateRandom(0.0f, 1.0f);
        // printf("random number gen: %f\n", randNum);
        if (randNum > diffuseComponent)
        {
            // diffusely reflect (uniformly choose random direction to reflect to)
            // should be same except direction chosen randomly rather than based on specular
            if (bounces > 0 && bounces != _maxBounces)
            {
                Photon newP = p.copy();
                Vector3f hitPoint = r.pointAtParameter(h.getT());
                newP.setPosition(hitPoint);
                v.push_back(newP);

                Vector3f newDir = randomDiffuseDirection(h.getNormal());
                newP._direction = newDir;
                tracePhoton(newP, bounces - 1, channel, v);
            }
        }

        // // ignore for now
        // else if (randNum < (diffuseComponent + specularComponent))
        // {
        //     // specularly reflect
        //     if (bounces > 0)
        //     {
        //         Vector3f k_s = h.getMaterial()->getSpecularColor();
        //         Vector3f eye = r.getDirection();
        //         Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
        //         Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
        //         p.x = newRayDir.x();
        //         p.y = newRayDir.y();
        //         p.z = newRayDir.z();

        //         p._direction = newRayDir;
        //         p._position = newRayOrigin;
        //         Material m = Material(p._power);
        //         p._sphere = new Sphere(p._position, 0.03f, &m);
        //         tracePhoton(p, bounces - 1);
        //     }
        // }

        else
        {
            // absorb photon by storing
            // "Absorption is the general termination condition upon which
            // we then store it in the photon map."
            Photon newP = p.copy();
            Vector3f hitPoint = r.pointAtParameter(h.getT());
            newP.setPosition(hitPoint);
            v.push_back(newP);
        }
    }

    return;
}

void PhotonMap::scalePhotonPower(const float scale)
{
    for (int i = 0; i < cloud.pts.size(); i++)
    {
        cloud.pts[i]._powerVector.print();
        cloud.pts[i]._power *= scale;
        cloud.pts[i]._powerVector.print();

    }
}




Vector3f PhotonMap::findRadiance(Hit h, Vector3f hitPoint)
{

    Vector3f normal = h.getNormal();
    const float query_pt[3] = {hitPoint.x(), hitPoint.y(), hitPoint.z()};
    // find N closest points
    size_t num_results = 50;
    std::vector<size_t> ret_index(num_results);
    std::vector<float> out_dist_sqr(num_results);
    nanoflann::KNNResultSet<Photon> resultSet(num_results);
    num_results = index.knnSearch(&query_pt[0], num_results, &ret_index[0], &out_dist_sqr[0]);
    ret_index.resize(num_results);
    out_dist_sqr.resize(num_results);
    float radius_2 = out_dist_sqr[num_results - 1];

    // Calculate incident irradiance:
    float surfaceArea = M_PI * radius_2;
    Vector3f I = Vector3f(0.0f);

    for (int i = 0; i < num_results; ++i)
    {
        size_t index = ret_index[i];
        Photon photon = cloud.pts[index];
        Vector3f dir = photon._direction.normalized();
        Vector3f power = photon._powerVector;
        Vector3f dist_vector = hitPoint - photon._position;
        float distToLight = dist_vector.abs();


        float angle = Vector3f::dot(normal, dir);
        Vector3f k_d = h.getMaterial()->getDiffuseColor();
        Vector3f hitNormNormalized = h.getNormal().normalized();

        float LN_angle = Vector3f::dot(hitNormNormalized, dir);
        float clamped_d = std::max(LN_angle, 0.0f);
        float di_x = clamped_d * power.x() * k_d.x();
        float di_y = clamped_d * power.y() * k_d.y();
        float di_z = clamped_d * power.z() * k_d.z();
        Vector3f d_i = k_d * Vector3f(di_x, di_y, di_z);


        if (angle > 0)
        {
            I += d_i * (1.0f / surfaceArea);

        }
        else
        {
            // printf("angle is negative\n");
            // facing inwards to surface and thus, don't consider
        }

    }
    if (surfaceArea > 0.05f)
    {
      
        float intensity = I.abs();

        float mostIntenseComponent = std::max(I.x(), std::max(I.y(), I.z()));
        // printf("Most intense component is: %f\n", mostIntenseComponent);
        if (mostIntenseComponent > 1.0f)
        {
            Vector3f scaledI = I / (I.x() + I.y() + I.z());
            return scaledI;
        }
        return I;
    }
    return Vector3f(0.0f);
}
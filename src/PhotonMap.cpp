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

// for now, lots of simplifications:
// assuming one point light only.
PhotonMap::PhotonMap(const ArgParser &_args) : _scene(_args.input_file)
{
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(-1, 1); //doubles from -1 to 1

    int w = _args.width;
    int h = _args.height;
    size_t num = 0;
    Image image(w, h);
    int numLights = _scene.getNumLights();
    for (int i = 0; i < numLights; ++i)
    {
        Light *l = _scene.getLight(i);

        if (l->_type == 0) // point light, scatter photons randomly
        {
            Camera *cam = _scene.getCamera();

            while (num < numberOfPhotons)
            {
                double x, y, z;
                do
                {
                    x = distribution(generator);
                    y = distribution(generator);
                    z = distribution(generator);
                } while ((x * x + y * y + z * z) > 1);

                Vector3f d = Vector3f(x, y, z);
                Vector3f p = l->_position;
                Ray r = Ray(p, d);
                Hit h;
                Vector3f color = traceRay(r, cam->getTMin(), 3, h);
            }

            image.savePNG("test_photonmap");
            // image.setPixel(x, y, color);
        }
    }
}

void PhotonMap::generateMap()
{
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(-1, 1); //doubles from -1 to 1

    size_t num = 0;
    int numLights = _scene.getNumLights();
    for (int i = 0; i < numLights; ++i)
    {
        // note: even though looping, calculations are based on one light only.
        Light *l = _scene.getLight(i);
        if (l->_type == 0) // point light, scatter photons randomly
        {
            Camera *cam = _scene.getCamera();

            //
            for (int n = 0; n < numberOfPhotons; ++n)
            {
                // double x, y, z;
                // do
                // {
                //     x = distribution(generator);
                //     y = distribution(generator);
                //     z = distribution(generator);
                // } while ((x * x + y * y + z * z) > 1);

                // a random position on unit sphere
                float pX = 2.0 * random() - 1.0;
                float pY = 2.0 * random() - 1.0;
                float pZ = 2.0 * random() - 1.0;
                Vector3f p(pX, pY, pZ);
                p.normalize();

            // initial ray for a photon
                Photon *p;
                p->direction = d;
                p->position = l->_position;

                // not sure about this?
                // Vector3f p = l->_position;
                // Ray r = Ray(p, d);
                // Hit h;
                // Vector3f color = traceRay(r, cam->getTMin(), 3, h);
            }
        }
    }
}

void PhotonMap::storePhoton(Photon &p)
{
}


// ported over tracing ray fxn because should be quite similar, but have yet to actually implement.
Vector3f PhotonMap::traceRay(const Ray &r,
                             float tmin,
                             int bounces,
                             Hit &h) const
{
    // Vector3f I = Vector3f(0.0f, 0.0f, 0.0f);

    // if (_scene.getGroup()->intersect(r, tmin, h))
    // {
    //     Vector3f hitPoint = r.pointAtParameter(h.getT());
    //     Vector3f ambient = h.getMaterial()->getDiffuseColor() * _scene.getAmbientLight();
    //     Vector3f diff_spec = Vector3f(0.0f, 0.0f, 0.0f);

    //     for (size_t i = 0; i < _scene.getNumLights(); ++i)
    //     {
    //         Vector3f tolight;
    //         Vector3f intensity;
    //         float distToLight;

    //         Light *l = _scene.getLight(i);
    //         l->getIllumination(hitPoint, tolight, intensity, distToLight);
    //         diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
    //     }

    //     I += ambient + diff_spec; // direct illumination

    //     //recursive ray tracing
    //     if (bounces > 0)
    //     {
    //         Vector3f k_s = h.getMaterial()->getSpecularColor();
    //         Vector3f eye = r.getDirection();
    //         Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
    //         Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
    //         Ray newRay = Ray(newRayOrigin, newRayDir);
    //         Hit newH = Hit();
    //         Vector3f secondary_ray = traceRay(newRay, 0.0f, bounces - 1, newH);
    //         I += k_s * secondary_ray;
    //     }
    //     return I;
    // }
    // else
    // {
    //     return _scene.getBackgroundColor(r.getDirection());
    // };
}
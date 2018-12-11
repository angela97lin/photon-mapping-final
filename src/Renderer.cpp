#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"
#include "PhotonMap.h"
#include <limits>
#include <random>
#define EPSILON 0.001f
#define NUM_SAMPLES 9

Renderer::Renderer(const ArgParser &args) : _args(args),
                                            _scene(args.input_file),
                                            _sceneCopy(args.input_file)
{
}

Renderer::~Renderer()
{
    delete _map;

}

void Renderer::Render()
{
    // First pass: generate photon map.
    _map = new PhotonMap(_args, 3000);
    auto photons = _map->getPhotons();
    _map->generateMap(photons);

    int w = _args.width;
    int h = _args.height;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);
    Image pimage(w, h); // visualize photon map position
    Image rimage(w, h); // visualize radiance
    Image beforeImage(w, h);
    Camera *cam = _scene.getCamera();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit h;
            Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h);
            image.setPixel(x, y, color);
        }
    }

    _sceneCopy._group->m_members.clear();
    

    for (int i = 0; i < photons.size(); ++i)
    {
    // std::random_device rd;  //Will be used to obtain a seed for the random number engine
    // std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    // std::uniform_real_distribution<> dis(0, photons.size()-1);
     
    //     int v = (int)dis(gen);
        Vector3f color = photons[i]._powerVector;
        // color.print();
        // color.clamped().print();
        Material *m = new Material(color.clamped(), Vector3f::ZERO, 0.0f, true);
        Sphere *s = new Sphere(photons[i]._position, 0.025f, m);
        _sceneCopy.getGroup()->addObject((Object3D *)s);
    }



    

    printf("Drawing photon map...\n");
    // visualize photon map
    for (int y = 0; y < h; ++y)
    {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x)
        {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
            Hit h;
            Vector3f color = traceRayForPhotons(r, cam->getTMin(), 3, h);
            pimage.setPixel(x, y, color);
        }
    }

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit h;
            Vector3f color = traceOriginalRay(r, cam->getTMin(), _args.bounces, h);
            beforeImage.setPixel(x, y, color);
        }
    }

    // save the files
    if (_args.output_file.size())
    {
        image.savePNG(_args.output_file);
    }

    pimage.savePNG(_args.normals_file);
    beforeImage.savePNG(_args.depth_file);
}

Vector3f
Renderer::traceRay(const Ray &r,
                   float tmin,
                   int bounces,
                   Hit &h) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.
    Vector3f I = Vector3f(0.0f, 0.0f, 0.0f);

    if (_scene.getGroup()->intersect(r, tmin, h))
    {
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        Vector3f ambient = h.getMaterial()->getDiffuseColor() * _scene.getAmbientLight();
        Vector3f diff_spec = Vector3f(0.0f, 0.0f, 0.0f);

        if (_args.shadows)
        {
            for (size_t i = 0; i < _scene.getNumLights(); ++i)
            {
                Vector3f tolight;
                Vector3f intensity;
                float distToLight;

                Light *l = _scene.getLight(i);
                l->getIllumination(hitPoint, tolight, intensity, distToLight);
                Vector3f secondaryRayDir = tolight;
                Vector3f secondaryRayOrigin = hitPoint + EPSILON * secondaryRayDir;
                Hit hitObject = Hit();
                Ray pointToLightRay = Ray(secondaryRayOrigin, secondaryRayDir);
                if (_scene.getGroup()->intersect(pointToLightRay, EPSILON, hitObject))
                {
                    Vector3f hitObjectPoint = pointToLightRay.pointAtParameter(hitObject.getT());
                    float distToIntersection = (hitObjectPoint - secondaryRayOrigin).abs();
                    if (distToIntersection < std::numeric_limits<float>().max() && distToIntersection > distToLight)
                    {
                        diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
                    }
                }
                else
                {
                    diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
                }
            }
        }

        //shadows not enabled
        else
        {
            for (size_t i = 0; i < _scene.getNumLights(); ++i)
            {
                Vector3f tolight;
                Vector3f intensity;
                float distToLight;

                Light *l = _scene.getLight(i);
                l->getIllumination(hitPoint, tolight, intensity, distToLight);
                diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
            }
        }
        I += ambient + diff_spec; // direct illumination

        //recursive ray tracing
        if (bounces > 0)
        {
            Vector3f k_s = h.getMaterial()->getSpecularColor();
            Vector3f eye = r.getDirection();
            Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
            Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
            Ray newRay = Ray(newRayOrigin, newRayDir);
            Hit newH = Hit();
            Vector3f secondary_ray = traceRay(newRay, 0.0f, bounces - 1, newH);
            I += k_s * secondary_ray;
        }
        I += _map->findRadiance(h, hitPoint);
        return I;
    }
    else
    {

        return _scene.getBackgroundColor(r.getDirection());
    };
}

Vector3f
Renderer::traceRayForPhotons(const Ray &r,
                             float tmin,
                             int bounces,
                             Hit &h) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.

    Vector3f I = Vector3f(0.0f, 0.0f, 0.0f);

    if (_sceneCopy.getGroup()->intersect(r, tmin, h))
    {

        Vector3f mat = h.getMaterial()->getDiffuseColor();
        // mat.print();
        return mat;

        Vector3f hitPoint = r.pointAtParameter(h.getT());
        Vector3f ambient = h.getMaterial()->getDiffuseColor() * _sceneCopy.getAmbientLight();
        Vector3f diff_spec = Vector3f(0.0f, 0.0f, 0.0f);

        if (_args.shadows)
        {
            for (size_t i = 0; i < _sceneCopy.getNumLights(); ++i)
            {
                Vector3f tolight;
                Vector3f intensity;
                float distToLight;

                Light *l = _scene.getLight(i);
                l->getIllumination(hitPoint, tolight, intensity, distToLight);
                Vector3f secondaryRayDir = tolight;
                Vector3f secondaryRayOrigin = hitPoint + EPSILON * secondaryRayDir;
                Hit hitObject = Hit();
                Ray pointToLightRay = Ray(secondaryRayOrigin, secondaryRayDir);
                if (_sceneCopy.getGroup()->intersect(pointToLightRay, EPSILON, hitObject))
                {
                    Vector3f hitObjectPoint = pointToLightRay.pointAtParameter(hitObject.getT());
                    float distToIntersection = (hitObjectPoint - secondaryRayOrigin).abs();
                    if (distToIntersection < std::numeric_limits<float>().max() && distToIntersection > distToLight)
                    {
                        diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
                    }
                }
                else
                {
                    diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
                }
            }
        }

        //shadows not enabled
        else
        {
            for (size_t i = 0; i < _sceneCopy.getNumLights(); ++i)
            {
                Vector3f tolight;
                Vector3f intensity;
                float distToLight;

                Light *l = _sceneCopy.getLight(i);
                l->getIllumination(hitPoint, tolight, intensity, distToLight);
                diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
            }
        }
        I += ambient + diff_spec; // direct illumination

        //recursive ray tracing
        if (bounces > 0)
        {
            Vector3f k_s = h.getMaterial()->getSpecularColor();
            Vector3f eye = r.getDirection();
            Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
            Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
            Ray newRay = Ray(newRayOrigin, newRayDir);
            Hit newH = Hit();
            Vector3f secondary_ray = traceRay(newRay, 0.0f, bounces - 1, newH);
            I += k_s * secondary_ray;
        }

        return I;
    }
    else
    {

        return _sceneCopy.getBackgroundColor(r.getDirection());
    };
}

Vector3f
Renderer::traceOriginalRay(const Ray &r,
                           float tmin,
                           int bounces,
                           Hit &h) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.
    Vector3f I = Vector3f(0.0f, 0.0f, 0.0f);

    if (_scene.getGroup()->intersect(r, tmin, h))
    {
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        Vector3f ambient = h.getMaterial()->getDiffuseColor() * _scene.getAmbientLight();
        Vector3f diff_spec = Vector3f(0.0f, 0.0f, 0.0f);

        if (_args.shadows)
        {
            for (size_t i = 0; i < _scene.getNumLights(); ++i)
            {
                Vector3f tolight;
                Vector3f intensity;
                float distToLight;

                Light *l = _scene.getLight(i);
                l->getIllumination(hitPoint, tolight, intensity, distToLight);
                Vector3f secondaryRayDir = tolight;
                Vector3f secondaryRayOrigin = hitPoint + EPSILON * secondaryRayDir;
                Hit hitObject = Hit();
                Ray pointToLightRay = Ray(secondaryRayOrigin, secondaryRayDir);
                if (_scene.getGroup()->intersect(pointToLightRay, EPSILON, hitObject))
                {
                    Vector3f hitObjectPoint = pointToLightRay.pointAtParameter(hitObject.getT());
                    float distToIntersection = (hitObjectPoint - secondaryRayOrigin).abs();
                    if (distToIntersection < std::numeric_limits<float>().max() && distToIntersection > distToLight)
                    {
                        diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
                    }
                }
                else
                {
                    diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
                }
            }
        }

        //shadows not enabled
        else
        {
            for (size_t i = 0; i < _scene.getNumLights(); ++i)
            {
                Vector3f tolight;
                Vector3f intensity;
                float distToLight;

                Light *l = _scene.getLight(i);
                l->getIllumination(hitPoint, tolight, intensity, distToLight);
                diff_spec += h.getMaterial()->shade(r, h, tolight, intensity);
            }
        }
        I += ambient + diff_spec; // direct illumination

        //recursive ray tracing
        if (bounces > 0)
        {
            Vector3f k_s = h.getMaterial()->getSpecularColor();
            Vector3f eye = r.getDirection();
            Vector3f newRayDir = (eye + 2.0f * (Vector3f::dot(-eye, h.getNormal().normalized())) * h.getNormal()).normalized();
            Vector3f newRayOrigin = hitPoint + EPSILON * newRayDir;
            Ray newRay = Ray(newRayOrigin, newRayDir);
            Hit newH = Hit();
            Vector3f secondary_ray = traceRay(newRay, 0.0f, bounces - 1, newH);
            I += k_s * secondary_ray;
        }
        // I += map->findRadiance(h, hitPoint);
        return I;
    }
    else
    {

        return _scene.getBackgroundColor(r.getDirection());
    };
}

Vector3f
Renderer::drawRadiance(const Ray &r,
                       float tmin,
                       Hit &h) const
{
    Vector3f I = Vector3f(0.0f, 0.0f, 0.0f);
    bool intersected = _sceneCopy.getGroup()->intersect(r, tmin, h);
    if (intersected)
    {
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        I = _map->findRadiance(h, hitPoint);
        return I;
    }
    else
    {
        return Vector3f(0.0f);
    }
}

Vector3f
Renderer::drawPhotons(const Ray &r,
                      float tmin,
                      Hit &h) const
{

    Vector3f I = Vector3f(0.0f, 0.0f, 0.0f);
    bool intersected = _sceneCopy.getGroup()->intersect(r, tmin, h);
    if (intersected)
    {
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        // I = h.getMaterial()->getDiffuseColor();
        I = Vector3f(1.0f);
        // I.print();
    }
    return I;
}

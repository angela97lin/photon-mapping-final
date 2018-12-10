#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"
#include "PhotonMap.h"
#include <limits>
#define EPSILON 0.001f
#define NUM_SAMPLES 9

Renderer::Renderer(const ArgParser &args) : _args(args),
                                            _scene(args.input_file),
                                            _sceneCopy(args.input_file)
{
}

void Renderer::Render()
{
    // First pass: generate photon map.
    map = new PhotonMap(_args, 40); // lol delete in destructor to not leak
    map->generateMap();

    int w = _args.width;
    int h = _args.height;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);
    Image pimage(w, h); // visualize photon map position
    Image rimage(w, h); // visualize radiance

    // loop through all the pixels in the image
    // generate all the samples

    // This look generates camera rays and calls traceRay.
    // It also write to the color, normal, and depth images.
    // You should understand what this code does.
    Camera *cam = _scene.getCamera();

    // note: although the following seems somewhat repetitive depending on
    // whether or not jitter is turned on, it speeds things up
    // significantly, when compared to checking for jittering within
    // the for loop :O

    if (_args.jitter)
    {
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                Vector3f avg_color;
                for (int sample = 0; sample < NUM_SAMPLES; ++sample)
                {
                    float LO = -0.4;
                    float HI = 0.4;
                    float rand_jitter = LO + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (HI - LO)));

                    float ndcy = (2 * ((y + rand_jitter) / (h - 1.0f)) - 1.0f);
                    float ndcx = (2 * ((x + rand_jitter) / (w - 1.0f)) - 1.0f);
                    // Use PerspectiveCamera to generate a ray.

                    Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
                    Hit h;
                    Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h);
                    avg_color += color;
                }

                avg_color = avg_color / (float)NUM_SAMPLES;
                image.setPixel(x, y, avg_color);
            }
        }
    }

    else
    {
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
    }

    // calculate normal and depth separately
    // for (int y = 0; y < h; ++y)
    // {
    //     float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
    //     for (int x = 0; x < w; ++x)
    //     {
    //         float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
    //         // Use PerspectiveCamera to generate a ray.
    //         // You should understand what generateRay() does.
    //         Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
    //         Hit h;
    //         Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h);

    //         nimage.setPixel(x, y, (h.getNormal() + 1.0f) / 2.0f);
    //         float range = (_args.depth_max - _args.depth_min);
    //         if (range)
    //         {
    //             dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));
    //         }
    //     }
    // }

    //_sceneCopy._group->m_members.clear();
    for (int i = 0; i < map->cloud.pts.size(); ++i)
    {
        Vector3f diffuse = map->cloud.pts[i]._power;
        Material m = Material(diffuse);
        Vector3f p = map->cloud.pts[i]._position;
        // p.print();
        Sphere *_s = new Sphere(p, 0.025f, &m);
        _sceneCopy.getGroup()->addObject((Object3D *)_s);
    }

    // visualize photon map
    for (int y = 0; y < h; ++y)
    {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x)
        {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
            Hit h;
            Vector3f color = tracePhotons(r, cam->getTMin(), h);
            pimage.setPixel(x, y, color);
        }
    }


    // visualize irradiance
    for (int y = 0; y < h; ++y)
    {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x)
        {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
            Hit h;
            Vector3f color = drawRadiance(r, cam->getTMin(), h);
            rimage.setPixel(x, y, color);
        }
    }

    // save the files
    if (_args.output_file.size())
    {
        image.savePNG(_args.output_file);
    }
    // if (_args.depth_file.size())
    // {
    //     dimage.savePNG(_args.depth_file);
    // }
    // if (_args.normals_file.size())
    // {
    //     nimage.savePNG(_args.normals_file);
    // }

    pimage.savePNG(_args.normals_file);
    rimage.savePNG(_args.depth_file);

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

        I += map->findRadiance(h, hitPoint, h.getNormal());

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
        I = map->findRadiance(h, hitPoint, h.getNormal());
        return I;
    }
    else
    {

        return Vector3f(0.0f);
    };
}

Vector3f
Renderer::tracePhotons(const Ray &r,
                       float tmin,
                       Hit &h) const
{

    Vector3f I = Vector3f(0.0f, 0.0f, 0.0f);
    bool intersected = _sceneCopy.getGroup()->intersect(r, tmin, h);
    if (intersected)
    {
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        I = h.getMaterial()->getDiffuseColor();
        return I;
    }
    else
    {

        return Vector3f(0.0f);
    };
}



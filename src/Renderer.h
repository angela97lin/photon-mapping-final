#ifndef RENDERER_H
#define RENDERER_H

#include <string>

#include "SceneParser.h"
#include "ArgParser.h"
#include "PhotonMap.h"

class Hit;
class Vector3f;
class Ray;

class Renderer
{
public:
  // Instantiates a renderer for the given scene.
  Renderer(const ArgParser &args);
  void Render();

private:
  Vector3f traceRay(const Ray &ray, float tmin, int bounces,
                    Hit &hit) const;

 Vector3f drawRadiance(const Ray &r,
                       float tmin,
                       Hit &h) const;

  Vector3f
  tracePhotons(const Ray &r,
               float tmin,
               Hit &h) const;
  ArgParser _args;
  SceneParser _scene;
  SceneParser _sceneCopy;

  PhotonMap *map;
};

#endif // RENDERER_H

#ifndef MATERIAL_H
#define MATERIAL_H

#include <cassert>
#include <vecmath.h>

#include "Ray.h"
#include "Image.h"
#include "Vector3f.h"

#include <string>

class Material
{
  public:
    Material(const Vector3f &diffuseColor, 
             const Vector3f &specularColor = Vector3f::ZERO, 
             float shininess = 0) :
        _diffuseColor(diffuseColor),
        _specularColor(specularColor),
        _shininess(shininess)
    { }

    const Vector3f & getDiffuseColor() const {
        return _diffuseColor;
    }

    const Vector3f & getSpecularColor() const {
        return _specularColor;
    }

    Vector3f shade(const Ray &ray,
        const Hit &hit,
        const Vector3f &dirToLight,
        const Vector3f &lightIntensity);

    // Material is considered diffusive if not specular at all.
    const bool isDiffusive() const {
        // return (_diffuseColor.absSquared() > 0); --> not true but was what caused 
        // todo: if photon is absorbed, then we want to check if the above condition is
        // true, and if so, store.
        // essentially, if a material has a diffusive component (> 0), then we want to store
        // the photon.
        // otherwise, the material must be entirely reflective, and we 
        // don't store (only bounce)

        return (_specularColor.absSquared() == 0);
    }


protected:

    Vector3f _diffuseColor;
    Vector3f _specularColor;
    float   _shininess;
};

#endif // MATERIAL_H

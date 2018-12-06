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

protected:

    Vector3f _diffuseColor;
    Vector3f _specularColor;
    float   _shininess;

    		// Material Name
		std::string name;
		// // Ambient Color
		// Vector3 Ka;
		// // Diffuse Color
		// Vector3 Kd;
		// // Specular Color
		// Vector3 Ks;

        
		// Specular Exponent
		float Ns;
		// Optical Density
		float Ni;
		// Dissolve
		float d;
		// Illumination
		int illum;
		// Ambient Texture Map
		std::string map_Ka;
		// Diffuse Texture Map
		std::string map_Kd;
		// Specular Texture Map
		std::string map_Ks;
		// Specular Hightlight Map
		std::string map_Ns;
		// Alpha Texture Map
		std::string map_d;
		// Bump Map
		std::string map_bump;
};

#endif // MATERIAL_H

#include "Material.h"


Vector3f Material::shade(const Ray &ray,
    const Hit &hit,
    const Vector3f &dirToLight,
    const Vector3f &lightIntensity)
{
    // TODO: implement Diffuse and Specular phong terms
    Vector3f dirToLightNormalized = dirToLight.normalized();
    Vector3f hitNormNormalized = hit.getNormal().normalized();
    Vector3f k_d = getDiffuseColor();
    Vector3f k_s = getSpecularColor();

    //Diffuse:
    float LN_angle = Vector3f::dot(dirToLightNormalized, hitNormNormalized);
    float clamped_d = std::max(LN_angle, 0.0f);

    float di_x = clamped_d * lightIntensity.x() * k_d.x();
    float di_y = clamped_d * lightIntensity.y() * k_d.y();
    float di_z = clamped_d * lightIntensity.z() * k_d.z();
    Vector3f d_i = Vector3f(di_x, di_y, di_z);

    //Specular:
    Vector3f eye = ray.getDirection();    
    Vector3f reflected_eye = eye + 2.0f * (Vector3f::dot(-eye, hit.getNormal())) * hit.getNormal();
    float LR_angle = Vector3f::dot(reflected_eye.normalized(), dirToLightNormalized);
    float clamped_s = std::powf(std::max(LR_angle, 0.0f), _shininess);
    Vector3f ray_travel = ray.pointAtParameter(hit.getT());
    
    float si_x = clamped_s * lightIntensity.x() * k_s.x();
    float si_y = clamped_s * lightIntensity.y() * k_s.y();
    float si_z = clamped_s * lightIntensity.z() * k_s.z();
    Vector3f s_i = Vector3f(si_x, si_y, si_z);

    //Calculate diffuse and specular components of phong model
    Vector3f illumination = s_i + d_i;
    return illumination;
}

#include "Object3D.h"

bool Sphere::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER

    // We provide sphere intersection code for you.
    // You should model other intersection implementations after this one.

    // Locate intersection point ( 2 pts )
    const Vector3f &rayOrigin = r.getOrigin(); //Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    Vector3f origin = rayOrigin - _center; //Ray origin in the sphere coordinate

    float a = dir.absSquared();
    float b = 2 * Vector3f::dot(dir, origin);
    float c = origin.absSquared() - _radius * _radius;

    // no intersection
    if (b * b - 4 * a * c < 0)
    {
        return false;
    }

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f * a);
    float tminus = (-b - d) / (2.0f * a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin))
    {
        return false;
    }

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin)
    {
        t = tminus;
    }

    // one intersection at the front. one at the back
    if ((tplus > tmin) && (tminus < tmin))
    {
        t = tplus;
    }

    if (t < h.getT())
    {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal = normal.normalized();
        h.set(t, this->material, normal);
        return true;
    }
    // END STARTER
    return false;
}

// Add object to group
void Group::addObject(Object3D *obj)
{
    m_members.push_back(obj);
}

// Return number of objects in group
int Group::getGroupSize() const
{
    return (int)m_members.size();
}

bool Group::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER
    // we implemented this for you
    bool hit = false;
    for (Object3D *o : m_members)
    {
        if (o->intersect(r, tmin, h))
        {
            hit = true;
        }
    }
    return hit;
    // END STARTER
}

Plane::Plane(const Vector3f &normal, float d, Material *m) : Object3D(m), _normal(normal), D(-d)
{
}

bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    float t = -(D + Vector3f::dot(r.getOrigin(), _normal)) / (Vector3f::dot(r.getDirection(), _normal));
    //check if not behind eye and closer than previous
    if (t > tmin && t < h.getT())
    {
        h.set(t, this->material, _normal);
        return true;
    }
    return false;
}

bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const
{
    Vector3f p_a = getVertex(0);
    Vector3f p_b = getVertex(1);
    Vector3f p_c = getVertex(2);

    Vector3f n_a = getNormal(0);
    Vector3f n_b = getNormal(1);
    Vector3f n_c = getNormal(2);

    Matrix3f A = Matrix3f((p_a - p_b), (p_a - p_c), r.getDirection(), true);
    Matrix3f A_inv = A.inverse();
    Vector3f b = p_a - r.getOrigin();
    Vector3f x = A_inv * b;

    float beta = x[0];
    float gamma = x[1];
    float t = x[2];
    float alpha = 1.0f - beta - gamma;

    Vector3f normal = (alpha * n_a + beta * n_b + gamma * n_c).normalized();

    if (beta >= 0.0f && gamma >= 0.0f && alpha >= 0.0f && t > tmin && t < h.getT() && beta + gamma <= 1.0f)
    {
        h.set(t, this->material, normal);
        return true;
    }
    return false;
}

Transform::Transform(const Matrix4f &m,
                     Object3D *obj) : _object(obj), _transform(m)
{
}

bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{

    // transform ray from world --> object
    // local -> world: multiply by M
    // world --> local: multiply by M-1
    // then just check for intersection
    Matrix4f trans_inv = _transform.inverse();
    Vector4f rayDir = Vector4f(r.getDirection(), 0.0f);
    Vector4f rayOrigin = Vector4f(r.getOrigin(), 1.0f);

    const Vector3f transformed_rayd = (trans_inv * rayDir).xyz();
    const Vector3f transformed_rayo = (trans_inv * rayOrigin).xyz();
    const Ray transformed_r = Ray(transformed_rayo, transformed_rayd);
    if (_object->intersect(transformed_r, tmin, h))
    {
        //calculate normal in world space
        Matrix4f trans_invtrans = trans_inv.transposed();
        Vector3f transformed_normal = (trans_invtrans * Vector4f(h.getNormal().normalized(), 0.0f)).xyz().normalized();
        h.set(h.getT(), _object->material, transformed_normal);
        return true;
    }

    return false;
}
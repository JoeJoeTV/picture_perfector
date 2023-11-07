#include <lightwave.hpp>

namespace lightwave {
class Sphere : public Shape {
    
public:
    Sphere(const Properties &properties) {
        
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        //define help variables for the calculaton of the intersection
        // using mitternachtsformula
        float a = 1.f;
        float b = 2 * (ray.direction.x() * ray.origin.x() 
                        + ray.direction.y() * ray.origin.y()
                        + ray.direction.z() * ray.origin.z());
        float distanceOfRayOriginToWorlOrigin = (ray.origin - Point(0.f,0.f,0.f)).length();
        float c = -1 + distanceOfRayOriginToWorlOrigin * distanceOfRayOriginToWorlOrigin;

        // there are two possible solution
        // float t1 = (-b + sqrt(b * b - 4.f * a * c)) / 2.f;
        // float t2 = (-b - sqrt(b * b - 4.f * a * c)) / 2.f;
        // check if they are valid by checking the term inside of sqrt()
        float aux = b * b - 4.f * a * c;
        float t = 0;
        if (aux < 0)
        {
            // no solution exists 
            return false;
        }
        else if (aux == 0)
        {
            // one solution exists
            t =  (-b) / 2.f;
        }
        else
        {
            // there are two solutions. Take the closer one i.e. the smaler t
            float t1 = (-b + sqrt(aux)) / 2.f;
            float t2 = (-b - sqrt(aux)) / 2.f;

            t =  min(t1, t2);
        }
        // discard the intersection if there was a closer one already or if it is to close
        if (t < Epsilon || t > its.t)
            return false;

        its.t = t;
        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(Point { -1, -1, -1 }, Point { +1, +1, 1 });
    }

    Point getCentroid() const override {
        return Point(0);
    }

    AreaSample sampleArea(Sampler &rng) const override {
        NOT_IMPLEMENTED
    }

    std::string toString() const override {
        return "Sphere[]";
    }
};
}
REGISTER_SHAPE(Sphere, "sphere")
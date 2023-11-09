#include <lightwave.hpp>

namespace lightwave {
class Sphere : public Shape {
    /**
     * @brief Constructs a surface event for a given position, used by @ref intersect to populate the @ref Intersection
     * and by @ref sampleArea to populate the @ref AreaSample .
     * @param surf The surface event to populate with texture coordinates, shading frame and area pdf
     * @param position The hitpoint (i.e., point in [-1,-1,0] to [+1,+1,0]), found via intersection or area sampling
     */
    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;
        
        // map the position from [-1,-1,0]..[+1,+1,0] to [0,0]..[1,1] by discarding the z component and rescaling
        // I did not yet adjust this!
        surf.uv.x() = (position.x() + 1) / 2;
        surf.uv.y() = (position.y() + 1) / 2;

        surf.frame.normal = Vector(position).normalized();
        surf.frame.tangent = surf.frame.normal.cross(Vector(1,0,0) + surf.frame.normal).normalized();
        surf.frame.bitangent = -1 * surf.frame.normal.cross(surf.frame.tangent).normalized();

        // TODO
        surf.pdf = 0;
    }
    
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
        if (aux < 0){
            // no solution exists 
            return false;
        } else if (aux == 0){
            // one solution exists
            t =  (-b) / 2.f;
        } else {
            // there are two solutions. Take the closer one i.e. the smaler t
            float t1 = (-b + sqrt(aux)) / 2.f;
            float t2 = (-b - sqrt(aux)) / 2.f;

            float smaler = min(t1, t2);
            float larger = max(t1, t2);
            if (smaler > 0) {
                t = smaler;
            } else if (larger > 0){
                t = larger;
            } else {
                return false;
            } 
        }

        // discard the intersection if there was a closer one already or if it is to close
        if (t < Epsilon || t > its.t)
            return false;

        its.t = t;
        Point position = ray(t);
        populate(its, position); // compute the shading frame, texture coordinates and area pdf (same as sampleArea)
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
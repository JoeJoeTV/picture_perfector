#include <lightwave.hpp>

namespace lightwave {

/// @brief Converts cartesian coordinates of a vector pointing from the center of a sphere to its surface to UV coordinates
/// @param cartesianPoint 
/// @return 2D Point containing UV of sphere
static Point2 getSphereUV(const Point& cartesianPoint) {
    float r = Vector(cartesianPoint).length();
    float theta = acos(cartesianPoint.y() / r);
    float phi = atan2(cartesianPoint.z(), cartesianPoint.x());

    return Point2(
        phi / (2 * Pi),
        (Pi - theta) / Pi
    );
}

/// @brief A Sphere with radius 1 centered at (0,0,0)
class Sphere : public Shape {
    /// @brief The center of the sphere
    Point m_center;

    /// @brief The radius of the sphere
    float m_radius;

    /**
     * @brief Constructs a surface event for a given position, used by @ref intersect to populate the @ref Intersection
     * and by @ref sampleArea to populate the @ref AreaSample .
     * @param surf The surface event to populate with texture coordinates, shading frame and area pdf
     * @param position The hitpoint (i.e., point in [-1,-1,0] to [+1,+1,0]), found via intersection or area sampling
     */
    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;
        
        surf.uv = getSphereUV(position);

        surf.frame.normal = (position - this->m_center).normalized();
        surf.frame.tangent = surf.frame.normal.cross(Vector{0.0f, 1.0f, 0.0f}).normalized();
        surf.frame.bitangent = surf.frame.normal.cross(surf.frame.tangent).normalized();

        surf.pdf = 1.f / (4.f * Pi);
    }

public:
    Sphere(const Properties &properties) {
        this->m_center = Point{0,0,0};
        this->m_radius = 1.0f;
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        
        // Calculate partial results
        const float b = (2 * ray.direction).dot(ray.origin - this->m_center);
        const float c = (ray.origin - this->m_center).lengthSquared() - sqr(this->m_radius);
        const float d = sqr(b) - 4 * c;

        // If partial result d is negative, there is no solution and thus no intersection
        if (d < 0) {
            return false;
        }

        // Calculate both hits
        const float t1 = (-b + sqrt(d)) / 2;
        const float t2 = (-b - sqrt(d)) / 2;


        // Determine which hit to choose if any
        float tClose;

        if (t1 < Epsilon) {
            if (t2 < Epsilon) {
                return false;
            } else {
                tClose = t2;
            }
        } else {
            if (t2 < Epsilon) {
                tClose = t1;
            } else {
                tClose = min(t1, t2);
            }
        }


        // If hit is farther away than previous hit or behind the camera,
        // discard it, as it won't be seen
        if ((tClose > its.t) or (tClose < 0)) {
            return false;
        }

        // Calculate hit point on sphere and normalize to make sure, point is exactly on the sphere
        const Vector hitPos = Vector(ray(tClose));
        const Point hitPoint = Point(hitPos.normalized());

        // We have found a successful hit, so update in intersection object
        its.t = tClose;

        populate(its, hitPoint);
        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(
                this->m_center + (Vector{-1, -1, -1} * this->m_radius),
                this->m_center + (Vector{1, 1, 1} * this->m_radius)
            );
    }

    Point getCentroid() const override {
        return this->m_center;
    }

    AreaSample sampleArea(Sampler &rng) const override {
        Point2 rnd = rng.next2D();
        Vector position = squareToUniformSphere(rnd);

        AreaSample sample;
        populate(sample, position); // compute the shading frame, texture coordinates and area pdf (same as intersection)
        return sample;
    }
    
    std::string toString() const override {
        return "Sphere[]";
    }
};

}

REGISTER_SHAPE(Sphere, "sphere")
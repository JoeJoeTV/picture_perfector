#include <lightwave.hpp>

//#define AUTODIFF_ENABLE_IMPLICIT_CONVERSION_REAL = 1
#include <autodiff/forward/real.hpp>
#include <autodiff/forward/real/eigen.hpp>
#include "sdf/sdfobject.hpp"

namespace lightwave {

/// @brief A shape defined by a signed distance function (SDF)
class SDFShape : public Shape {
    /// @brief Maximum amount of ray-marching steps to take before counting as no intersection
    int m_maxSteps;

    /// @brief The minimum distance to the SDF that counts as a hit
    float m_minDistance;

    /// @brief The epsilon used to estimate the normal vectors for hit points
    float m_normalEpsilon;

    /// @brief The actual sdf to use for distance estimation
    ref<SDFObject> m_sdfChild;

    /// @brief The Bunds of the SDF object. Will be pre-computed in constructor
    Bounds m_bounds;

    // See @file acces.hpp for comments
    float intersectBounds(const Ray &ray) const {
        const auto t1 = (this->m_bounds.min() - ray.origin) / ray.direction;
        const auto t2 = (this->m_bounds.max() - ray.origin) / ray.direction;

        const auto tNear = elementwiseMin(t1, t2).maxComponent();
        const auto tFar = elementwiseMax(t1, t2).minComponent();

        if (tFar < tNear)
            return Infinity;
        if (tFar < Epsilon)
            return Infinity;

        return tNear;
    }

public:
    SDFShape(const Properties &properties) {
        this->m_maxSteps = properties.get<int>("maxSteps", 50);
        this->m_minDistance = properties.get<float>("minDistance", 0.01f);
        this->m_normalEpsilon = this->m_minDistance;

        this->m_sdfChild = properties.getChild<SDFObject>();

        // Pre-compute bounding box
        this->m_bounds = this->m_sdfChild->getBoundingBox();

        logger(EInfo, "built bounding box for SDF with: min=%s max=%s", this->m_bounds.min(), this->m_bounds.max());
    }

    autodiff::real estimateDistance(const PointReal& p) const {
        return this->m_sdfChild->estimateDistance(p);
    }

    #define ADVANCE_MULTIPLIER 3

    /// @note Implementation inspired by https://www.youtube.com/watch?v=beNDx5Cvt7M
    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        // Calculate distance at ray origin to determine if we're inside or outside the SDF object and how close we are to the border
        const float originDist = static_cast<float>(this->estimateDistance(ray.origin.cast<autodiff::real>()));

        Ray marchRay = Ray(ray.origin, ray.direction, ray.depth);

        // If the ray depth is >= 1 or the distance is smaller than the minimal distance,
        // we advance the ray origin a bit in the ray direction to avoid self intersections
        if ((ray.depth >= 1) or (abs(originDist) < this->m_minDistance)) {
            marchRay = Ray(ray(this->m_minDistance * ADVANCE_MULTIPLIER), ray.direction, ray.depth);
        }

        // Check, if the ray hits the bounding box of the SDF object and return false if not, since then, no intersection can occur
        const float boundsT = intersectBounds(marchRay);

        if (boundsT == Infinity) {
            return false;
        }

        // If the distance is negative enough, we're inside the SDF object.
        // In this case, we negate every distance calculated, which inverts the SDF object
        // and helps with finding the nearest border intersection from inside
        float distMult = 1;

        float marchRayOriginDist = static_cast<float>(this->estimateDistance(marchRay.origin.cast<autodiff::real>()));

        if (marchRayOriginDist < 0) {
            distMult = -1;
        }

        // Start the ray-marching loop
        float marchedDist = 0.0f;
        int step;

        for (step = 0; step < this->m_maxSteps; step++) {
            // Calculate point at current march distance
            const PointReal marchPoint = marchRay(marchedDist).cast<autodiff::real>();
            // Caulculate/Estimate distance of current point to the SDF object
            const float distance = distMult * static_cast<float>(this->estimateDistance(marchPoint));

            // Check conditions for no intersection
            if ((its and (marchedDist > its.t))   // Hit would be obstructed by existing hit
                or (marchedDist >= Infinity)      // Marching out of bounds
                or ((marchedDist > boundsT) and (!this->m_bounds.includes(marchRay(marchedDist))))  // Hit would be outside of the bounding box of the SDF object
                ) {
                return false;
            }

            // If the absolute value of the distance is smaller than the minimal distance,
            // we have found a hit and can break out of the loop to handle the rest
            if (abs(distance) < this->m_minDistance) {
                break;
            }

            marchedDist += distance;
        }

        // If the maximum amount of steps was reached, we didn't find an intersection, so return false
        if (step >= this->m_maxSteps) {
            return false;
        }

        // Update intersection instance with updated values
        its.t = marchedDist;

        /// Store fraction of steps to maximum steps such that it can be visualized with the "sdf" integrator
        its.stats.sdfStepFraction = static_cast<float>(step) / this->m_maxSteps;

        /// We currently don't have any texture mapping, so the UV coordinates are constant
        its.uv[0] = 0;
        its.uv[1] = 0;

        /// Calculate hit point and normal vector by using derivatives in each direction
        PointReal hitPoint = marchRay(marchedDist).cast<autodiff::real>();

        its.position = hitPoint.cast<float>();

        // For convenience :)
        using autodiff::wrt;
        using autodiff::at;
        using autodiff::derivative;

        its.frame.normal = Vector(
            static_cast<float>(derivative([&](auto p){return distMult * this->estimateDistance(p);}, wrt(hitPoint.x()), at(hitPoint))),
            static_cast<float>(derivative([&](auto p){return distMult * this->estimateDistance(p);}, wrt(hitPoint.y()), at(hitPoint))),
            static_cast<float>(derivative([&](auto p){return distMult * this->estimateDistance(p);}, wrt(hitPoint.z()), at(hitPoint)))
        ).normalized();

        its.frame.tangent = its.frame.normal.cross(Vector(1.0f, 0.0f, 0.0f));

        // If both vectors are parallel (length of tangent is 0), use different coordinate
        if ((-Epsilon < its.frame.tangent.lengthSquared()) and (its.frame.tangent.lengthSquared() < Epsilon)) {
            its.frame.tangent = its.frame.normal.cross(Vector(1.0f, 1.0f, 0.0f));
        }

        // Tangent is not yet normalized, so do that
        its.frame.tangent = its.frame.tangent.normalized();
        its.frame.bitangent = its.frame.normal.cross(its.frame.tangent).normalized();

        its.pdf = 0.0f;

        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(this->m_bounds);
    }

    Point getCentroid() const override {
        return Point{0.0f, 0.0f, 0.0f};
    }

    AreaSample sampleArea(Sampler &rng) const override {
        NOT_IMPLEMENTED
    }
    
    std::string toString() const override {
        return tfm::format(
            "SDFShape[\n"
            "  maxSteps = %d,\n"
            "  minDistance = %f,\n"
            "  childSDF = %s,\n"
            "]",
            this->m_maxSteps,
            this->m_minDistance,
            indent(this->m_sdfChild)
        );
    }
};

}

REGISTER_SHAPE(SDFShape, "sdf")
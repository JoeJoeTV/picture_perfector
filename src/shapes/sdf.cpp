#include <lightwave.hpp>

//#define AUTODIFF_ENABLE_IMPLICIT_CONVERSION_REAL = 1
#include <autodiff/forward/real.hpp>
#include <autodiff/forward/real/eigen.hpp>
#include "sdf/sdfobject.hpp"

namespace lightwave {

autodiff::real f(const Point2Real v) {
    return Vector2Real(v).length();
}

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

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        float marchDistance = 0.0f;
        int steps;

        float boundsT = intersectBounds(ray);

        // If the ray doesn't hit the bounding box, we know it doesn't hit the SDF
        if (boundsT == Infinity) {
            return false;
        }

        for (steps = 0; steps < this->m_maxSteps; steps++) {
            PointReal currPoint = ray(marchDistance).cast<autodiff::real>();
            float distance = static_cast<float>(this->estimateDistance(currPoint));

            // Advance ray by calculated distance
            marchDistance += distance;

            // If distance is too large, we know that we don't have a (new) intersection and can directly abort
            if ((marchDistance > its.t) or (marchDistance >= Infinity) or (marchDistance < 0)) {
                return false;
            }

            // If hitpoint would be behind the bounding box, we don't need to check further
            // as the bounding box includes the object to render and thus the distance will only increase
            if ((marchDistance > boundsT) and (!this->m_bounds.includes(ray(marchDistance)))) {
                return false;
            }

            // If the distance is smaller than the threshold, we can count this as a hit on the object and return the intersection
            if (distance < this->m_minDistance) {
                break;
            }
        }

        if ((steps >= this->m_maxSteps) or (marchDistance <= this->m_minDistance)) {
            return false;
        }

        its.t = marchDistance;

        // Store fraction of steps to maximum steps such that it can be visualized with the "sdf" integrator
        its.stats.sdfStepFraction = static_cast<float>(steps) / this->m_maxSteps;

        // We currently don't have any texture mapping, so the UV coordinates are constant
        its.uv[0] = 0;
        its.uv[1] = 0;


        PointReal hitPoint = ray(marchDistance).cast<autodiff::real>();

        its.position = hitPoint.cast<float>();

        using autodiff::wrt;
        using autodiff::at;
        using autodiff::derivative;

        its.frame.normal = Vector(
            static_cast<float>(derivative([&](auto p){return this->estimateDistance(p);}, wrt(hitPoint.x()), at(hitPoint))),
            static_cast<float>(derivative([&](auto p){return this->estimateDistance(p);}, wrt(hitPoint.y()), at(hitPoint))),
            static_cast<float>(derivative([&](auto p){return this->estimateDistance(p);}, wrt(hitPoint.z()), at(hitPoint)))
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
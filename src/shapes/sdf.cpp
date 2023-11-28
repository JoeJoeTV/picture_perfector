#include <lightwave.hpp>
#include "sdf/sdfshape.hpp"

namespace lightwave {

/// @brief A Sphere with radius 1 centered at (0,0,0)
class SDF : public Shape {
    /// @brief Maximum amount of ray-marching steps to take before counting as no intersection
    int m_maxSteps;

    /// @brief The minimum distance to the SDF that counts as a hit
    float m_minDistance;

    /// @brief The epsilon used to estimate the normal vectors for hit points
    float m_normalEpsilon;

    /// @brief The actual sdf to use for distance estimation
    ref<SDFShape> m_sdfChild;

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
    SDF(const Properties &properties) {
        this->m_maxSteps = properties.get<int>("maxSteps", 50);
        this->m_minDistance = properties.get<float>("minDistance", 0.01f);
        this->m_normalEpsilon = this->m_minDistance;

        this->m_sdfChild = properties.getChild<SDFShape>();

        // Pre-compute bounding box
        this->m_bounds = this->m_sdfChild->getBoundingBox();
    }

    float estimateDistance(const Point p) const {
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

        // Set starting t to the Bounds distance
        marchDistance = boundsT;

        for (steps = 0; steps < this->m_maxSteps; steps++) {
            Point currPoint = ray(marchDistance);

            float distance = this->estimateDistance(currPoint);

            marchDistance += distance;

            // If distance is too large, we know that we don't have a (new) intersection and can directly abort
            if ((marchDistance > its.t) or (marchDistance >= Infinity) or (marchDistance < 0)) {
                return false;
            }

            // If the distance is smaller than the threshold, we can count this as a hit on the object and return the intersection
            if (distance < this->m_minDistance) {
                break;
            }
        }

        if (steps >= this->m_maxSteps) {
            return false;
        }

        its.t = marchDistance;

        Point hitP = ray(marchDistance);

        // Since we don't have texture coordinates, store amount of steps used in UV
        its.uv[0] = static_cast<float>(steps) / this->m_maxSteps;
        its.uv[1] = 0;

        its.position = hitP;

        its.frame.normal = Vector{
            this->estimateDistance(hitP + Vector{1.0, 0.0f, 0.0f} * this->m_normalEpsilon) - this->estimateDistance(hitP - Vector{1.0f, 0.0f, 0.0f} * this->m_normalEpsilon),
            this->estimateDistance(hitP + Vector{0.0f, 1.0f, 0.0f} * this->m_normalEpsilon) - this->estimateDistance(hitP - Vector{0.0f, 1.0f, 0.0f} * this->m_normalEpsilon),
            this->estimateDistance(hitP + Vector{0.0f, 0.0f, 1.0f} * this->m_normalEpsilon) - this->estimateDistance(hitP - Vector{0.0f, 0.0f, 1.0f} * this->m_normalEpsilon),
        }.normalized();
        its.frame.tangent = its.frame.normal.cross(Vector{1.0f, 0.0f, 1.0f}).normalized();

        if ((-Epsilon < its.frame.tangent.length()) and (its.frame.tangent.length() <= Epsilon)) {
            its.frame.tangent = its.frame.normal.cross(Vector{0.0f, 1.0f, 0.0f}).normalized();
        }

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
            "SDF[\n"
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

REGISTER_SHAPE(SDF, "sdf")
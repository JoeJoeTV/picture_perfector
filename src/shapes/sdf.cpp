#include <lightwave.hpp>
#include "sdf/sdfshape.hpp"

namespace lightwave {

static float sphereSDF(const Point p, const Point center, const float radius) {
    return (p - center).length() - radius;
} 

static float cubeSDF(const Point p, const Point corner) {
    Point absP = Point{abs(p.x()), abs(p.y()), abs(p.z())};

    Vector q = absP - corner;
    Vector maxQ0 = Vector{max(q.x(), 0.0f), max(q.y(), 0.0f), max(q.z(), 0.0f)};
    
    return maxQ0.length() + min(max(q.x(), max(q.y(), q.z())), 0.0f);
}

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
public:
    SDF(const Properties &properties) {
        this->m_maxSteps = properties.get<int>("maxSteps", 50);
        this->m_minDistance = properties.get<float>("minDistance", 0.01f);
        this->m_normalEpsilon = this->m_minDistance;

        this->m_sdfChild = properties.getChild<SDFShape>();
    }

    float estimateDistance(const Point p) const {
        //return max(cubeSDF(p, Point{1.5f, 1.0f, 0.5f}), sphereSDF(p, Point{0.0f, 0.0f, 0.0f}, 1.0f));
        static bool wasCalled = false;

        if (!wasCalled) {
            logger(EInfo, "[SDF] 'estimateDistance' called!");
            wasCalled = true;
        }

        return this->m_sdfChild->estimateDistance(p);
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        float marchDistance = 0.0f;
        int steps;

        for (steps = 0; steps < this->m_maxSteps; steps++) {
            if (marchDistance > its.t) {
                return false;
            }

            Point currPoint = ray(marchDistance);

            float distance = this->estimateDistance(currPoint);

            marchDistance += distance;

            if (distance < this->m_minDistance) {
                break;
            }
        }

        if (steps >= this->m_maxSteps) {
            return false;
        }

        its.t = marchDistance;

        Point hitP = ray(marchDistance);

        its.position = hitP;

        its.frame.normal = (Vector{1.0f, 1.0f, 1.0f} * (1.0-static_cast<float>(steps)/static_cast<float>(this->m_maxSteps)));

        its.frame.normal = Vector{
            this->estimateDistance(hitP + Vector{1.0, 0.0f, 0.0f} * this->m_normalEpsilon) - this->estimateDistance(hitP - Vector{1.0f, 0.0f, 0.0f} * this->m_normalEpsilon),
            this->estimateDistance(hitP + Vector{0.0f, 1.0f, 0.0f} * this->m_normalEpsilon) - this->estimateDistance(hitP - Vector{0.0f, 1.0f, 0.0f} * this->m_normalEpsilon),
            this->estimateDistance(hitP + Vector{0.0f, 0.0f, 1.0f} * this->m_normalEpsilon) - this->estimateDistance(hitP - Vector{0.0f, 0.0f, 1.0f} * this->m_normalEpsilon),
        }.normalized();

        its.pdf = 0.0f;

        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(
            Point{5.0f, 5.0f, 5.0f},
            Point{-5.0f, -5.0f, -5.0f}
        );
    }

    Point getCentroid() const override {
        return Point{0.0f, 0.0f, 0.0f};
    }

    AreaSample sampleArea(Sampler &rng) const override {
        NOT_IMPLEMENTED
    }
    
    std::string toString() const override {
        return "SDF[]";
    }
};

}

REGISTER_SHAPE(SDF, "sdf")
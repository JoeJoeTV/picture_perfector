#include <lightwave.hpp>

//#define AUTODIFF_ENABLE_IMPLICIT_CONVERSION_REAL = 1
#include <autodiff/forward/real.hpp>
#include <autodiff/forward/real/eigen.hpp>
#include "sdf/sdfobject.hpp"

namespace lightwave {

enum UVMapMode {
    NONE, SPHERE
};

/// @brief A shape defined by a signed distance function (SDF)
class SDFShape : public Shape {
    /// @brief Maximum amount of ray-marching steps to take before counting as no intersection
    int m_maxSteps;

    /// @brief The minimum distance to the SDF that counts as a hit
    float m_minDistance;

    /// @brief The epsilon used to estimate the normal vectors for hit points
    float m_normalEpsilon;

    /// @brief The mode used to map uv coordinates to shape
    UVMapMode m_uvMapMode;

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

    /// @brief Maps the given intersection to UV coordinates according to the specified @var m_uvMapMode
    /// @param its The intersection of the current hit
    /// @return Mapped UV coordinates
    Point2 mapUVCoordinates(const Intersection &its) const {
        switch (this->m_uvMapMode) {
        case UVMapMode::NONE:
            return Point2(0.0f, 0.0f);
        case UVMapMode::SPHERE:
            Vector dir = Vector(its.position).normalized();

            // Get theta angle (y points up, so take cos^-1 of y value)
            const float theta = acos(dir.y());
            const float phi = atan2(dir.z(), dir.x());

            return Point2((phi + Pi) * Inv2Pi, theta * InvPi);
        }
    }

    /// @brief Calculates the normal vector at the point @param hitPoint with relation to the SDF using the derivatives in each coordinate direction
    /// @param hitPoint The point at which to calculate the normal vector
    /// @return The calculated normal vector
    Vector deriveNormalVector(const Point &hitPoint) const {
        PointReal realHit = hitPoint.cast<autodiff::real>();

        // For convenience :)
        using autodiff::wrt;
        using autodiff::at;
        using autodiff::derivative;

        return Vector(
            static_cast<float>(derivative([&](auto p){return this->estimateDistance(p);}, wrt(realHit.x()), at(realHit))),
            static_cast<float>(derivative([&](auto p){return this->estimateDistance(p);}, wrt(realHit.y()), at(realHit))),
            static_cast<float>(derivative([&](auto p){return this->estimateDistance(p);}, wrt(realHit.z()), at(realHit)))
        ).normalized();
    }

public:
    SDFShape(const Properties &properties) {
        this->m_maxSteps = properties.get<int>("maxSteps", 50);
        this->m_minDistance = properties.get<float>("minDistance", 0.01f);
        this->m_normalEpsilon = this->m_minDistance;

        this->m_sdfChild = properties.getChild<SDFObject>();

        this->m_uvMapMode = properties.getEnum<UVMapMode>("mapMode", UVMapMode::NONE,
                {
                    {"sphere", UVMapMode::SPHERE},
                    {"none", UVMapMode::NONE}
                }
            );

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
        /// @brief The distance to the SDF object at the current march point
        /// @note Initialized with the distance to the origin of thee ray
        float distance = static_cast<float>(this->estimateDistance(ray.origin.cast<autodiff::real>()));

        /// @brief The ray whis is marched along
        Ray marchRay = Ray(ray.origin, ray.direction, ray.depth);

        // If the absolute distance is smaller than the minimal distance, we're on the surface of the SDF object
        if (abs(distance) < this->m_minDistance) {
            const Vector normal = deriveNormalVector(ray.origin);
            const float cosTheta =  1 - (Frame::cosTheta(normal) - Frame::cosTheta(ray.direction));
            marchRay = Ray(ray.origin + normal * cosTheta * (this->m_minDistance * ADVANCE_MULTIPLIER), ray.direction, ray.depth);
        }


        // Check, if the ray hits the bounding box of the SDF object and return false if not, since then, no intersection can occur
        const float boundsT = intersectBounds(marchRay);

        if (boundsT == Infinity) {
            return false;
        }

        // Start the ray-marching loop
        float marchedDist = 0.0f;
        int step = 1;

        for (; step < this->m_maxSteps; step++) {
            // Calculate point at current march distance
            const PointReal marchPoint = marchRay(marchedDist).cast<autodiff::real>();
            // Caulculate/Estimate distance of current point to the SDF object
            distance = std::copysign(std::max(abs(static_cast<float>(this->estimateDistance(marchPoint))), this->m_minDistance / 2), distance);

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

            marchedDist += abs(distance);
        }

        // If the maximum amount of steps was reached, we didn't find an intersection, so return false
        if (step >= this->m_maxSteps) {
            return false;
        }

        // Update intersection instance with updated values
        its.t = marchedDist;

        /// Store fraction of steps to maximum steps such that it can be visualized with the "sdf" integrator
        its.stats.sdfStepFraction = static_cast<float>(step) / this->m_maxSteps;

        /// Calculate hit point and normal vector by using derivatives in each direction
        Point hitPoint = marchRay(marchedDist);

        its.position = hitPoint;
        its.frame.normal = deriveNormalVector(hitPoint);

        its.frame.tangent = its.frame.normal.cross(Vector(1.0f, 0.0f, 0.0f));

        // If both vectors are parallel (length of tangent is 0), use different coordinate
        if ((-Epsilon < its.frame.tangent.lengthSquared()) and (its.frame.tangent.lengthSquared() < Epsilon)) {
            its.frame.tangent = its.frame.normal.cross(Vector(1.0f, 1.0f, 0.0f));
        }

        // Tangent is not yet normalized, so do that
        its.frame.tangent = its.frame.tangent.normalized();
        its.frame.bitangent = its.frame.normal.cross(its.frame.tangent).normalized();

        /// Map intersection to UV coordinates
        its.uv = mapUVCoordinates(its);

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
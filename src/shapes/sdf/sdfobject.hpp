#include <lightwave.hpp>

#include <autodiff/forward/real.hpp>
#include <autodiff/forward/real/eigen.hpp>

namespace lightwave {

class SDFObject : public Object {
public:

    const float BBCHECK_DISTANCE = 1e7;

    /// @brief Returns the distance to the SDF object from point @param p
    /// @param p The point at which to evaluate the distance
    /// @return The distance to the SDF object
    virtual autodiff::real estimateDistance(const PointReal& p) const = 0;

    /// @brief Calculates the axis aligned bounding box encapsulating the SDF object
    /// @return @class Bounds object represensing the axis aligned bounding box
    /// @note The default implmentation uses the @def estimateDistance function to create a bounding box
    virtual Bounds getBoundingBox() const {
        // Calculate max point in each direction
        const Vector maxDist = Vector(
            static_cast<float>(estimateDistance(PointReal(BBCHECK_DISTANCE, 0.0f, 0.0f))),
            static_cast<float>(estimateDistance(PointReal(0.0f, BBCHECK_DISTANCE, 0.0f))),
            static_cast<float>(estimateDistance(PointReal(0.0f, 0.0f, BBCHECK_DISTANCE)))
        );
        const Point maxP = Point(BBCHECK_DISTANCE) - maxDist;

        // Calculate min point in each direction
        const Vector minDist = Vector(
            static_cast<float>(estimateDistance(PointReal(-BBCHECK_DISTANCE, 0.0f, 0.0f))),
            static_cast<float>(estimateDistance(PointReal(0.0f, -BBCHECK_DISTANCE, 0.0f))),
            static_cast<float>(estimateDistance(PointReal(0.0f, 0.0f, -BBCHECK_DISTANCE)))
        );
        const Point minP = Point(-BBCHECK_DISTANCE) + minDist;

        return Bounds(minP, maxP);
    }

    std::string toString() const override {
        return "SDFObject[]";
    }
};
    
}
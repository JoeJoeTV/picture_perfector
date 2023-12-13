#include <lightwave.hpp>

namespace lightwave {

class SDFObject : public Object {
public:

    const float BBCHECK_DISTANCE = 1e7;

    /// @brief Returns the distance to the SDF object from point @param p
    /// @param p The point at which to evaluate the distance
    /// @return The distance to the SDF object
    virtual float estimateDistance(const Point p) const = 0;

    /// @brief Calculates the axis aligned bounding box encapsulating the SDF object
    /// @return @class Bounds object represensing the axis aligned bounding box
    /// @note The default implmentation uses the @def estimateDistance function to create a bounding box
    virtual Bounds getBoundingBox() const {
        // Calculate max point in each direction
        const Point maxP = Point(
            BBCHECK_DISTANCE - estimateDistance(Point(BBCHECK_DISTANCE, 0.0f, 0.0f)),
            BBCHECK_DISTANCE - estimateDistance(Point(0.0f, BBCHECK_DISTANCE, 0.0f)),
            BBCHECK_DISTANCE - estimateDistance(Point(0.0f, 0.0f, BBCHECK_DISTANCE))
        );

        // Calculate min point in each direction
        const Point minP = Point(
            -BBCHECK_DISTANCE + estimateDistance(Point(-BBCHECK_DISTANCE, 0.0f, 0.0f)),
            -BBCHECK_DISTANCE + estimateDistance(Point(0.0f, -BBCHECK_DISTANCE, 0.0f)),
            -BBCHECK_DISTANCE + estimateDistance(Point(0.0f, 0.0f, -BBCHECK_DISTANCE))
        );

        return Bounds(minP, maxP);
    }

    std::string toString() const override {
        return "SDFObject[]";
    }
};
    
}
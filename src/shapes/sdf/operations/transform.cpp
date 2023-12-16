#include <lightwave.hpp>
#include "../sdfobject.hpp"

namespace lightwave {

class SDFTransform : public SDFObject {
    /// @brief The SDF object that is transformed
    ref<SDFObject> m_child;

    /// @brief The transform to apply to the child SDF object
    ref<Transform> m_transform;

    /// @brief Separate scale factor, since it needs to be handled differently
    float m_scale;

public:
    SDFTransform(const Properties &properties) {
        this->m_child = properties.getChild<SDFObject>();
        this->m_transform = properties.getChild<Transform>();

        const Vector scale = this->m_transform->popScale();

        if ((Epsilon <= abs(scale.x() - scale.y())) or (Epsilon <= abs(scale.y() - scale.z())) or (Epsilon <= abs(scale.z() - scale.x()))) {
            lightwave_throw("Only uniform scaling is allowed for SDF transformation! Given: %s", scale);
        }

        this->m_scale = scale.x();
    }

    autodiff::real estimateDistance(const PointReal& p) const override {
        PointReal pt = this->m_transform->inverse(p);

        return this->m_child->estimateDistance(PointReal(VectorReal(pt) / this->m_scale)) * this->m_scale;
    }

    Bounds getBoundingBox() const override {
        const Bounds oldBounds = this->m_child->getBoundingBox();
        const Point oldMin = oldBounds.min();
        const Point oldMax = oldBounds.max();

        // Calculate new AABB for transformed bounding box
        std::vector<Point> tPoints;

        // Add all corner points to list
        tPoints.push_back(oldMin);                                      // 000
        tPoints.push_back(Point(oldMin.x(), oldMin.y(), oldMax.z()));   // 001
        tPoints.push_back(Point(oldMin.x(), oldMax.y(), oldMin.z()));   // 010
        tPoints.push_back(Point(oldMin.x(), oldMax.y(), oldMax.z()));   // 011
        tPoints.push_back(Point(oldMax.x(), oldMin.y(), oldMin.z()));   // 100
        tPoints.push_back(Point(oldMax.x(), oldMin.y(), oldMax.z()));   // 101
        tPoints.push_back(Point(oldMax.x(), oldMax.y(), oldMin.z()));   // 110
        tPoints.push_back(oldMax);                                      // 111

        Bounds tBounds;

        // Extend bounding box, until all transformed points fit in it
        for (const auto& p : tPoints) {
            const Point pt = Point(Vector(this->m_transform->apply(p)) * this->m_scale);

            tBounds.extend(pt);
        }

        return tBounds;
    }

    std::string toString() const override {
        return tfm::format(
            "SDFTransform[\n"
            "  child = %s,\n"
            "  transform = %s,\n"
            "  scale = %f,\n"
            "]",
            indent(this->m_child->toString()),
            indent(this->m_transform->toString()),
            this->m_scale
        );
    }
};
    
}

REGISTER_CLASS(SDFTransform, "sdf", "transform")
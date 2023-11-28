#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

class SDFCylinder : public SDFShape {
    /// @brief The radius which is used to smoothe the edges. Set to 0 to disable
    float m_edgeRadius;

    /// @brief The height of the cylinder
    float m_height;

    /// @brief The radius of the cylinder
    float m_radius;

    /// @brief Transform for SDF
    ref<Transform> m_transform;
public:
    SDFCylinder(const Properties &properties) {
        this->m_height = properties.get<float>("height");
        this->m_radius = properties.get<float>("radius");
        this->m_edgeRadius = properties.get<float>("edgeRadius", 0.0f);

        this->m_transform = properties.getOptionalChild<Transform>();
    }

    float estimateDistance(const Point p) const override {
        Point transP;

        if (this->m_transform) {
            transP = this->m_transform->inverse(p);
        } else {

            transP = p;
        }

        Vector2 d = Vector2(abs(Vector2(transP.x(), transP.z()).length()), abs(transP.y())) - Vector2(this->m_radius, this->m_height);

        return min(max(d.x(), d.y()), 0.0f) + Vector2(max(d.x(), 0.0f), max(d.y(), 0.0f)).length() - this->m_edgeRadius;
    }

    Bounds getBoundingBox() const override {
        Point maxP = Point(0.0f, 0.0f, 0.0f) + Vector(this->m_radius, this->m_height, this->m_radius) + Vector(this->m_edgeRadius);
        Point minP = Point(0.0f, 0.0f, 0.0f) - Vector(this->m_radius, this->m_height, this->m_radius) - Vector(this->m_edgeRadius);

        if (this->m_transform) {
            maxP = this->m_transform->apply(maxP);
            minP = this->m_transform->apply(minP);
        }

        return Bounds(minP, maxP);
    }

    std::string toString() const override {
        return tfm::format(
            "SDFCylinder[\n"
            "  height = %f,\n"
            "  radius = %f,\n"
            "  edgeRadius = %f,\n"
            "]",
            this->m_height,
            this->m_radius,
            this->m_edgeRadius
        );
    }
};
    
}

REGISTER_CLASS(SDFCylinder, "sdf", "cylinder")
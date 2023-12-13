#include <lightwave.hpp>
#include "../sdfobject.hpp"

namespace lightwave {

class SDFCylinder : public SDFObject {
    /// @brief The height of the cylinder SDF
    float m_height;

    /// @brief The radius of the cylinder SDF
    float m_radius;

public:
    SDFCylinder(const Properties &properties) {
        this->m_height = properties.get<float>("height", 1.0f);
        this->m_radius = properties.get<float>("radius", 1.0f);
    }

    float estimateDistance(const Point p) const override {
        Vector2 d = Vector2(abs(Vector2(p.x(), p.z()).length()), abs(p.y())) - Vector2(this->m_radius, this->m_height);

        return min(max(d.x(), d.y()), 0.0f) + Vector2(max(d.x(), 0.0f), max(d.y(), 0.0f)).length();
    }

    Bounds getBoundingBox() const override {
        return Bounds(
            Point(-this->m_radius, -this->m_height, -this->m_radius),
            Point(this->m_radius, this->m_height, this->m_radius)
        );
    }

    std::string toString() const override {
        return tfm::format(
            "SDFCylinder[\n"
            "  height = %f,\n"
            "  radius = %f,\n"
            "]",
            this->m_height,
            this->m_radius
        );
    }
};
    
} // namespace lightwave

REGISTER_CLASS(SDFCylinder, "sdf", "cylinder")
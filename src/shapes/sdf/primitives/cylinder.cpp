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

    autodiff::real estimateDistance(const PointReal& p) const override {
        Vector2Real d = Vector2Real(abs(Vector2Real(p.x(), p.z()).length()), abs(p.y())) - Vector2Real(this->m_radius, this->m_height);

        return min(max(d.x(), d.y()), 0.0f) + Vector2Real(max(d.x(), 0.0f), max(d.y(), 0.0f)).length();
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
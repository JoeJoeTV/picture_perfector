#include <lightwave.hpp>
#include "../sdfobject.hpp"

namespace lightwave {

class SDFSphere : public SDFObject {
    /// @brief The radius of the sphere SDF
    float m_radius;

public:
    SDFSphere(const Properties &properties) {
        this->m_radius = properties.get<float>("radius", 1.0f);
    }

    float estimateDistance(const Point p) const override {
        return (p - Point{0.0f, 0.0f, 0.0f}).length() - this->m_radius;
    }

    Bounds getBoundingBox() const override {
        return Bounds(
            Point(-this->m_radius, -this->m_radius, -this->m_radius),
            Point(this->m_radius, this->m_radius, this->m_radius)
        );
    }

    std::string toString() const override {
        return tfm::format(
            "SDFSphere[\n"
            "  radius = %f,\n"
            "]",
            this->m_radius
        );
    }
};
    
} // namespace lightwave

REGISTER_CLASS(SDFSphere, "sdf", "sphere")
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

    autodiff::real estimateDistance(const PointReal& p) const override {
        return VectorReal(p).length() - this->m_radius;
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
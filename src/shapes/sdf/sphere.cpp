#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

class SDFSphere : public SDFShape {
    /// @brief The radius of the sphere SDF
    float m_radius;

    /// @brief Transform for SDF
    ref<Transform> m_transform;
public:
    SDFSphere(const Properties &properties) {
        this->m_radius = properties.get<float>("radius", 1.0f);
        this->m_transform = properties.getOptionalChild<Transform>();
    }

    float estimateDistance(const Point p) const override {
        Point transP;

        if (this->m_transform) {
            transP = this->m_transform->inverse(p);
        } else {
            transP = p;
        }
        

        return (transP - Point{0.0f, 0.0f, 0.0f}).length() - this->m_radius;
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
    
}

REGISTER_CLASS(SDFSphere, "sdf", "sphere")
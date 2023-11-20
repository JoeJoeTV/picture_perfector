#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

class SDFSphere : public SDFShape {
    /// @brief The radius of the sphere SDF
    float m_radius;
public:
    SDFSphere(const Properties &properties) {
        this->m_radius = properties.get<float>("radius", 1.0f);
    }

    float estimateDistance(const Point p) const override {
        static bool wasCalled = false;

        if (!wasCalled) {
            logger(EInfo, "[SDFSphere] 'estimateDistance' called!");
            wasCalled = true;
        }

        return (p - Point{0.0f, 0.0f, 0.0f}).length() - this->m_radius;
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
#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

class SDFMin : public SDFShape {
    /// @brief SDF Shapes combines using this min pseudo shape
    ref<SDFShape> m_leftSDF;
    ref<SDFShape> m_rightSDF;
public:
    SDFMin(const Properties &properties) {
        const std::vector<ref<SDFShape>> sdf_children = properties.getChildren<SDFShape>();

        if (sdf_children.size() == 2) {
            this->m_leftSDF = sdf_children[0];
            this->m_rightSDF = sdf_children[1];
        } else {
            lightwave_throw("Exactly 2 SDFs required for 'min', %d given!", sdf_children.size());
        }
    }

    float estimateDistance(const Point p) const override {
        static bool wasCalled = false;

        if (!wasCalled) {
            logger(EInfo, "[SDFMin] 'estimateDistance' called!");
            wasCalled = true;
        }

        return min(this->m_leftSDF->estimateDistance(p), this->m_rightSDF->estimateDistance(p));
    }

    std::string toString() const override {
        return tfm::format(
            "SDFMin[\n"
            "  left = %s,\n"
            "  right = %s,\n"
            "]",
            indent(this->m_leftSDF->toString()),
            indent(this->m_rightSDF->toString())
        );
    }
};
    
}

REGISTER_CLASS(SDFMin, "sdf", "min")
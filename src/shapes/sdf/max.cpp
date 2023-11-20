#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

class SDFmax : public SDFShape {
    /// @brief SDF Shapes combines using this min pseudo shape
    ref<SDFShape> m_leftSDF;
    ref<SDFShape> m_rightSDF;
public:
    SDFmax(const Properties &properties) {
        const std::vector<ref<SDFShape>> sdf_children = properties.getChildren<SDFShape>();

        if (sdf_children.size() == 2) {
            this->m_leftSDF = sdf_children[0];
            this->m_rightSDF = sdf_children[1];
        } else {
            lightwave_throw("Exactly 2 SDFs required for 'min', %d given!", sdf_children.size());
        }
    }

    float estimateDistance(const Point p) const override {
        return max(this->m_leftSDF->estimateDistance(p), this->m_rightSDF->estimateDistance(p));
    }

    std::string toString() const override {
        return tfm::format(
            "SDFmax[\n"
            "  left = %s,\n"
            "  right = %s,\n"
            "]",
            indent(this->m_leftSDF->toString()),
            indent(this->m_rightSDF->toString())
        );
    }
};
    
}

REGISTER_CLASS(SDFmax, "sdf", "max")
#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

enum CombineMode {
    UNION, SUB, INTERSECT, XOR
};

class SDFCombine : public SDFShape {
    /// @brief SDF Shapes combines using this min pseudo shape
    ref<SDFShape> m_leftSDF;
    ref<SDFShape> m_rightSDF;

    CombineMode m_combineMode;
public:
    SDFCombine(const Properties &properties) {
        const std::vector<ref<SDFShape>> sdf_children = properties.getChildren<SDFShape>();

        if (sdf_children.size() == 2) {
            this->m_leftSDF = sdf_children[0];
            this->m_rightSDF = sdf_children[1];
        } else {
            lightwave_throw("Exactly 2 SDFs required for 'combine', %d given!", sdf_children.size());
        }

        this->m_combineMode = properties.getEnum<CombineMode>("mode", CombineMode::UNION,
                {
                    {"union", CombineMode::UNION},
                    {"sub", CombineMode::SUB},
                    {"intersect", CombineMode::INTERSECT},
                    {"xor", CombineMode::XOR},
                }
            );
    }

    float estimateDistance(const Point p) const override {
        const float EDLeft = this->m_leftSDF->estimateDistance(p);
        const float EDRight = this->m_rightSDF->estimateDistance(p);
        
        switch (this->m_combineMode) {
        case CombineMode::UNION:
            return min(EDLeft, EDRight);
        case CombineMode::SUB:
            return max(-EDLeft, EDRight);
        case CombineMode::INTERSECT:
            return max(EDLeft, EDRight);
        case CombineMode::XOR:
            return max(min(EDLeft, EDRight), -min(EDLeft, EDRight));
        default:
            return 0;
        }
    }

    std::string toString() const override {
        return tfm::format(
            "SDFCombine[\n"
            "  left = %s,\n"
            "  right = %s,\n"
            "  mode = %s,\n"
            "]",
            indent(this->m_leftSDF->toString()),
            indent(this->m_rightSDF->toString()),
            indent(this->m_combineMode)
        );
    }
};
    
}

REGISTER_CLASS(SDFCombine, "sdf", "combine")
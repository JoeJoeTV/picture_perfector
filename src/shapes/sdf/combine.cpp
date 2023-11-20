#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

enum CombineMode {
    UNION, SUB, INTERSECT
};

// See https://iquilezles.org/articles/distfunctions/
float smoothUnion(const float d1, const float d2, const float k) {
    float h = max(k - abs(d1-d2), 0.0f);
    return min(d1, d2) - h*h*0.25/k;
}

class SDFCombine : public SDFShape {
    /// @brief SDF Shapes combines using this min pseudo shape
    ref<SDFShape> m_leftSDF;
    ref<SDFShape> m_rightSDF;

    /// @brief Describes how the two SDFs should be combined
    CombineMode m_combineMode;

    /// @brief Wether to use smooth union, intersection and subtraction operations
    bool m_smooth;

    /// @brief The size of the smoothing
    float m_smooth_size;
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
                }
            );
        
        this->m_smooth = properties.get<bool>("smooth", false);
        this->m_smooth_size = properties.get<float>("k", 1.0f);
    }

    float estimateDistance(const Point p) const override {
        const float EDLeft = this->m_leftSDF->estimateDistance(p);
        const float EDRight = this->m_rightSDF->estimateDistance(p);
        
        if (this->m_smooth) {
            switch (this->m_combineMode) {
            case CombineMode::UNION:
                return smoothUnion(EDLeft, EDRight, this->m_smooth_size);
            case CombineMode::SUB:
                return -smoothUnion(EDLeft, -EDRight, this->m_smooth_size);
            case CombineMode::INTERSECT:
                return -smoothUnion(-EDLeft, -EDRight, this->m_smooth_size);
            default:
                return 0;
            }
        } else {
            switch (this->m_combineMode) {
            case CombineMode::UNION:
                return min(EDLeft, EDRight);
            case CombineMode::SUB:
                return max(-EDLeft, EDRight);
            case CombineMode::INTERSECT:
                return max(EDLeft, EDRight);
            default:
                return 0;
            }
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
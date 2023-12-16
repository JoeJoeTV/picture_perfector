#include <lightwave.hpp>
#include "../sdfobject.hpp"

namespace lightwave {

class SDFThicken : public SDFObject {
    /// @brief The SDF object that is thickend
    ref<SDFObject> m_child;

    /// @brief By how much the SDF child object should be made thicker
    float m_amount;

public:
    SDFThicken(const Properties &properties) {
        this->m_child = properties.getChild<SDFObject>();
        this->m_amount = properties.get<float>("amount");
    } 

    autodiff::real estimateDistance(const PointReal& p) const override {
        return this->m_child->estimateDistance(p) - this->m_amount;
    }

    Bounds getBoundingBox() const override {
        const Bounds oldBounds = this->m_child->getBoundingBox();
        const Vector oldMin = Vector(oldBounds.min());
        const Vector oldMax = Vector(oldBounds.max());

        return Bounds(Point(oldMin - Vector(this->m_amount)), Point(oldMax + Vector(this->m_amount)));
    }

    std::string toString() const override {
        return tfm::format(
            "SDFThicken[\n"
            "  child = %s,\n"
            "  amount = %f,\n"
            "]",
            indent(this->m_child->toString()),
            this->m_amount
        );
    }
};
    
}

REGISTER_CLASS(SDFThicken, "sdf", "thicken")
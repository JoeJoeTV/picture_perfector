#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {
    /// @brief The shape representing the area light
    ref<Instance> m_instance;

public:
    AreaLight(const Properties &properties) {
        this->m_instance = properties.getChild<Instance>();
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        AreaSample sample = this->m_instance->sampleArea(rng);  

        const Vector wi = (sample.position - origin).normalized();
        const float distance = (sample.position - origin).length();

        Color intensity = m_instance->emission()->evaluate(sample.uv, sample.frame.toLocal(-1*wi)).value;

        return DirectLightSample{
            .wi = wi,
            .weight = intensity,
            .distance = distance
        };

    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "AreaLight[\n");
    }
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "area")

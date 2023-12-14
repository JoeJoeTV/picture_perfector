#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {
    /// @brief The shape representing the area light
    ref<Instance> m_instance;
    //ref<Emission> m_emission;

public:
    AreaLight(const Properties &properties) {
        this->m_instance = properties.getChild<Instance>();
        //this->m_emission = properties.getChild<Emission>();
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        AreaSample sample = this->m_instance->sampleArea(rng);  

        const Vector wi = (sample.position - origin).normalized();
        const float distance = (sample.position - origin).length();

        // const Color intensity = sample.instance->emission()->evaluate(sample.uv, wi).value;
        const Color intensity = m_instance->emission()->evaluate(sample.uv, wi).value;

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

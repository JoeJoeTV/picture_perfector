#include <lightwave.hpp>

namespace lightwave {

class DirectionalLight final : public Light {
    /// @brief The intensity of the light emitted from the light source
    Color m_intensity;

    /// @brief The direction in which the light is emitted
    Vector m_direction;
public:
    DirectionalLight(const Properties &properties) {
        this->m_intensity = properties.get<Color>("intensity");
        this->m_direction = properties.get<Vector>("direction").normalized();
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        const Vector wi = this->m_direction;

        return DirectLightSample{
            .wi = wi,
            .weight = this->m_intensity,
            .distance = Infinity
        };

    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "DirectionalLight[\n"
            "  intensity = %s,\n"
            "  direction = %s,\n"
            "]",
            this->m_intensity,
            this->m_direction);
    }
};

} // namespace lightwave

REGISTER_LIGHT(DirectionalLight, "directional")

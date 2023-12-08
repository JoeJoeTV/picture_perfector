#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {
    /// @brief The power/flux emitted from the point light
    Color m_power;

    /// @brief The position at which the light source resides
    Point m_position;
public:
    PointLight(const Properties &properties) {
        this->m_power = properties.get<Color>("power");
        this->m_position = properties.get<Point>("position");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        const Vector wi = (this->m_position - origin).normalized();
        const float distance = (this->m_position - origin).length();

        // Since the point light source is emitting light in every direction,
        // we divide the power output by 4π to get the intensity
        const Color intensity = this->m_power / (4 * Pi);

        return DirectLightSample{
            .wi = wi,
            .weight = intensity / sqr(distance),
            .distance = distance
        };

    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "PointLight[\n"
            "  power = %s,\n"
            "  position = %s,\n"
            "]",
            this->m_power,
            this->m_position);
    }
};

} // namespace lightwave

REGISTER_LIGHT(PointLight, "point")

#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting or refracting `wo' is zero, hence we can
        // just ignore that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {

        // ior is given as etaI / etaE , where etaI is the refraction index of the material
        // and etaE is the refraction index of the outside (normally air)
        const float ior = this->m_ior->scalar(uv);

        // The direction vectors of the reflection and refraction
        // based upon if we're entering the metarial or exiting it
        Vector vecReflected;
        Vector vecRefracted;

        // The relative reflection index based upon if we're entering or exiting the material
        float eta;

        const float cosThetaI = Frame::cosTheta(wo);

        if (cosThetaI >= 0) {
            // Transition: outside -> material
            eta = ior;
            vecReflected = reflect(wo, Vector(0.0f, 0.0f, 1.0f));
            vecRefracted = refract(wo, Vector(0.0f, 0.0f, 1.0f), eta);
        } else {
            // Transition: material -> outside
            eta = 1/ior;
            vecReflected = reflect(wo, Vector(0.0f, 0.0f, -1.0f));
            vecRefracted = refract(wo, Vector(0.0f, 0.0f, -1.0f), eta);
        }

        // Calculate the fresnel term and use it as a probability to choose reflection or refraction
        const float fresnel = fresnelDielectric(cosThetaI, eta);

        if (rng.next() < fresnel) {
            return BsdfSample{
                .wi = vecReflected,
                .weight = this->m_reflectance->evaluate(uv)
            };
        } else {
            return BsdfSample{
                .wi = vecRefracted,
                .weight = this->m_transmittance->evaluate(uv) / sqr(eta)
            };
        }
    }

    std::string toString() const override {
        return tfm::format("Dielectric[\n"
                           "  ior           = %s,\n"
                           "  reflectance   = %s,\n"
                           "  transmittance = %s\n"
                           "]",
                           indent(m_ior), indent(m_reflectance),
                           indent(m_transmittance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Dielectric, "dielectric")

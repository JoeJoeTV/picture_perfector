#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughConductor : public Bsdf {
    ref<Texture> m_reflectance;
    ref<Texture> m_roughness;

public:
    RoughConductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
        m_roughness   = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // Using the squared roughness parameter results in a more gradual
        // transition from specular to rough. For numerical stability, we avoid
        // extremely specular distributions (alpha values below 10^-3)
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        // Calculate microfacet normal from wi and wo
        const Vector n = (wi + wo).normalized();

        // Calculate BSDF term
        const Color a = this->m_reflectance->evaluate(uv)
                        * microfacet::evaluateGGX(alpha, n)
                        * microfacet::smithG1(alpha, n, wi)
                        * microfacet::smithG1(alpha, n, wo);
        const float b = 4  * abs(Frame::cosTheta(wo));
        const Color weight = (a / b);
        
        return BsdfEval{
            .value = weight
        };

        // hints:
        // * the microfacet normal can be computed from `wi' and `wo'
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        // Sample random microfacet normal vector
        const Vector n = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());

        // Reflect wo at sampled normal vector to get wi
        const Vector wi = reflect(wo, n);

        // Calculate weight using BSDF function, PDF of sampling function and 
        // TODO: Add more notes about what cancels out + Find out why cos(theta:i) cancels out
        const Color weight = this->m_reflectance->evaluate(uv) * microfacet::smithG1(alpha, n, wi);

        return BsdfSample{
            .wi = wi,
            .weight = weight
        };

        // hints:
        // * do not forget to cancel out as many terms from your equations as possible!
        //   (the resulting sample weight is only a product of two factors)
    }

    std::string toString() const override {
        return tfm::format("RoughConductor[\n"
                           "  reflectance = %s,\n"
                           "  roughness = %s\n"
                           "]",
                           indent(m_reflectance), indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughConductor, "roughconductor")

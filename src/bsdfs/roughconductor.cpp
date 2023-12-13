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

        Vector halfvector = (wo + wi).normalized();
  
        Color numerator = this->m_reflectance->evaluate(uv) *
                            microfacet::evaluateGGX(alpha, halfvector) * 
                            microfacet::smithG1(alpha, halfvector, wo) *
                            microfacet::smithG1(alpha, halfvector, wi);
          
        // the forshortening cosTheta(wi) cancels in denominator
        float denominator = 4*Frame::cosTheta(wo);
        
        return BsdfEval{
            .value = numerator/denominator,
        };
        // hints:
        // * the microfacet normal can be computed from `wi' and `wo'
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        // sample for a random visibel normal.
        Vector halfvector = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D()).normalized();
        // the resulting vector when reflecting at the microfacet
        Vector wi = reflect(wo, halfvector).normalized();

        Color weight = this->m_reflectance->evaluate(uv) * microfacet::smithG1(alpha, halfvector, wi);
        
        return BsdfSample{
            .wi = wi,
            .weight = weight,
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

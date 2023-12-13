#include <lightwave.hpp>

#include "fresnel.hpp"
#include "microfacet.hpp"

namespace lightwave {

struct DiffuseLobe {
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        const float foreshortening = Frame::cosTheta(wi);
        const Color weight = (this->color / Pi) * foreshortening;

        return BsdfEval{
            .value = weight
        };

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        Vector wi = squareToCosineHemisphere(rng.next2D()).normalized();

        return BsdfSample{
            .wi = wi,
            .weight = this->color
        };

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
    }
};

struct MetallicLobe {
    float alpha;
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        Vector halfvector = (wo + wi).normalized();
  
        Color numerator = this->color *
                            microfacet::evaluateGGX(this->alpha, halfvector) * 
                            microfacet::smithG1(this->alpha, halfvector, wo) *
                            microfacet::smithG1(this->alpha, halfvector, wi);
        // the forshortening cosTheta(wi) cancels in denominator
        float denominator = 4*Frame::cosTheta(wo);
        
        return BsdfEval{
            .value = (numerator/denominator)
        };
        // hints:
        // * copy your roughconductor bsdf evaluate here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        // sample for a random visibel normal.
        Vector halfvector = microfacet::sampleGGXVNDF(this->alpha, wo, rng.next2D()).normalized();
        // the resulting vector when reflecting at the microfacet
        Vector wi = reflect(wo, halfvector).normalized();

        Color weight = this->color * microfacet::smithG1(this->alpha, halfvector, wi);
        
        return BsdfSample{
            .wi = wi,
            .weight = weight,
        };
        // hints:
        // * copy your roughconductor bsdf sample here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
    }
};

class Principled : public Bsdf {
    ref<Texture> m_baseColor;
    ref<Texture> m_roughness;
    ref<Texture> m_metallic;
    ref<Texture> m_specular;

    struct Combination {
        float diffuseSelectionProb;
        DiffuseLobe diffuse;
        MetallicLobe metallic;
    };

    Combination combine(const Point2 &uv, const Vector &wo) const {
        const auto baseColor = m_baseColor->evaluate(uv);
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const auto specular = m_specular->scalar(uv);
        const auto metallic = m_metallic->scalar(uv);
        const auto F =
            specular * schlick((1 - metallic) * 0.08f, Frame::cosTheta(wo));

        const DiffuseLobe diffuseLobe = {
            .color = (1 - F) * (1 - metallic) * baseColor,
        };
        const MetallicLobe metallicLobe = {
            .alpha = alpha,
            .color = F * Color(1) + (1 - F) * metallic * baseColor,
        };

        return {
            .diffuseSelectionProb =
                diffuseLobe.color.mean() /
                (diffuseLobe.color.mean() + metallicLobe.color.mean()),
            .diffuse  = diffuseLobe,
            .metallic = metallicLobe,
        };
    }

public:
    Principled(const Properties &properties) {
        m_baseColor = properties.get<Texture>("baseColor");
        m_roughness = properties.get<Texture>("roughness");
        m_metallic  = properties.get<Texture>("metallic");
        m_specular  = properties.get<Texture>("specular");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        const auto combination = combine(uv, wo);
        // evaluate both bsdfs
        Color diffuseWeight = combination.diffuse.evaluate(wo, wi).value;
        Color metallicWeight = combination.metallic.evaluate(wo, wi).value;
        
        return BsdfEval{
            .value = diffuseWeight + metallicWeight
        };
        // hint: evaluate `combination.diffuse` and `combination.metallic` and
        // combine their results
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto combination = combine(uv, wo);
        float probOfDiffusionSample = rng.next();
        
        if (probOfDiffusionSample < combination.diffuseSelectionProb) {
            BsdfSample diffuseSample = combination.diffuse.sample(wo, rng);
            return BsdfSample{
                .wi = diffuseSample.wi,
                .weight = diffuseSample.weight / combination.diffuseSelectionProb
            };
        } else {
            BsdfSample metallicSample = combination.metallic.sample(wo, rng);
            return BsdfSample{
                .wi = metallicSample.wi,
                .weight = metallicSample.weight / (1-combination.diffuseSelectionProb)
            };
        }
        // hint: sample either `combination.diffuse` (probability
        // `combination.diffuseSelectionProb`) or `combination.metallic`
    }

    std::string toString() const override {
        return tfm::format("Principled[\n"
                           "  baseColor = %s,\n"
                           "  roughness = %s,\n"
                           "  metallic  = %s,\n"
                           "  specular  = %s,\n"
                           "]",
                           indent(m_baseColor), indent(m_roughness),
                           indent(m_metallic), indent(m_specular));
    }
};

} // namespace lightwave

REGISTER_BSDF(Principled, "principled")

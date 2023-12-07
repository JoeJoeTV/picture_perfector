#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        NOT_IMPLEMENTED
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // sample an outgoing ray wi where normal is in z direction 
        // Vector wi = squareToUniformHemisphere(rng.next2D());
        Vector wi = squareToCosineHemisphere(rng.next2D()).normalized();

        // calculate the weight of the ray
        // float foreshortening =  Frame::cosTheta(wi);
        // Color weight = ((m_albedo->evaluate(uv)/Pi) * foreshortening / uniformHemispherePdf());
        // The cosineHemispherePdf and the forshortening and the PIs cancel out
        Color weight = m_albedo->evaluate(uv);

        return BsdfSample{
            .wi = wi,
            .weight = weight
        };
    }

    std::string toString() const override {
        return tfm::format("Diffuse[\n"
                           "  albedo = %s\n"
                           "]",
                           indent(m_albedo));
    }
};

} // namespace lightwave

REGISTER_BSDF(Diffuse, "diffuse")

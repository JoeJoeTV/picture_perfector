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
        //Vector wi = squareToUniformHemisphere(rng.next2D());
        Vector wi = squareToCosineHemisphere(rng.next2D()).normalized();

        // calculate the weight of the ray
        // scale it by the weight of the sample f(wi,wo) = albedo / PI
        // propability of a ray along the hemisphere is 1/2*PI
        // sample with cosine weighted hemisphere instead of uniform
        // finaly, account for foreshortening with cos(wi) = wi.dot(normal) / (len(wi) * len(normal))
        float foreshortening = wi.dot(Vector(0,0,1));
        //Color weight = ((m_albedo->evaluate(uv)/Pi) * foreshortening / uniformHemispherePdf());
        // add a small value in order to avoid division by zero
        Color weight = ((m_albedo->evaluate(uv)/Pi) * foreshortening / (cosineHemispherePdf(wi)+1e-8f));

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

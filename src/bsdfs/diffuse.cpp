#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // sample an outgoing ray w1 where normal is in z direction 
        Vector wi = squareToCosineHemisphere(rng.next2D());

        // calculate the weight of the ray
        // scale it by the weight of the sample f(w1,wi) = albedo / PI
        // propability of a ray along the hemisphere is 1/2*PI
        // sample with cosine weighted hemisphere instead of uniform
        // finaly, account for foreshortening with cos(wi) = wi.dot(normal) / (len(wi) * len(normal))
        float foreshortening = wi.dot(Vector(0,0,1));
        Color weight = (m_albedo->evaluate(uv) * (2*Pi * foreshortening)) / Pi;

        return BsdfSample{
            .wi = wi.normalized(),
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

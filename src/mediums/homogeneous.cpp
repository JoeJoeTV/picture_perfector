#include <lightwave.hpp>

namespace lightwave {

class HomogeneousMedium : public Medium {
    float m_sigmaT;
    float m_sigmaS;
    Color m_color;

public:
    HomogeneousMedium(const Properties &properties) {
        m_sigmaT = properties.get<float>("sigmaT");
        m_color = properties.get<Color>("color", Color(0));
    }

    float Tr(const Ray &ray, const float tIntersection, Sampler &rng) const override {
        // implement beer's law
        float distance = (ray.origin - ray(tIntersection)).length();
        float T = exp(-distance * m_sigmaT);

        // for now this is only absorption
        return T;
    }

    float sampleHitDistance(const Ray &ray, Sampler &rng) const override {
        // sample a distance at which we scatter in the volume
        return -std::log(1 - rng.next())/m_sigmaT;
    }

    float probabilityOfSampelingBeforeT(float t) const override {
        return  exp(-m_sigmaT * t);
    }

    float probabilityOfSampelingThisPoint(float t) const override {
        return m_sigmaT*exp(-m_sigmaT*t);
    }

    Color getColor() const override {
        return m_color;
    }

    float getSigmaS() const override {
        return m_sigmaS;
    }

    Vector samplePhase(Intersection &its, Sampler &rng) const override {
        return squareToUniformSphere(rng.next2D());
    }

    std::string toString() const override {
        return tfm::format("Homogeneous medium[\n"
                           "  density = %s\n"
                           "]",
                           indent(m_sigmaT));
    }
};

} // namespace lightwave

REGISTER_CLASS(HomogeneousMedium, "medium", "homogeneous")

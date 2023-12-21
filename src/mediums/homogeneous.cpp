#include <lightwave.hpp>

namespace lightwave {

class HomogeneousMedium : public Medium {
    float m_density;

public:
    HomogeneousMedium(const Properties &properties) {
        m_density = properties.get<float>("density");
    }

    Color Tr(const Ray &ray, Sampler &rng) {
        
    }

    std::string toString() const override {
        return tfm::format("Homogeneous medium[\n"
                           "  density = %s\n"
                           "]",
                           indent(m_density));
    }
};

} // namespace lightwave

REGISTER_EMISSION(Lambertian, "lambertian")

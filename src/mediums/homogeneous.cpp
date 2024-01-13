#include <lightwave.hpp>

namespace lightwave {

class HomogeneousMedium : public Medium {
    float m_density;
    Color m_color;

public:
    HomogeneousMedium(const Properties &properties) {
        m_density = properties.get<float>("density");
        m_color = properties.get<Color>("color", Color(0));
    }

    float Tr(const Ray &ray, const Intersection its, Sampler &rng) const override {
        // implement beer's law
        if (!its) {
            return 0;
        }
        float distance = (ray.origin - ray(its.t)).length();
        float T = exp(-distance * m_density);

        // for now this is only absorption
        return T;
    }

    Color getColor() const override {
        return m_color;
    }

    std::string toString() const override {
        return tfm::format("Homogeneous medium[\n"
                           "  density = %s\n"
                           "]",
                           indent(m_density));
    }
};

} // namespace lightwave

REGISTER_CLASS(HomogeneousMedium, "medium", "homogeneous")

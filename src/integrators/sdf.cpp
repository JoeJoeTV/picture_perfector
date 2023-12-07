#include <lightwave.hpp>

namespace lightwave {
class SDFIntegrator : public SamplingIntegrator {
public:
    SDFIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        // Do nothing?
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced by the camera model.
     * This will be run for each pixel of the image, potentially with multiple samples for each pixel.
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        // intersect the ray with the scene
        Intersection its = m_scene->intersect(ray, rng);

        // if no intersection occured
        if (!its) {
            return Color(0.0f);
        } else {
            return Color(its.stats.sdfStepFraction);
        }
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "SDFIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image)
        );
    }
};

}

REGISTER_INTEGRATOR(SDFIntegrator, "sdf")
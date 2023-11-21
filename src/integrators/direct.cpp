#include <lightwave.hpp>

namespace lightwave {
class DirectIntegretor : public SamplingIntegrator {
    /// @brief Whether to remap the normal from [-1,1] to [0,1] 
    // bool m_remap;

public:
    DirectIntegretor(const Properties &properties)
    : SamplingIntegrator(properties) {
        // to parse properties from the scene description, use properties.get(name, default_value)
        // you can also omit the default value if you want to require the user to specify a value
        // m_remap = properties.get<bool>("remap", true);
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced by the camera model.
     * This will be run for each pixel of the image, potentially with multiple samples for each pixel.
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        // intersect the ray with the scene
        Intersection its = m_scene->intersect(ray, rng);

        // if no intersection occured
        if (its.instance == nullptr) {
            return (m_scene->evaluateBackground(ray.direction)).value;
        }

        // sample the bsdf of the hit instance
        BsdfSample sample = its.sampleBsdf(rng);
        
        // update weight of sample to account for emission if there are emissions
        Color emissions = its.evaluateEmission();
        
        // trace secondary ray
        Vector directionVectorSecondRay = sample.wi.normalized();
        Ray secondaryRay = Ray(its.position, directionVectorSecondRay);
        Intersection its2 = m_scene->intersect(secondaryRay, rng);

        if (its2.instance == nullptr) {
            // no hit occured -> update the weight with the light
            sample.weight *= (m_scene->evaluateBackground(secondaryRay.direction)).value;    
        } else {
            sample.weight *= its2.evaluateEmission();
        }

        return Color(sample.weight) + emissions;
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "CameraIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image)
        );
    }
};

}

// this informs lightwave to use our class CameraIntegrator whenever a <integrator type="camera" /> is found in a scene file
REGISTER_INTEGRATOR(DirectIntegretor, "direct")
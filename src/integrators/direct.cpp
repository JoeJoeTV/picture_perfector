#include <lightwave.hpp>

namespace lightwave {
class DirectIntegretor : public SamplingIntegrator {
    
    Color calculateLight(Intersection &its, Sampler &rng) {
        if (not this->m_scene->hasLights()) {
            return Color(0.0f);
        }

        // Sample random light source in the scene and sample point on selected light source
        const LightSample ls = this->m_scene->sampleLight(rng);

        // If light can be intersected, don't count it (since it will be hit by the ray already)
        if (ls.light->canBeIntersected()) {
            return Color(0.0f);
        }

        const DirectLightSample dls = ls.light->sampleDirect(its.position, rng);

        // Check if light source is blocked for intersection
        const Intersection its_shadow = this->m_scene->intersect(Ray(its.position, dls.wi), rng);

        if (its_shadow and (its_shadow.t < dls.distance)) {
            return Color(0.0f);
        }

        // check that the lightsouce is not behind the object
        if (dls.wi.dot(its.frame.normal) < 0) {
            return Color(0.f);
        }

        const BsdfEval bsdf_sample = its.evaluateBsdf(dls.wi);

        Color contribution = (dls.weight * bsdf_sample.value) / ls.probability;

        return contribution;
    }

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

        const Color lightContribution = calculateLight(its, rng);

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

        return Color(sample.weight) + emissions + lightContribution;
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
#include <lightwave.hpp>

namespace lightwave {
class Pathtracer : public SamplingIntegrator {

    int m_depth;
    
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
        /*if (dls.wi.dot(its.frame.normal) < 0) {
            return Color(0.f);
        }*/

        const BsdfEval bsdf_sample = its.evaluateBsdf(dls.wi);

        Color contribution = (dls.weight * bsdf_sample.value) / ls.probability;

        return contribution;
    }

public:
    Pathtracer(const Properties &properties)
    : SamplingIntegrator(properties) {
        // to parse properties from the scene description, use properties.get(name, default_value)
        // you can also omit the default value if you want to require the user to specify a value
        m_depth = properties.get<int>("depth", 2);
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced by the camera model.
     * This will be run for each pixel of the image, potentially with multiple samples for each pixel.
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        // accumulatedLight can be as the current value of the pixel
        Color accumulatedLight = Color(0.f);
        // accumulatedWeight is the light will be absopred along the path so far
        Color accumulatedWeight = Color(1.f);
        Ray currentRay = ray;

        for (int i = 0; i < m_depth; i++) {
            // intersect the ray with the scene
            Intersection its = m_scene->intersect(currentRay, rng);

            // if no intersection occured
            if (!its) {
                Color backgroundLight = (m_scene->evaluateBackground(currentRay.direction)).value;
                accumulatedLight += accumulatedWeight * backgroundLight;
                break;
            }
            
            // sample the bsdf for a new bounce and weight
            BsdfSample sample = its.sampleBsdf(rng);

            // get emissions of intersection
            Color emissions = its.evaluateEmission();

            // next event estimation to evaluate light
            Color lightContribution = calculateLight(its, rng);

            // update accumulated light and weigt
            if (i == m_depth-1) {
                lightContribution = Color(0.f);
            }
            accumulatedLight += accumulatedWeight * (emissions + lightContribution);
            accumulatedWeight *= sample.weight;         

            // update variables for next iteration
            currentRay = Ray(its.position, sample.wi.normalized());
        }  

        return accumulatedLight;
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "Pathtracer[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "  depth = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image),
            indent(m_depth)
        );
    }
};

}

REGISTER_INTEGRATOR(Pathtracer, "pathtracer")
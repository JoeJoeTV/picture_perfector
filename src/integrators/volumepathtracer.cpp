#include <lightwave.hpp>

namespace lightwave {
class Volumepathtracer : public SamplingIntegrator {

    int m_depth;

    // intersect but ignor volums. Account for them with Tr() and change light contribution
    // returns a weight that is one of the light is hit right away
    // returns 0 if the light is blocked
    // returns any value between 0 and 1 if there is a volume
    float intersectTr(const Ray &ray, DirectLightSample dls, Sampler &rng) {
        float weight = 1;
        Ray currentRay = ray;
        float distance = dls.distance;
        while (true) {
            // Check if there is something between light and hitpoit
            const Intersection its_shadow = this->m_scene->intersect(currentRay, rng);

            if (!its_shadow || (its_shadow.t > distance)) {
                return weight;
            }
            
            if (its_shadow.instance->bsdf() != nullptr) {
                return 0;
            }

            weight *= its_shadow.instance->medium()->Tr(currentRay, its_shadow.t, rng);

            distance -= (its_shadow.position - currentRay.origin).length();
            currentRay = Ray(its_shadow.position, currentRay.direction);
        }
    }
    
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
        /*const Intersection its_shadow = this->m_scene->intersect(Ray(its.position, dls.wi), rng);

        if (its_shadow and (its_shadow.t < dls.distance)) {
            return Color(0.0f);
        }*/

        /*if (this->intersectTr(Ray(its.position, dls.wi), dls, rng)) {
            return Color(0.0f);
        }*/
        float traceWeight = this->intersectTr(Ray(its.position, dls.wi), dls, rng);

        const BsdfEval bsdf_sample = its.evaluateBsdf(dls.wi);

        Color contribution = (traceWeight * dls.weight * bsdf_sample.value) / ls.probability;

        return contribution;
    }

public:
    Volumepathtracer(const Properties &properties)
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

        Medium* currentMedium = nullptr;

        for (int i = 0; i < m_depth; i++) {
            // intersect the ray with the scene
            Intersection its = m_scene->intersect(currentRay, rng);

            // sample the active medium
            float tScatter = Infinity;
            float pOfSampleingMedium = 0;
            if (currentMedium != nullptr) {
                tScatter = currentMedium->sampleHitDistance(currentRay, rng);
                pOfSampleingMedium =  clamp((*currentMedium).probabilityOfSampelingBeforeT(its.t), 0.f, 1.f);
            }

            // if no intersection occured
            if (!its) {
                // There is also no medium change, so if there is a medium the light contribution will be zero
                // Only works if thee is no emision from the medium
                if (currentMedium != nullptr) {
                    break;
                }

                Color backgroundLight = (m_scene->evaluateBackground(currentRay.direction)).value;
                accumulatedLight += accumulatedWeight * backgroundLight;
                break;
            }

            // either evaluate medium or surface interaction
            if (tScatter < its.t) {
                // Medium scatter event

                float mediumTValue = (*currentMedium).Tr(currentRay, tScatter, rng);

                Intersection itsMedium = Intersection(its.wo, tScatter);
                itsMedium.position = currentRay(tScatter);
                // next event estimation to evaluate light
                Color lightContribution = calculateLight(itsMedium, rng);

                if (i == m_depth-1) {
                    lightContribution = Color(0.f);
                }

                float probabilityOfThisScatter = (*currentMedium).probabilityOfSampelingThisPoint(tScatter);

                accumulatedWeight *= mediumTValue * currentMedium->getColor() / (probabilityOfThisScatter*Pi);
                accumulatedLight +=  accumulatedWeight * lightContribution;
                
                // sample a direction that we scater in
                Vector wi = currentMedium->samplePhase(itsMedium, rng);

                // new ray
                currentRay = Ray(itsMedium.position, wi.normalized(), i+1);
            } else {
                // Surface scatter event

                // sample the bsdf for a new bounce and weight
                BsdfSample sample = its.sampleBsdf(rng);

                // check if we enter the instance or leave or neither
                if (Frame::cosTheta(its.frame.toLocal(its.wo)) < 0 &&
                    Frame::cosTheta(its.frame.toLocal(sample.wi)) > 0) {
                    // leave the shape
                    currentMedium = nullptr;
                } 
                if (Frame::cosTheta(its.frame.toLocal(its.wo)) > 0 &&
                    Frame::cosTheta(its.frame.toLocal(sample.wi)) < 0) {
                    // enter the shape
                    currentMedium = its.instance->medium();
                } 

                // get emissions of intersection
                Color emissions = its.evaluateEmission();

                // next event estimation to evaluate light
                Color lightContribution = Color(0);
                if (its.instance->bsdf() != nullptr) {
                    lightContribution = calculateLight(its, rng);
                }

                // update accumulated light and weigt
                if (i == m_depth-1) {
                    lightContribution = Color(0.f);
                }
                //std::cout << pOfSampleingMedium << std::endl;
                accumulatedLight += accumulatedWeight * (emissions + lightContribution) / (1-pOfSampleingMedium);
                accumulatedWeight *= sample.weight;
                
                // update variables for next iteration
                currentRay = Ray(its.position, sample.wi.normalized(), i+1);
            }
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

REGISTER_INTEGRATOR(Volumepathtracer, "volumePathtracer")
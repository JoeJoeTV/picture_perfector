/**
 * @file integrator.hpp
 * @brief Contains the Integrator interface and related interfaces.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/color.hpp>
#include <lightwave/math.hpp>
#include <lightwave/sampler.hpp>
#include <lightwave/image.hpp>
#include <lightwave/scene.hpp>

namespace lightwave {

//#define DEBUG_PIXEL
//#define DEBUG_PIXEL_POS Point2i(0, 0)

#ifdef DEBUG_PIXEL
typedef struct t_debugPixel {
    bool active = false;
    int sample = 0;
} t_debugPixel;
extern t_debugPixel debugPixel;
#endif

#ifdef DEBUG_PIXEL
#define DEBUG_PIXEL_LOG(...) if (debugPixel.active) logger(EDebug, __VA_ARGS__)
#else
#define DEBUG_PIXEL_LOG(...) do {} while(0)
#endif

/**
 * @brief Integrators are rendering algorithms that take a scene and produce an image from them (e.g., using path tracing).
 * The term integrator refers to the key challenge of simulating light transport, namely solving the reflected radiance integral.
 * Integrators can also be used to visualize quantities other than radiance for debugging purposes, for example, visualizing
 * the normals of surfaces that were intersected.
 */
class Integrator : public Executable {
public:
    Integrator(const Properties &properties) {
    }
};

/**
 * @brief A sampling integrator uses random numbers to solve the integration problem, e.g., by using Monte Carlo integration.
 */
class SamplingIntegrator : public Integrator {
protected:
    /// @brief The random number generator used to steer sampling decisions.
    ref<Sampler> m_sampler;
    /// @brief The output image generated by the rendering algorithm.
    ref<Image> m_image;
    /// @brief The scene that should be rendered.
    ref<Scene> m_scene;

public:
    SamplingIntegrator(const Properties &properties)
    : Integrator(properties) {
        m_sampler = properties.getChild<Sampler>();
        m_image = properties.getOptionalChild<Image>();
        m_scene = properties.getChild<Scene>();
    }

    /// @brief Sets the output image that should be populated by rendering.
    void setImage(const ref<Image> &image) { m_image = image; }

    /// @brief Gets the output image that will be populated by rendering. 
    Image *image() { return m_image.get(); }
    /// @brief Gets the scene that will be rendered. 
    Scene *scene() { return m_scene.get(); }
    /// @brief Gets the random number generator that steers the sampling decisions. 
    Sampler *sampler() { return m_sampler.get(); }

    /// @brief Computes all pixels of the image by constructing camera rays for them and invoking the @ref Li method.
    void execute() override;
    
    /**
     * @brief Returns (an estimate of) the incident radiance for a given ray.
     * By default, the integrator will take care of looping over all pixels, constructing camera rays for each of them,
     * and then invoking this method to determine the pixel values. If you need to customize this process, override the
     * @ref execute function of the integrator.
     */
    virtual Color Li(const Ray &ray, Sampler &rng) = 0;
};

}

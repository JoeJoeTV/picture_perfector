#include <lightwave/integrator.hpp>
#include <lightwave/camera.hpp>
#include <lightwave/parallel.hpp>

#include <algorithm>
#include <chrono>

#include <lightwave/streaming.hpp>
#include <lightwave/iterators.hpp>

namespace lightwave {

#ifdef DEBUG_PIXEL
t_debugPixel debugPixel;
#endif

void SamplingIntegrator::execute() {
    if (!m_image) {
        lightwave_throw("<integrator /> needs an <image /> child to render into!");
    }

    const Vector2i resolution = m_scene->camera()->resolution();
    m_image->initialize(resolution);

    const float norm = 1.0f / m_sampler->samplesPerPixel();
    
    Streaming stream { *m_image };
    ProgressReporter progress { resolution.product() };
    for_each_parallel(BlockSpiral(resolution, Vector2i(64)), [&](auto block) {
        auto sampler = m_sampler->clone();

#ifdef DEBUG_PIXEL
        debugPixel.active = false;
#endif

        for (auto pixel : block) {
#ifdef DEBUG_PIXEL
            if (pixel == DEBUG_PIXEL_POS)
                debugPixel.active = true;
#endif
            DEBUG_PIXEL_LOG("Debug Pixel at %s:", DEBUG_PIXEL_POS);
            
            Color sum;
            for (int sample = 0; sample < m_sampler->samplesPerPixel(); sample++) {
#ifdef DEBUG_PIXEL
                if (debugPixel.active) {
                    debugPixel.sample = sample;
                    logger(EDebug, "Debug Sample #%d:", debugPixel.sample);
                }
#endif
                sampler->seed(pixel, sample);
                auto cameraSample = m_scene->camera()->sample(pixel, *sampler);
                sum += cameraSample.weight * Li(cameraSample.ray, *sampler);
            }
            m_image->get(pixel) = norm * sum;
            
#ifdef DEBUG_PIXEL
            debugPixel.active = false;
#endif
        }

        progress += block.diagonal().product();
        stream.updateBlock(block);
    });
    progress.finish();

    m_image->save();
}

}

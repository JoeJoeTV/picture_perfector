#include <lightwave.hpp>
#include <OpenImageDenoise/oidn.hpp>

namespace lightwave {

class ImageDenoise : public Postprocess {
    /// @brief The input image that is to be processed.
    ref<Image> m_albedo;
    /// @brief The output image that will be produced.
    ref<Image> m_normals;

public:
    ImageDenoise(const Properties &properties): Postprocess(properties) {
        m_albedo = properties.get<Image>("albedo", nullptr);
        m_normals = properties.get<Image>("normals", nullptr);
    }

    void execute() override {
        // Create an Open Image Denoise device
        oidn::DeviceRef device = oidn::newDevice(); // CPU or GPU if available
        device.commit();

        int width = m_input->resolution()[0];
        int height = m_input->resolution()[1];

        // Create buffers for input/output images accessible by both host (CPU) and device (CPU/GPU)
        oidn::BufferRef colorBuf  = device.newBuffer(width * height * 3 * sizeof(float));
        oidn::BufferRef albedoBuf = device.newBuffer(width * height * 3 * sizeof(float));
        oidn::BufferRef normalBuf  = device.newBuffer(width * height * 3 * sizeof(float));

        // Create a filter for denoising a beauty (color) image using optional auxiliary images too
        // This can be an expensive operation, so try no to create a new filter for every image!
        oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
        filter.setImage("color",  colorBuf,  oidn::Format::Float3, width, height); // beauty
        if (m_albedo != nullptr) 
            filter.setImage("albedo", albedoBuf, oidn::Format::Float3, width, height); // auxiliary

        if (m_normals != nullptr)
            filter.setImage("normal", normalBuf, oidn::Format::Float3, width, height); // auxiliary

        filter.setImage("output", colorBuf,  oidn::Format::Float3, width, height); // denoised beauty
        //filter.set("hdr", true); // beauty image is HDR
        filter.commit();

        // Fill the input image buffers;
        m_output->copy(*m_albedo);
        memcpy(colorBuf.getData(), m_input->data(), width * height * 3 * sizeof(float));

        if (m_albedo != nullptr) 
            memcpy(albedoBuf.getData(), m_albedo->data(), width * height * 3 * sizeof(float));

        if (m_normals != nullptr)
            memcpy(normalBuf.getData(), m_normals->data(), width * height * 3 * sizeof(float));

        // Filter the beauty image
        filter.execute();

        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
            logger(EWarn, errorMessage);

        // copy result into output
        memcpy(m_output->data(), colorBuf.getData(), width * height * 3 * sizeof(float));
        m_output->save();
    }

    virtual std::string toString() const override{
        return tfm::format("Image Denoise");
    }
};

} // namespace lightwave

REGISTER_CLASS(ImageDenoise, "postprocess", "denoising")

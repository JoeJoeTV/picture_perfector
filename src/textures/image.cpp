#include <lightwave.hpp>

namespace lightwave {

class ImageTexture : public Texture {
    enum class BorderMode {
        Clamp,
        Repeat,
    };

    enum class FilterMode {
        Nearest,
        Bilinear,
    };

    ref<Image> m_image;
    float m_exposure;
    BorderMode m_border;
    FilterMode m_filter;

public:
    ImageTexture(const Properties &properties) {
        if (properties.has("filename")) {
            m_image = std::make_shared<Image>(properties);
        } else {
            m_image = properties.getChild<Image>();
        }
        m_exposure = properties.get<float>("exposure", 1);

        m_border =
            properties.getEnum<BorderMode>("border", BorderMode::Repeat,
                                           {
                                               { "clamp", BorderMode::Clamp },
                                               { "repeat", BorderMode::Repeat },
                                           });

        m_filter = properties.getEnum<FilterMode>(
            "filter", FilterMode::Bilinear,
            {
                { "nearest", FilterMode::Nearest },
                { "bilinear", FilterMode::Bilinear },
            });
    }

    Color evaluate(const Point2 &uv) const override {
        Point2 fixedPoint = uv;
        if (m_border == BorderMode::Clamp) {
            if (fixedPoint[0] < 0) {
                fixedPoint[0] = 0;
            } else if (fixedPoint[0] > 1) {
                fixedPoint[0] = 1;
            }

            if (fixedPoint[1] < 0) {
                fixedPoint[1] = 0;
            } else if (fixedPoint[1] > 1) {
                fixedPoint[1] = 1;
            }
        } else {
            fixedPoint[0] = fixedPoint[0] - floor(fixedPoint[0]);
            fixedPoint[1] = fixedPoint[1] - floor(fixedPoint[1]);
        }

        Point2 fixedPointInImagePixels = Point2(fixedPoint[0]*(m_image->bounds().max()[0]-1), 
                                                (m_image->bounds().max()[1]-1) - fixedPoint[1]*(m_image->bounds().max()[1]-1));
        if (m_filter == FilterMode::Nearest) {
            Point2i pixelCoordinates = Point2i(floor(fixedPointInImagePixels[0] + 0.5), 
                                                floor(fixedPointInImagePixels[1] + 0.5));

            return m_image->get(pixelCoordinates);
        } else {
            Point2i topRight = Point2i(floor(fixedPointInImagePixels[0] + 1), 
                                                floor(fixedPointInImagePixels[1] + 1));
            Point2i topLeft = Point2i(floor(fixedPointInImagePixels[0]), 
                                                floor(fixedPointInImagePixels[1] + 1));
            Point2i bottomRight = Point2i(floor(fixedPointInImagePixels[0] + 1), 
                                                floor(fixedPointInImagePixels[1]));
            Point2i bottomLeft = Point2i(floor(fixedPointInImagePixels[0]), 
                                                floor(fixedPointInImagePixels[1]));

            // get the values in the image
            Color colorTopRight = m_image->get(topRight);
            Color colorTopLeft = m_image->get(topLeft);
            Color colorBottomRight = m_image->get(bottomRight);
            Color colorBottomLeft = m_image->get(bottomLeft);

            // interpolate in x direction
            Color xInterpolatedTop = (topRight[0]- fixedPointInImagePixels[0])*colorTopLeft +
                                     (fixedPointInImagePixels[0] - topLeft[0])*colorTopRight;
            Color xInterpolatedBottom = (bottomRight[0]- fixedPointInImagePixels[0])*colorBottomLeft +
                                     (fixedPointInImagePixels[0] - bottomLeft[0])*colorBottomRight;

            // interpolate in y direction
            Color interpolatedColor = (topRight[1] - fixedPointInImagePixels[1]) * xInterpolatedBottom +
                                      (fixedPointInImagePixels[1] - bottomRight[1]) * xInterpolatedTop;

            return interpolatedColor;
        }
        return Color(0.f);
    }

    std::string toString() const override {
        return tfm::format("ImageTexture[\n"
                           "  image = %s,\n"
                           "  exposure = %f,\n"
                           "]",
                           indent(m_image), m_exposure);
    }
};

} // namespace lightwave

REGISTER_TEXTURE(ImageTexture, "image")

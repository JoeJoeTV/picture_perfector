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
            if (fixedPoint[0] <= 0) {
                fixedPoint[0] = 0+1e-6f;
            } else if (fixedPoint[0] >= 1) {
                fixedPoint[0] = 1-1e-6f;
            }

            if (fixedPoint[1] <= 0) {
                fixedPoint[1] = 0+1e-6f;
            } else if (fixedPoint[1] >= 1) {
                fixedPoint[1] = 1-1e-6f;
            }
        } else {
            fixedPoint[0] = fixedPoint[0] - floor(fixedPoint[0]);
            fixedPoint[1] = fixedPoint[1] - floor(fixedPoint[1]);

            // make sure that 0 and 1 are excluded
            if (fixedPoint[0] <= 0) {
                fixedPoint[0] = 0+1e-6f;
            } else if (fixedPoint[0] >= 1) {
                fixedPoint[0] = 1-1e-6f;
            }

            if (fixedPoint[1] <= 0) {
                fixedPoint[1] = 0+1e-6f;
            } else if (fixedPoint[1] >= 1) {
                fixedPoint[1] = 1-1e-6f;
            }
        }

        // the point goes from ]-0.5 : pixels+0.5[^2 in such that all pixles have the same size.
        // without the 0.5 the outer pixles are only half as big as the central ones.
        Point2 fixedPointInImagePixels = Point2(fixedPoint[0]*(m_image->bounds().max()[0]) - 0.5, 
                                                (m_image->bounds().max()[1]-1) - 
                                                (fixedPoint[1]*(m_image->bounds().max()[1]) - 0.5));

        assert(fixedPointInImagePixels[0] < m_image->bounds().max()[0]-0.5);
        assert(fixedPointInImagePixels[1] < m_image->bounds().max()[1]-0.5);
        assert(fixedPointInImagePixels[0] > -0.5);
        assert(fixedPointInImagePixels[1] > -0.5);

        if (m_filter == FilterMode::Nearest) {
            Point2i pixelCoordinates = Point2i(floor(fixedPointInImagePixels[0] + 0.5), 
                                                floor(fixedPointInImagePixels[1] + 0.5));

            return m_image->get(pixelCoordinates)*m_exposure;
        } else {
            // compute the four closest pixel coordintes
            int xRight = floor(fixedPointInImagePixels[0] + 1);
            int xLeft = floor(fixedPointInImagePixels[0]);
            int yTop = floor(fixedPointInImagePixels[1] + 1);
            int yBottom = floor(fixedPointInImagePixels[1]);

            // compute the corresponding pixel values that are in the image
            // this might be different because (-1,-1) need to access (maxX,maxY) and max+1 needs to map to zero
            int xRightInImage = xRight;
            int xLeftInImage = xLeft;
            int yTopInImage = yTop;
            int yBottomInImage = yBottom;

            if (m_border == BorderMode::Clamp) {
                if (xRightInImage > m_image->bounds().max()[0]-1) {
                xRightInImage = m_image->bounds().max()[0]-1;
                }

                if (xLeftInImage < 0) {
                    xLeftInImage = 0;
                }

                if (yTopInImage > m_image->bounds().max()[1]-1) {
                    yTopInImage = m_image->bounds().max()[1]-1;
                }

                if (yBottomInImage < 0) {
                    yBottomInImage = 0;
                }
            } else {
                if (xRightInImage > m_image->bounds().max()[0]-1) {
                    xRightInImage = 0;
                }

                if (xLeftInImage < 0) {
                    xLeftInImage = m_image->bounds().max()[0]-1;
                }

                if (yTopInImage > m_image->bounds().max()[1]-1) {
                    yTopInImage = 0;
                }   

                if (yBottomInImage < 0) {
                    yBottomInImage = m_image->bounds().max()[1]-1;
                }
            }

            // get the values in the image
            Color colorTopRight = m_image->get(Point2i(xRightInImage, yTopInImage));
            Color colorTopLeft = m_image->get(Point2i(xLeftInImage, yTopInImage));
            Color colorBottomRight = m_image->get(Point2i(xRightInImage, yBottomInImage));
            Color colorBottomLeft = m_image->get(Point2i(xLeftInImage, yBottomInImage));

            // interpolate in x direction
            Color xInterpolatedTop = (xRight- fixedPointInImagePixels[0])*colorTopLeft +
                                     (fixedPointInImagePixels[0] - xLeft)*colorTopRight;
            Color xInterpolatedBottom = (xRight- fixedPointInImagePixels[0])*colorBottomLeft +
                                     (fixedPointInImagePixels[0] - xLeft)*colorBottomRight;

            // interpolate in y direction
            Color interpolatedColor = (yTop - fixedPointInImagePixels[1]) * xInterpolatedBottom +
                                      (fixedPointInImagePixels[1] - yBottom) * xInterpolatedTop;

            return interpolatedColor * m_exposure;
        }
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

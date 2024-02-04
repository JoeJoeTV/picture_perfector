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

    // Look at this nice music I found: https://shorturl.at/hlzSW

    /// @brief Returns the Color at the pixel pointed to by the given integer coordinates while respecting the BorderMode of the image texture
    /// @param iuv Integer coordinates on the image
    /// @return The Color ofthe pixel
    Color GetPixel(const Point2i &iuv) const {
        const Point2i maxCoords = Point2i(
            this->m_image->resolution().x() - 1,
            this->m_image->resolution().y() - 1
        );

        Point2i coords = Point2i();

        switch (this->m_border) {
        case BorderMode::Clamp:
            coords.x() = max(0, min(iuv.x(), maxCoords.x()));
            coords.y() = max(0, min(iuv.y(), maxCoords.y()));
            break;
        
        case BorderMode::Repeat:
            coords.x() = iuv.x() % this->m_image->resolution().x();
            coords.y() = iuv.y() % this->m_image->resolution().y();

            // If negative, we have to flip the orientation for correct repetition
            if (coords.x() < 0) 
                coords.x() = this->m_image->resolution().x() + coords.x();
            if (coords.y() < 0) 
                coords.y() = this->m_image->resolution().y() + coords.y();
            break;
        }
        
        return this->m_image->get(coords);
    }

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
        // Flip y axis to correct ortientation
        Point2 texPos = Point2(uv.x(), 1.0f - uv.y());

        // Scale UV coordinates to texture size
        texPos.x() = texPos.x() * this->m_image->resolution().x();
        texPos.y() = texPos.y() * this->m_image->resolution().y();

        Color pxColor;
        switch (this->m_filter) {
        case FilterMode::Nearest:
            // For nearest neighbor, we simply floor the scaled coordinates to get the texture coordinates as integers
            pxColor = GetPixel(Point2i(
                floor(texPos.x()),
                floor(texPos.y())
            ));
            break;
        case FilterMode::Bilinear: {
                // Shift the coordinates, such that the texture is not shifted wrong
                Point2 texMidPos = texPos - Vector2(0.5f);

                Point2i cellPos = Point2i(
                    floor(texMidPos.x()),
                    floor(texMidPos.y())
                );
                Point2 cellOffset = texMidPos - Point2(cellPos.x(), cellPos.y());

                // Get 4 edge colors to interpolate between
                Color colorTL = GetPixel(cellPos);
                Color colorTR = GetPixel(Point2i(cellPos.x() + 1, cellPos.y()));
                Color colorBL = GetPixel(Point2i(cellPos.x(), cellPos.y() + 1));
                Color colorBR = GetPixel(Point2i(cellPos.x() + 1, cellPos.y() + 1));

                // First, we interpolate between left and right
                Color colorTI = colorTR * cellOffset.x() + colorTL * (1.0f - cellOffset.x());
                Color colorBI = colorBR * cellOffset.x() + colorBL * (1.0f - cellOffset.x());

                // Finally, interpolate between top and bottom color
                pxColor = colorBI * cellOffset.y() + colorTI * (1.0f - cellOffset.y());
            } break;
        }

        // Finally, apply exposure to calculated color
        return pxColor * this->m_exposure;
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

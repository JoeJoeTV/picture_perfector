#include <lightwave.hpp>

namespace lightwave {

class CheckerboardTexture : public Texture {
    Color m_color0;
    Color m_color1;
    Vector2 m_scale;

public:
    CheckerboardTexture(const Properties &properties) {
        m_color0 = properties.get<Color>("color0");
        m_color1 = properties.get<Color>("color1");
        m_scale = properties.get<Vector2>("scale");
    }

    Color evaluate(const Point2 &uv) const override { 
        int scaleduvX = floor(uv[0] * m_scale[0]);
        int scaleduvY = floor(uv[1] * m_scale[1]);

        if (scaleduvX % 2 == 0) {
            if (scaleduvY % 2 == 0) {
                return m_color0;
            } else {
                return m_color1;
            }
        } else {
            if (scaleduvY % 2 == 0) {
                return m_color1;
            } else {
                return m_color0;
            }
        }
    }

    std::string toString() const override {
        return tfm::format("CheckerboardTexture[\n"
                           "  color1 = %s\n"
                           "  color2 = %s\n"
                           "  scale = %s\n"
                           "]",
                           indent(m_color0),
                           indent(m_color1),
                           indent(m_scale));
    }
};

} // namespace lightwave

REGISTER_TEXTURE(CheckerboardTexture, "checkerboard")

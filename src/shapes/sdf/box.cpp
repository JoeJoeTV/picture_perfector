#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

class SDFBox : public SDFShape {
    /// @brief The corner of the cube specifying the size
    Point m_corner;

    /// @brief The corner radius for the edges of the box. Set to 0 to disable
    float m_cornerRadius;

    /// @brief Transform for SDF
    ref<Transform> m_transform;
public:
    SDFBox(const Properties &properties) {
        this->m_corner = Point{
            properties.get<float>("cx", 1.0f),
            properties.get<float>("cy", 1.0f),
            properties.get<float>("cz", 1.0f)
        };

        this->m_cornerRadius = properties.get<float>("radius", 0.0f);

        this->m_transform = properties.getOptionalChild<Transform>();
    }

    float estimateDistance(const Point p) const override {
        Point transP;

        if (this->m_transform) {
            transP = this->m_transform->inverse(p);
        } else {

            transP = p;
        }

        Point absP = Point{abs(transP.x()), abs(transP.y()), abs(transP.z())};

        Vector q = absP - this->m_corner;
        Vector maxQ0 = Vector{max(q.x(), 0.0f), max(q.y(), 0.0f), max(q.z(), 0.0f)};
        
        return maxQ0.length() + min(max(q.x(), max(q.y(), q.z())), 0.0f) - this->m_cornerRadius;
    }

    Bounds getBoundingBox() const override {
        Point maxP = this->m_corner + Vector(this->m_cornerRadius);
        Point minP = Point(-Vector(this->m_corner)) - Vector(this->m_cornerRadius);

        if (this->m_transform) {
            maxP = this->m_transform->apply(maxP);
            minP = this->m_transform->apply(minP);
        }

        return Bounds(minP, maxP);
    }

    std::string toString() const override {
        return tfm::format(
            "SDFBox[\n"
            "  corner = (%f, %f, %f),\n"
            "  radius = %f,\n"
            "]",
            this->m_corner.x(),
            this->m_corner.y(),
            this->m_corner.z(),
            this->m_cornerRadius
        );
    }
};
    
}

REGISTER_CLASS(SDFBox, "sdf", "box")
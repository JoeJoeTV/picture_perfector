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
        Point origin = Point(0.0f);
        Vector xVec = Vector(this->m_corner.x(), 0.0f, 0.0f);
        Vector yVec = Vector(0.0f, this->m_corner.y(), 0.0f);
        Vector zVec = Vector(0.0f, 0.0f, this->m_corner.z());

        if (this->m_transform) {
            origin = this->m_transform->apply(origin);
            xVec = this->m_transform->apply(xVec);
            yVec = this->m_transform->apply(yVec);
            zVec = this->m_transform->apply(zVec);
        }

        Vector vertex00 = xVec - yVec - zVec;
        Vector vertex01 = xVec - yVec + zVec;
        Vector vertex10 = xVec + yVec - zVec;
        Vector vertex11 = xVec + yVec + zVec;

        Vector boundMax = Vector(
            std::max({vertex00.x(), vertex01.x(), vertex10.x(), vertex11.x()}),
            std::max({vertex00.y(), vertex01.y(), vertex10.y(), vertex11.y()}),
            std::max({vertex00.z(), vertex01.z(), vertex10.z(), vertex11.z()})
        );

        Point maxP = origin + boundMax + Vector(this->m_cornerRadius);
        Point minP = origin - boundMax - Vector(this->m_cornerRadius);

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
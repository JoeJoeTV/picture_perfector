#include <lightwave.hpp>
#include "sdfshape.hpp"

namespace lightwave {

class SDFCube : public SDFShape {
    /// @brief The corner of the cube specifying the size
    Point m_corner;
public:
    SDFCube(const Properties &properties) {
        this->m_corner = Point{
            properties.get<float>("cx", 1.0f),
            properties.get<float>("cy", 1.0f),
            properties.get<float>("cz", 1.0f)
        };
    }

    float estimateDistance(const Point p) const override {
        static bool wasCalled = false;

        if (!wasCalled) {
            logger(EInfo, "[SDFCube] 'estimateDistance' called!");
            wasCalled = true;
        }

        Point absP = Point{abs(p.x()), abs(p.y()), abs(p.z())};

        Vector q = absP - this->m_corner;
        Vector maxQ0 = Vector{max(q.x(), 0.0f), max(q.y(), 0.0f), max(q.z(), 0.0f)};
        
        return maxQ0.length() + min(max(q.x(), max(q.y(), q.z())), 0.0f);
    }

    std::string toString() const override {
        return tfm::format(
            "SDFCube[\n"
            "  corner = (%f, %f, %f),\n"
            "]",
            this->m_corner.x(),
            this->m_corner.y(),
            this->m_corner.z()
        );
    }
};
    
}

REGISTER_CLASS(SDFCube, "sdf", "cube")
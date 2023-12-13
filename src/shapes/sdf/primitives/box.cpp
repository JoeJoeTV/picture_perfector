#include <lightwave.hpp>
#include "../sdfobject.hpp"

namespace lightwave {

class SDFBox : public SDFObject {
    /// @brief The corner defining the size of the shape in each direction
    Point m_corner;

public:
    SDFBox(const Properties &properties) {
        this->m_corner = properties.get<Point>("corner", Point(1.0f, 1.0f, 1.0f));
    }

    float estimateDistance(const Point p) const override {
        Point absP = Point{abs(p.x()), abs(p.y()), abs(p.z())};

        Vector q = absP - this->m_corner;
        Vector maxQ0 = Vector{max(q.x(), 0.0f), max(q.y(), 0.0f), max(q.z(), 0.0f)};
        
        return maxQ0.length() + min(max(q.x(), max(q.y(), q.z())), 0.0f);
    }

    Bounds getBoundingBox() const override {
        return Bounds(
            Point(-this->m_corner.x(), -this->m_corner.y(), -this->m_corner.z()),
            this->m_corner
        );
    }

    std::string toString() const override {
        return tfm::format(
            "SDFBox[\n"
            "  corner = %s,\n"
            "]",
            this->m_corner
        );
    }
};
    
} // namespace lightwave

REGISTER_CLASS(SDFBox, "sdf", "box")
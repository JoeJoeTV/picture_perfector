#include <lightwave.hpp>

namespace lightwave {

class SDFShape : public Object {
public:

    virtual float estimateDistance(const Point p) const = 0;

    std::string toString() const override {
        return "SDFShape[]";
    }
};
    
}
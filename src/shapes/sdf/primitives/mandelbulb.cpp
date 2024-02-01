#include <lightwave.hpp>
#include "../sdfobject.hpp"

namespace lightwave {

class SDFMandelbulb : public SDFObject {
    float m_power;
    int m_iterations;
    float m_bailout;

public:
    SDFMandelbulb(const Properties &properties) {
        this->m_power = properties.get<float>("power", 8.0f);
        this->m_iterations = properties.get<int>("iterations", 5);
        this->m_bailout = properties.get<float>("bailout", 1.15f);
    }

    autodiff::real estimateDistance(const PointReal& p) const override {
        using autodiff::real;
        using autodiff::detail::pow;
        using autodiff::detail::acos;
        using autodiff::detail::atan2;
        using autodiff::detail::sin;
        using autodiff::detail::cos;
        using autodiff::detail::log;
        using autodiff::detail::min;
        using autodiff::detail::max;
        
        VectorReal z = VectorReal(p);
        real dr = 1.0f;
        real r = 0.0f;

        for (int i = 0; i < this->m_iterations; i++) {
            r = z.length();

            if (r > this->m_bailout) {
                break;
            }

            // Convert to polar coordinates
            real theta = acos(z.z() / r);
            real phi = atan2(z.y(), z.x());
            dr = pow(r, this->m_power - 1.0f) * this->m_power * dr + 1.0f;

            // scale and rotate the point
            real zr = pow(r, this->m_power);
            theta = theta * this->m_power;
            phi = phi * this->m_power;

            // Convert back to cartesian coordinates
            z = zr * VectorReal(sin(theta)*cos(phi), sin(phi)*sin(theta), cos(theta));
            z += VectorReal(p);
        }

        return 0.5 * log(r) * r / dr;
    }

    Bounds getBoundingBox() const override {
        return Bounds(
            Point(-1.5f, -1.5f, -1.5f),
            Point(1.5f, 1.5f, 1.5f)
        );
    }

    std::string toString() const override {
        return tfm::format(
            "SDFMandelbulb[\n"
            "  power = %f,\n"
            "  iterations = %d,\n"
            "]",
            this->m_power,
            this->m_iterations
        );
    }
};
    
} // namespace lightwave

REGISTER_CLASS(SDFMandelbulb, "sdf", "mandelbulb")
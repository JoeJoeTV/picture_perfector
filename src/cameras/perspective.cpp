#include <lightwave.hpp>

namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 * 
 * In local coordinates (before applying m_transform), the camera looks in positive z direction [0,0,1].
 * Pixels on the left side of the image ( @code normalized.x < 0 @endcode ) are directed in negative x
 * direction ( @code ray.direction.x < 0 ), and pixels at the bottom of the image ( @code normalized.y < 0 @endcode )
 * are directed in negative y direction ( @code ray.direction.y < 0 ).
 */
class Perspective : public Camera {
private:
    float lengthOfImagePlaneX;
    float lengthOfImagePlaneY;

public:
    Perspective(const Properties &properties)
    : Camera(properties) {
        const float fov = properties.get<float>("fov");

        // compute functions to get from 2d pixels on image plane to 3d camera coordinates
        lengthOfImagePlaneX = sin(fov/2.0);

        // length of the y axis of the image plane is proportional to the x axis length
        // taking the aspect ratio into account
        lengthOfImagePlaneY = lengthOfImagePlaneX * (m_resolution[1] / m_resolution[0]);

        // hints:
        // * precompute any expensive operations here (most importantly trigonometric functions)
        // * use m_resolution to find the aspect ratio of the image
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        // transform the 2d point from image plane into 3d camera coordinate system
        // and create a ray from camera origin through the point on the image plain
        Ray ray = Ray(Vector(0.f, 0.f, 0.f), 
                  Vector(normalized.x()*lengthOfImagePlaneX, 
                          normalized.y()*-lengthOfImagePlaneY, 
                          1.f));
              
        // transform the ray to the world coordinate system
        ray = (*m_transform).apply(ray);

        // normalize the ray
        ray = ray.normalized();

        // create sample and return it
        return CameraSample{
                .ray = ray,
                .weight = Color(1.0f)
            }; 

        // hints:
        // * use m_transform to transform the local camera coordinate system into the world coordinate system
    }

    std::string toString() const override {
        return tfm::format(
            "Perspective[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  transform = %s,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            indent(m_transform)
        );
    }
};

}

REGISTER_CAMERA(Perspective, "perspective")

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
class Thinlens : public Camera {
private:
    float cameraCoordWidth;
    float cameraCoordHeight;
    float pixelSize;

    float apertureRadius;
    float focalDistance;

public:
    Thinlens(const Properties &properties)
    : Camera(properties) {
        const float fov = properties.get<float>("fov");
        const std::string fovAxis = properties.get<std::string>("fovAxis");

        this->apertureRadius = properties.get<float>("apertureRadius");
        this->focalDistance = properties.get<float>("focalDistance");

        float radAngle = (fov / 2.0f) * Deg2Rad;

        const float focal_width = tan(radAngle);

        // Get width and heigh of "camera plane" at z = 1 depending on fovAxis property
        if (fovAxis == "x") {
            this->cameraCoordWidth = focal_width;
            float aspectRatio = static_cast<float>(this->m_resolution[1]) / this->m_resolution[0];
            this->cameraCoordHeight = focal_width * aspectRatio;

            this->pixelSize = (this->cameraCoordWidth * 2) / this->m_resolution[0];
        } else if (fovAxis == "y") {
            this->cameraCoordHeight = focal_width;
            float aspectRatio = static_cast<float>(this->m_resolution[0]) / this->m_resolution[1];
            this->cameraCoordWidth = focal_width * aspectRatio;

            this->pixelSize = (this->cameraCoordHeight * 2) / this->m_resolution[1];
        } else {
            // Any fov axis other than "x" or "y" is invalid
            logger(EWarn, "FOV Axis other than x or y in scene found!");
        }

        // hints:
        // * precompute any expensive operations here (most importantly trigonometric functions)
        // * use m_resolution to find the aspect ratio of the image
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {

        // Get direction vector to pixel on image with correct image aspect ratio
        const Vector dirVector = Vector(
            (normalized.x() * this->cameraCoordWidth),
            (normalized.y() * this->cameraCoordHeight),
            1.0f);

        const Ray origRay = Ray(Point(0.0f, 0.0f, 0.0f), dirVector).normalized();

        // Get point on focal plane that is hit by ray
        const float t = this->focalDistance / origRay.direction.z();
        const Point focalPlaneHit = origRay(t);

        // Get random point for lens in [-1; 1]^2
        const Vector2 lensCoord = ((Vector2(rng.next2D()) * 2) - Vector2(1.0f, 1.0f)) * this->apertureRadius;
        const Point lensOrigin = Point(lensCoord.x(), lensCoord.y(), 0.0f);

        const Ray sampleRay = Ray(lensOrigin, (focalPlaneHit - lensOrigin));

        return CameraSample{
            .ray = this->m_transform->apply(sampleRay).normalized(),
            .weight = Color(1.0f)
            };

        // hints:
        // * use m_transform to transform the local camera coordinate system into the world coordinate system
    }

    std::string toString() const override {
        return tfm::format(
            "Thinlens[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  transform = %s,\n"
            "  apertureRadius = %s,\n"
            "  focalDistance = %s,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            indent(m_transform),
            this->apertureRadius,
            this->focalDistance
        );
    }
};

}

REGISTER_CAMERA(Thinlens, "thinlens")

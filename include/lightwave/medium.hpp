/**
 * @file medium.hpp
 * @brief Contains the Medium interface and related structures.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/color.hpp>
#include <lightwave/math.hpp>

namespace lightwave {

/// @brief A Medium, representing the medium inside of an instance.
class Medium : public Object {
public:
    Medium() {}

    /**
     * @brief Evaluates the beam trancmittance along the ray up to its maxT.
     * 
     * @param ray The ray for which to compute the trancmittance in local coordinates.
     * @param A random number generator used to steer sampling decisions.
     */
    virtual float Tr(const Ray &ray, const float tIntersection, Sampler &rng) const = 0;

    /**
     * @brief Samples the medium and decides if we go into the scattering case or pass through the medium.
    */
    virtual float sampleHitDistance(const Ray &ray, Sampler &rng) const = 0;

    virtual Color getColor() const = 0;

    virtual float getSigmaS() const = 0;

    virtual float probabilityOfSampelingBeforeT(float t) const = 0;

    virtual float probabilityOfSampelingThisPoint(float t) const = 0;

    virtual Vector samplePhase(Intersection &its, Sampler &rng) const = 0;
};

}

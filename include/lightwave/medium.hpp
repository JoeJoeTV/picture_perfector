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
    virtual Color Tr(const Ray &ray, Sampler &rng) const = 0;
};

}

#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>
#include <lightwave/integrator.hpp>

namespace lightwave {

void Instance::transformFrame(SurfaceEvent &surf) const {
    // hints:
    // * transform the hitpoint and frame here
    // * if m_flipNormal is true, flip the direction of the bitangent (which in effect flips the normal)
    // * make sure that the frame is orthonormal (you are free to change the bitangent for this, but keep
    //   the direction of the transformed tangent the same)

    surf.position = this->m_transform->apply(surf.position);

    // Apply normal map if requested
    if (this->m_normal != nullptr) {
        // Read normal vector from normal map texture
        const Color nc = this->m_normal->evaluate(surf.uv);
        // Map x,y and z values into [-1,1]
        const Vector n = Vector(
            2.0f * nc.r() - 1.0f,
            2.0f * nc.g() - 1.0f,
            2.0f * nc.b() - 1.0f
        ).normalized();
        // Go from shading space to object space
        surf.frame.normal = surf.frame.toWorld(n).normalized();
    }

    // Transform normal using transpose of inverse to world space
    surf.frame.normal = this->m_transform->applyNormal(surf.frame.normal);

    // Build orthonormal basis using default constructor, which is good enough for now
    surf.frame = Frame(surf.frame.normal);
}

bool Instance::intersect(const Ray &worldRay, Intersection &its, Sampler &rng) const {
    Intersection originalIts = its;
    
    if (!m_transform) {
        // fast path, if no transform is needed
        Ray localRay = worldRay;

        if (m_shape->intersect(localRay, its, rng)) {
            // If this instance is used as a portal, we have to perform more logic
            if (this->m_link) {
                // Check if portal mask is hit. If not, treat it as if no intersection happened at all
                if (this->m_link->shouldTeleport(this, its)) {
                    its.forward.doForward = true;
                    its.forward.ray = this->m_link->getTeleportedRay(this, localRay, its.position);
                } else {
                    // Restore entire original intersection, because it was modified by shape intersect function
                    its = originalIts;
                    return false;
                }
            } else {
                // We know that this instance is not a portal and this instance is in front of any previous portal, so we can reset the values
                its.forward.doForward = false;
            }
            
            its.instance = this;

            return true;
        } else {
            return false;
        }
    }

    const float previousT = its.t;
    Ray localRay;

    localRay = this->m_transform->inverse(worldRay).normalized();
    
    // If intersection object contains previous hit, re-calculate t in local space,
    // so that comparison in shape intersect methods works as expected
    if (its) {
        its.t = (localRay.origin - this->m_transform->inverse(its.position)).length();
    }

    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        // If this instance is used as a portal, we have to perform more logic
        if (this->m_link) {
            // Check if portal mask is hit. If not, treat it as if no intersection happened at all
            if (this->m_link->shouldTeleport(this, its)) {
                its.forward.doForward = true;
                its.forward.ray = this->m_link->getTeleportedRay(this, localRay, its.position);
            } else {
                DEBUG_PIXEL_LOG("[Instance/%s] Ray: o=%s d=%s  No Intersection (Not teleported)", this->id(), worldRay.origin, worldRay.direction);

                // Restore entire original intersection, because it was modified by shape intersect function
                its = originalIts;
                return false;
            }
        } else {
            // We know that this instance is not a portal and this instance is in front of any previous portal, so we can reset the values
            its.forward.doForward = false;
        }

        // We know that we hit the shape, so set related data and return true
        its.instance = this;
        
        transformFrame(its);

        // Calculate new t in world space
        its.t = (its.position - worldRay.origin).length();

        DEBUG_PIXEL_LOG("[Instance/%s] Ray: o=%s d=%s  Intersection: t=%f pos=%s", this->id(), worldRay.origin, worldRay.direction, its.t, its.position);

        return true;
    } else {
        DEBUG_PIXEL_LOG("[Instance/%s] Ray: o=%s d=%s  No Intersection", this->id(), worldRay.origin, worldRay.direction);

        its.t = previousT;
        return false;
    }
}

Bounds Instance::getBoundingBox() const {
    if (!m_transform) {
        // fast path
        return m_shape->getBoundingBox();
    }

    const Bounds untransformedAABB = m_shape->getBoundingBox();
    if (untransformedAABB.isUnbounded()) {
        return Bounds::full();
    }

    Bounds result;
    for (int point = 0; point < 8; point++) {
        Point p = untransformedAABB.min();
        for (int dim = 0; dim < p.Dimension; dim++) {
            if ((point >> dim) & 1) {
                p[dim] = untransformedAABB.max()[dim];
            }
        }
        p = m_transform->apply(p);
        result.extend(p);
    }
    return result;
}

Point Instance::getCentroid() const {
    if (!m_transform) {
        // fast path
        return m_shape->getCentroid();
    }

    return m_transform->apply(m_shape->getCentroid());
}

AreaSample Instance::sampleArea(Sampler &rng) const {
    AreaSample sample = m_shape->sampleArea(rng);

    // calculate how the area changes
    Vector tangent = m_transform->apply(sample.frame.tangent);
    Vector bitangent = m_transform->apply(sample.frame.bitangent);
    float crossProductLength = tangent.cross(bitangent).length();

    // scale the 
    sample.area *= crossProductLength;
    transformFrame(sample);
    return sample;
}

}

REGISTER_CLASS(Instance, "instance", "default")
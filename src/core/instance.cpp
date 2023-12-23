#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

Transform getRotationTransform(const Vector &a, const Vector &b) {
    const Vector ab_cross = a.cross(b).normalized();
    const float ab = a.dot(b);

    //logger(EInfo, "Dot product: %f", ab);

    const double angle = acos(ab);

    //logger(EInfo, "Rotation angle: %f", angle * Rad2Deg);

    Transform T;

    T.rotate(ab_cross, angle);

    return T;
}

void Instance::transformFrame(SurfaceEvent &surf) const {
    // hints:
    // * transform the hitpoint and frame here
    // * if m_flipNormal is true, flip the direction of the bitangent (which in effect flips the normal)
    // * make sure that the frame is orthonormal (you are free to change the bitangent for this, but keep
    //   the direction of the transformed tangent the same)

    surf.position = this->m_transform->apply(surf.position);

    // Keep tangent direction, transfrom and normalize it
    surf.frame.tangent = this->m_transform->apply(surf.frame.tangent).normalized();
    surf.frame.bitangent = this->m_transform->apply(surf.frame.bitangent).normalized();

    if (this->m_flipNormal) {
        surf.frame.bitangent = -surf.frame.bitangent;
    }

    surf.frame.normal = surf.frame.tangent.cross(surf.frame.bitangent).normalized();
    surf.frame.bitangent = surf.frame.normal.cross(surf.frame.tangent).normalized();

    /// Apply normal map
    if (this->m_normal != nullptr) {
        const Color c = this->m_normal->evaluate(surf.uv);
        //const Color tc = (2.0f * c) - Color(1.0f);
        const Color tc = Color(
            2.0f * c.r() - 1.0f,
            2.0f * c.g() - 1.0f,
            c.b()
        );
        //const Color tc = c;
        const Vector n = Vector(tc.r(), tc.g(), tc.b());
        const Vector nw = surf.frame.toWorld(n).normalized();

        //logger(EInfo, "-----------------------------------");

        //logger(EInfo, "Image Map Normal vector: %s", surf.frame.normal);

        const Transform T = getRotationTransform(surf.frame.normal, nw);

        //logger(EInfo, "Unmapped Normal vector: %s", nw);

        surf.frame.normal = T.apply(surf.frame.normal).normalized();
        surf.frame.tangent = T.apply(surf.frame.tangent).normalized();
        surf.frame.bitangent = T.apply(surf.frame.bitangent).normalized();

        //logger(EInfo, "Mapped World Normal vector: %s", surf.frame.normal);
    }

}

bool Instance::intersect(const Ray &worldRay, Intersection &its, Sampler &rng) const {
    if (!m_transform) {
        // fast path, if no transform is needed
        Ray localRay = worldRay;
        if (m_shape->intersect(localRay, its, rng)) {
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

    // hints:
    // * transform the ray (do not forget to normalize!)
    // * how does its.t need to change?

    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        // hint: how does its.t need to change?
        
        its.instance = this;
        transformFrame(its);

        // Calculate new t in world space
        its.t = (its.position - worldRay.origin).length();

        return true;
    } else {
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
    transformFrame(sample);
    return sample;
}

}

REGISTER_CLASS(Instance, "instance", "default")
#include <lightwave.hpp>
#include <misc/portal_link.hpp>

// Thinking with portals

namespace lightwave {

void PortalLink::registerPortal(const Instance* portal, const ref<Transform> transform) {
    if (this->m_firstPortal.instance == nullptr) {
        this->m_firstPortal.instance = portal;
        this->m_firstPortal.transform = transform;

        if (PORTALS_DEBUG) logger(EInfo, "Registered instance as first portal: %s", portal);
    } else if (this->m_secondPortal.instance == nullptr) {
        this->m_secondPortal.instance = portal;
        this->m_secondPortal.transform = transform;

        if (PORTALS_DEBUG) logger(EInfo, "Registered instance as second portal: %s", portal);
    } else {
        lightwave_throw("A third instance tried to register itself with a portal link, which can only hold two linked portals!");
    }
}

Ray PortalLink::getTeleportedRay(const Instance* portal, const Ray &incomingRay, const Point &origin) {
    PortalData destPortal;

    if (portal == this->m_firstPortal.instance) {
        // The ray goes into the first portal and should come out of the second portal
        destPortal = this->m_secondPortal;
    } else if (portal == this->m_secondPortal.instance) {
        // The ray goes into the second portal and should come out of the first portal
        destPortal = this->m_firstPortal;
    } else {
        lightwave_throw("getTeleportedRay called with instance that is not registered as a portal: %s", portal);
    }

    if (destPortal.transform) {
        return Ray(
            destPortal.transform->apply(origin),
            destPortal.transform->apply(incomingRay.direction).normalized(),
            incomingRay.depth + 1
        );
    } else {
        return Ray(incomingRay.origin, incomingRay.direction, incomingRay.depth + 1);
    }
}

bool PortalLink::shouldTeleport(const Instance* portal, const Intersection &hit) {
    //TODO: Implement Image mask
    return true;
}
    
}

REGISTER_CLASS(PortalLink, "link", "default")
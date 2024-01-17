#include <lightwave.hpp>
#include <misc/portal_link.hpp>

namespace lightwave {

void PortalLink::registerPortal(const ref<Instance> portal, const ref<Transform> transform) {
    if (this->m_firstPortal.instance == nullptr) {
        this->m_firstPortal.instance = portal;
    } else if (this->m_secondPortal.instance == nullptr) {
        this->m_secondPortal.instance = portal;
    } else {
        lightwave_throw("A third instance tried to register itself with a portal link, which can only hold two linked portals!");
    }
}

Ray PortalLink::getTeleportedRay(const ref<Instance> portal, const Ray &incomingRay) {
    if (portal == this->m_firstPortal.instance) {
        // The ray goes into the first portal and should come out of the second portal
        if (this->m_secondPortal.transform) {
            return Ray(
                this->m_secondPortal.transform->apply(incomingRay.origin),
                this->m_secondPortal.transform->apply(incomingRay.direction).normalized(),
                incomingRay.depth + 1
            );
        } else {
            return Ray(incomingRay.origin, incomingRay.direction, incomingRay.depth + 1);
        }
    } else if (portal == this->m_secondPortal.instance) {
        // The ray goes into the second portal and should come out of the first portal
        if (this->m_firstPortal.transform) {
            return Ray(
                this->m_firstPortal.transform->apply(incomingRay.origin),
                this->m_firstPortal.transform->apply(incomingRay.direction).normalized(),
                incomingRay.depth + 1
            );
        } else {
            return Ray(incomingRay.origin, incomingRay.direction, incomingRay.depth + 1);
        }
    } else {
        lightwave_throw("getTeleportedRay called using instance not registered as portal: %s", portal);
    }
}
    
}

REGISTER_CLASS(PortalLink, "link", "default")
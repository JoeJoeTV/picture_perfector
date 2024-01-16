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
    //TODO: Implement
    NOT_IMPLEMENTED
}
    
}

REGISTER_CLASS(PortalLink, "link", "default")
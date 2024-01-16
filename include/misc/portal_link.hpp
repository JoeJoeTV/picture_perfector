#include <lightwave.hpp>

#pragma once

namespace lightwave {

typedef struct PortalData {
    ref<Instance> instance = nullptr;
    ref<Transform> transform = nullptr;
} PortalData;

class PortalLink : public Object {
    /// @brief A texture defining the "shape" of the portal surface on the plane
    ref<Texture> m_portalSurface;

    /// @brief Data of the two linked portals
    PortalData m_firstPortal;
    PortalData m_secondPortal;
public:
    PortalLink(const Properties &properties) {
        m_portalSurface = properties.get<Texture>("portal_surface", nullptr);
    }

    /// @brief Registers an instance as either the first or second portal
    /// @param portal The instance representing the portal
    /// @param transform The transform from local to world space
    void registerPortal(const ref<Instance> portal, const ref<Transform> transform);

    /// @brief Given a portal instance and an incoming ray, returns the ray that was "teleported" to the other portal
    /// @param portal The instance representing the portal
    /// @param incomingRay The ray that hit the portal surface
    /// @return The "teleported" Ray
    Ray getTeleportedRay(const ref<Instance> portal, const Ray &incomingRay);

    std::string toString() const override {
        return tfm::format(
            "PortalLink[\n"
            "  portal_surface = %s,\n"
            "  portal #1 = %s,\n"
            "  portal #2 = %s,\n"
            "]",
            indent(m_portalSurface),
            m_firstPortal.instance == nullptr ? "" : indent(m_firstPortal.instance),
            m_secondPortal.instance == nullptr ? "" : indent(m_secondPortal.instance)
        );
    }
};
    
}
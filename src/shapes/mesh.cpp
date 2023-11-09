#include <lightwave.hpp>

#include "../core/plyparser.hpp"
#include "accel.hpp"

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of triangles, which share an index and vertex buffer.
 * Since individual triangles are rarely needed (and would pose an excessive amount of overhead), collections of
 * triangles are combined in a single shape.
 */
class TriangleMesh : public AccelerationStructure {
    /**
     * @brief The index buffer of the triangles.
     * The n-th element corresponds to the n-th triangle, and each component of the element corresponds to one
     * vertex index (into @c m_vertices ) of the triangle.
     * This list will always contain as many elements as there are triangles.
     */
    std::vector<Vector3i> m_triangles;
    /**
     * @brief The vertex buffer of the triangles, indexed by m_triangles.
     * Note that multiple triangles can share vertices, hence there can also be fewer than @code 3 * numTriangles @endcode
     * vertices.
     */
    std::vector<Vertex> m_vertices;
    /// @brief The file this mesh was loaded from, for logging and debugging purposes.
    std::filesystem::path m_originalPath;
    /// @brief Whether to interpolate the normals from m_vertices, or report the geometric normal instead.
    bool m_smoothNormals;

protected:
    int numberOfPrimitives() const override {
        return int(m_triangles.size());
    }

    /**
     * @brief Constructs a surface event for a given position, used by @ref intersect to populate the @ref Intersection
     * and by @ref sampleArea to populate the @ref AreaSample .
     * @param surf The surface event to populate with texture coordinates, shading frame and area pdf
     * @param position The hitpoint (i.e., point in [-1,-1,0] to [+1,+1,0]), found via intersection or area sampling
     */
    inline void populate(SurfaceEvent &surf, const Point &position, const Vertex v0, const Vertex v1, const Vertex v2) const {
        surf.position = position;
        
        // map the position from [-1,-1,0]..[+1,+1,0] to [0,0]..[1,1] by discarding the z component and rescaling
        // I did not yet adjust this!
        surf.uv.x() = (position.x() + 1) / 2;
        surf.uv.y() = (position.y() + 1) / 2;

        // TODO fix the tangent and bitangent...
        // I dont know how they point on a sphere
        // the tangent always points in positive x direction
        surf.frame.tangent = Vector(1, 0, 0);
        // the bitagent always points in positive y direction
        surf.frame.bitangent = Vector(0, 1, 0);
        // and accordingly, the normal always points away from the center
        if (m_smoothNormals) {
            //surf.frame.normal = Vertex::interpolate(position, v0, v1, v2);
        }
        surf.frame.normal = Vector(position);

        // TODO
        surf.pdf = 0;
    }

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its, Sampler &rng) const override {
        // get vertices
        Vertex v0 = m_vertices[m_triangles[primitiveIndex][0]];
        Vertex v1 = m_vertices[m_triangles[primitiveIndex][1]];
        Vertex v2 = m_vertices[m_triangles[primitiveIndex][2]];

        float a, f, u, v;
        Vector edge1 = v1.position - v0.position;
        Vector edge2 = v2.position - v0.position;
        Vector h = ray.direction.cross(edge2);
        a = edge1.dot(h);

        if (a > -Epsilon && a < Epsilon)
            return false;    // This ray is parallel to this triangle.

        f = 1.0 / a;
        Vector s = ray.origin - v0.position;
        u = f * s.dot(h);

        if (u < 0.0 || u > 1.0)
            return false;

        Vector q = s.cross(edge1);
        v = f * ray.direction.dot(q);

        if (v < 0.0 || u + v > 1.0)
            return false;

        // At this stage we can compute t to find out where the intersection point is on the line.
        float t = f * edge2.dot(q);

        if (t < Epsilon) // ray intersection
        {
            return false;
        }
        
        if (its.t < t)
        {
            return false;
        }

        its.t = t;
        Point position = ray(t);
        //populate(its, position);
        its.position = position;

        if (m_smoothNormals) {
            its.frame.normal = Vertex::interpolate(Vector2(u, v), v0, v1, v2).normal.normalized();
        } else {
            its.frame.normal = edge1.cross(edge2).normalized();
        }

        its.frame.tangent = edge1.normalized();
        its.frame.bitangent = edge1.cross(its.frame.normal).normalized();

        return true;
        //

        // hints:
        // * use m_triangles[primitiveIndex] to get the vertex indices of the triangle that should be intersected
        // * if m_smoothNormals is true, interpolate the vertex normals from m_vertices
        //   * make sure that your shading frame stays orthonormal!
        // * if m_smoothNormals is false, use the geometrical normal (can be computed from the vertex positions)
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        float maxX, maxY, maxZ;
        float minX, minY, minZ;

        Vertex v0 = m_vertices[m_triangles[primitiveIndex][0]];
        Vertex v1 = m_vertices[m_triangles[primitiveIndex][1]];
        Vertex v2 = m_vertices[m_triangles[primitiveIndex][2]];

        maxX = max(v0.position[0], max(v1.position[0], v2.position[0]));
        maxY = max(v0.position[1], max(v1.position[1], v2.position[1]));
        maxZ = max(v0.position[2], max(v1.position[2], v2.position[2]));

        minX = min(v0.position[0], min(v1.position[0], v2.position[0]));
        minY = min(v0.position[1], min(v1.position[1], v2.position[1]));
        minZ = min(v0.position[2], min(v1.position[2], v2.position[2]));
        return Bounds(Point { minX, minY, minZ }, Point { maxX, maxY, maxZ });
    }

    Point getCentroid(int primitiveIndex) const override {
        Vertex v0 = m_vertices[m_triangles[primitiveIndex][0]];
        Vertex v1 = m_vertices[m_triangles[primitiveIndex][1]];
        Vertex v2 = m_vertices[m_triangles[primitiveIndex][2]];

        return Point((v0.position[0]+v1.position[0]+v2.position[0]) / 3.f, 
                     (v0.position[1]+v1.position[1]+v2.position[1]) / 3.f,
                     (v0.position[2]+v1.position[2]+v2.position[2]) / 3.f);
    }

public:
    TriangleMesh(const Properties &properties) {
        m_originalPath = properties.get<std::filesystem::path>("filename");
        m_smoothNormals = properties.get<bool>("smooth", true);
        readPLY(m_originalPath.string(), m_triangles, m_vertices);
        logger(EInfo, "loaded ply with %d triangles, %d vertices",
            m_triangles.size(),
            m_vertices.size()
        );
        buildAccelerationStructure();
    }

    AreaSample sampleArea(Sampler &rng) const override {
        // only implement this if you need triangle mesh area light sampling for your rendering competition
        NOT_IMPLEMENTED
    }

    std::string toString() const override {
        return tfm::format(
            "Mesh[\n"
            "  vertices = %d,\n"
            "  triangles = %d,\n"
            "  filename = \"%s\"\n"
            "]",
            m_vertices.size(),
            m_triangles.size(),
            m_originalPath.generic_string()
        );
    }
};

}

REGISTER_SHAPE(TriangleMesh, "mesh")

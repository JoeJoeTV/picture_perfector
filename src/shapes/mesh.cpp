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

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its, Sampler &rng) const override {
        Vector3i triangle = m_triangles[primitiveIndex];

        Vertex vertex1 = m_vertices[triangle[0]];   // "Start" vertex
        Vertex vertex2 = m_vertices[triangle[1]];
        Vertex vertex3 = m_vertices[triangle[2]];

        const Vector planeEdge1 = vertex2.position - vertex1.position;    // v1 -> v2  => e1
        const Vector planeEdge2 = vertex3.position - vertex1.position;    // v1 -> v3  => e2

        // Solve    o + t * d = v_0 + w_1 * (v_1 - v_0) + w_2 * (v_2 - v_0)     (Barycentric coordinates in triangle and ray equation)
        //       => w_1 * e_1 + w_2 * e_2 - t * d = o - v_0
        //
        // -> Solve System of Linear Equations:
        //
        //      /   -d_x    e_1x    e_2x \   / t   \.
        //      |   -d_y    e_1y    e_2y | * | w_1 |  =  o - v_0
        //      \   -d_z    e_1z    e_2z /   \ w_2 /     
        //
        // -> The triple product of three vectors can be used to calculate the determinant of the matrix with them as column vectors
        // -> Use Cramer's rule in addition

        // Calculate determinant of original matrix using triple product
        const Vector crossRayEdge2 = ray.direction.cross(planeEdge2);
        const float matrixDet = planeEdge1.dot(crossRayEdge2);

        // If determinant, thus the triple product is 0, the ray is in the plane of the triangle
        // normal epsilon was not small enough. But we cant make it smaler because we will get self intersections otherwise
        if ((-1e-8f < matrixDet) and (matrixDet < 1e-8f)) {
            return false;
        }

        // Factor to scale other determinant by (Multiply by this instead of dividing every time)
        const float scaleFactor = 1.0f / matrixDet;

        // Get right side vector of equation (distance from ray origin to first vertex of triangle)
        const Vector rayToVert = ray.origin - vertex1.position;

        // Get first barycentric coordinate from determinant using Cramer's rule
        const float baryU = rayToVert.dot(crossRayEdge2) * scaleFactor;

        // Since we're working with barycentric coordinates, we know that each coordinate can't be less than 0 or greater than 1 to be in the triangle
        if ((baryU < 0.0f) or (baryU > 1.0f)) {
            return false;
        }

        // Calculate second barycentric coordinate
        const Vector crossRayToVertEdge1 = rayToVert.cross(planeEdge1);

        const float baryV = ray.direction.dot(crossRayToVertEdge1) * scaleFactor;

        // Again, check that the barycentric coordinate is in bounds
        if ((baryV < 0.0f) or (baryU + baryV > 1.0f)) {
            return false;
        }

        // Calculate hit point of ray on triangle
        const float t = planeEdge2.dot(crossRayToVertEdge1) * scaleFactor;

        const Point hitPoint = ray(t);

        // If hit is too small or close hit already exists, don't update
        if (t < Epsilon || t > its.t) {
            return false;
        }

        // We have successfully found a hit, so update intersection
        its.t = t;
        its.position = hitPoint;

        const Vertex interpolatedVertex = Vertex::interpolate(Vector2(baryU, baryV), vertex1, vertex2, vertex3);

        its.uv = interpolatedVertex.texcoords;

        // Use one triangle edge as tangent and get bitangent from cross product
        if (this->m_smoothNormals) {
            its.frame.normal = interpolatedVertex.normal.normalized();
            its.frame.tangent = interpolatedVertex.normal.cross(planeEdge1).normalized();
            its.frame.bitangent = interpolatedVertex.normal.cross(its.frame.tangent).normalized();
        } else {
            const Vector normal = planeEdge1.cross(planeEdge2).normalized();

            its.frame.normal = normal;
            its.frame.tangent = planeEdge1.normalized();
            its.frame.bitangent = normal.cross(planeEdge1).normalized();
        }

        its.pdf = 0.0f;

        return true;

        // hints:
        // * use m_triangles[primitiveIndex] to get the vertex indices of the triangle that should be intersected
        // * if m_smoothNormals is true, interpolate the vertex normals from m_vertices
        //   * make sure that your shading frame stays orthonormal!
        // * if m_smoothNormals is false, use the geometrical normal (can be computed from the vertex positions)
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        Vector3i triangle = m_triangles[primitiveIndex];

        Vertex vertex1 = m_vertices[triangle[0]];
        Vertex vertex2 = m_vertices[triangle[1]];
        Vertex vertex3 = m_vertices[triangle[2]];

        float minX = min(vertex1.position.x(), min(vertex2.position.x(), vertex3.position.x()));
        float minY = min(vertex1.position.y(), min(vertex2.position.y(), vertex3.position.y()));
        float minZ = min(vertex1.position.z(), min(vertex2.position.z(), vertex3.position.z()));
        float maxX = max(vertex1.position.x(), max(vertex2.position.x(), vertex3.position.x()));
        float maxY = max(vertex1.position.y(), max(vertex2.position.y(), vertex3.position.y()));
        float maxZ = max(vertex1.position.z(), max(vertex2.position.z(), vertex3.position.z()));

        return Bounds(Point{minX, minY, minZ}, Point{maxX, maxY, maxZ});
    }

    Point getCentroid(int primitiveIndex) const override {
        Vector3i triangle = m_triangles[primitiveIndex];

        Vertex vertex1 = m_vertices[triangle[0]];
        Vertex vertex2 = m_vertices[triangle[1]];
        Vertex vertex3 = m_vertices[triangle[2]];

        return Point(
            (vertex1.position.x() + vertex2.position.x() + vertex3.position.x()) / 3,
            (vertex1.position.y() + vertex2.position.y() + vertex3.position.y()) / 3,
            (vertex1.position.z() + vertex2.position.z() + vertex3.position.z()) / 3
        );
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
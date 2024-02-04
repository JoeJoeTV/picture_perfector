/**
 * @file transform.hpp
 * @brief Contains the Transform interface.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/color.hpp>
#include <lightwave/math.hpp>

namespace lightwave {

/**
 * @brief Transfers points or vectors from one coordinate system to another.
 * @note This is an interface to allow time-dependent transforms (e.g., motion blur), or non-linear transforms (be creative!)
 */
class Transform : public Object {
protected:
    Matrix4x4 m_transform = Matrix4x4::identity();
    Matrix4x4 m_inverse = Matrix4x4::identity();

public:
    Transform() {}
    Transform(const Properties &) {}

    /// @brief Transforms the given point.
    Point apply(const Point &point) const {
        const Vector4 result = m_transform * Vector4(Vector(point), 1);
        return Vector(result.x(), result.y(), result.z()) / result.w();
    }

    /// @brief Transforms the given vector.
    Vector apply(const Vector &vector) const {
        const Vector4 result = m_transform * Vector4(vector, 0);
        return Vector(result.x(), result.y(), result.z());
    }

    /// @brief Transforms the given point.
    PointReal apply(const PointReal &point) const {
        const Vector4Real result = m_transform.cast<autodiff::real>() * Vector4Real(VectorReal(point), 1);
        return VectorReal(result.x(), result.y(), result.z()) / result.w();
    }

    /// @brief Transforms the given vector.
    VectorReal apply(const VectorReal &vector) const {
        const Vector4Real result = m_transform.cast<autodiff::real>() * Vector4Real(vector, 0);
        return VectorReal(result.x(), result.y(), result.z());
    }

    /**
     * @brief Transforms the given ray.
     * @warning The ray direction will not be normalized, as its transformed length
     * is typically useful for other tasks (e.g., instancing).
     */
    Ray apply(const Ray &ray) const {
        Ray result(ray);
        result.origin = apply(ray.origin);
        result.direction = apply(ray.direction);
        return result;
    }

    /// @brief Applies the inverse transform to the given point.
    Point inverse(const Point &point) const {
        const Vector4 result = m_inverse * Vector4(Vector(point), 1);
        return Vector(result.x(), result.y(), result.z()) / result.w();
    }

    /// @brief Applies the inverse transform to the given vector.
    Vector inverse(const Vector &vector) const {
        const Vector4 result = m_inverse * Vector4(vector, 0);
        return Vector(result.x(), result.y(), result.z());
    }

    /// @brief Applies the inverse transform to the given point.
    PointReal inverse(const PointReal &point) const {
        const Vector4Real result = m_inverse.cast<autodiff::real>() * Vector4Real(VectorReal(point), 1);
        return VectorReal(result.x(), result.y(), result.z()) / result.w();
    }

    /// @brief Applies the inverse transform to the given vector.
    VectorReal inverse(const VectorReal &vector) const {
        const Vector4Real result = m_inverse.cast<autodiff::real>() * Vector4Real(vector, 0);
        return VectorReal(result.x(), result.y(), result.z());
    }

    /**
     * @brief Transforms the given ray.
     * @warning The ray direction will not be normalized.
     */
    Ray inverse(const Ray &ray) const {
        Ray result(ray);
        result.origin = inverse(ray.origin);
        result.direction = inverse(ray.direction);
        return result;
    }

    /// @brief Appends a matrix in homogeneous coordinates to this transform.
    void matrix(const Matrix4x4 &value) {
        m_transform = value * m_transform;
        if (auto inv = invert(value)) {
            m_inverse = m_inverse * *inv;
        } else {
            lightwave_throw("transform is not invertible");
        }
    }

    /// @brief Appends a translation to this transform.
    void translate(const Vector &translation) {
        m_transform = Matrix4x4 {
            1, 0, 0, translation.x(),
            0, 1, 0, translation.y(),
            0, 0, 1, translation.z(),
            0, 0, 0, 1
        } * m_transform;
        m_inverse = m_inverse * Matrix4x4 {
            1, 0, 0, -translation.x(),
            0, 1, 0, -translation.y(),
            0, 0, 1, -translation.z(),
            0, 0, 0, 1
        };
    }

    /// @brief Appends a (potentially non-uniform) scaling to this transform.
    void scale(const Vector &scaling) {
        if (scaling.product() == 0) {
            lightwave_throw("scaling is not invertible");
        }

        m_transform = Matrix4x4 {
            scaling.x(), 0, 0, 0,
            0, scaling.y(), 0, 0,
            0, 0, scaling.z(), 0,
            0, 0, 0, 1
        } * m_transform;
        m_inverse = m_inverse * Matrix4x4 {
            1 / scaling.x(), 0, 0, 0,
            0, 1 / scaling.y(), 0, 0,
            0, 0, 1 / scaling.z(), 0,
            0, 0, 0, 1
        };
    }

    /// @brief Appends a rotation around the given axis to this transform.
    void rotate(const Vector &axis, float angle) {
        const auto u = axis.normalized();
        const float cos = std::cos(angle);
        const float sin = std::sin(angle);

        auto rotation = Matrix4x4::identity();
        for (int row = 0; row < axis.Dimension; row++) {
            for (int column = 0; column < axis.Dimension; column++) {
                rotation(row, column) = (1 - cos) * u[row] * u[column] + (
                    row == column ?
                        cos :
                        (row == (column + 1) % axis.Dimension ? +1 : -1) * sin * u[((axis.Dimension - row) + (axis.Dimension - column)) % axis.Dimension]
                );
            }
        }

        m_transform = rotation * m_transform;
        m_inverse = m_inverse * rotation.transpose();
    }

    /**
     * @brief Appends a "lookat" operation to this transform, which is useful to aim cameras or light sources at other objects.
     * The z-axis will be re-oriented to be aligned with @code target - origin @endcode , and the y-axis will be in the plane
     * that the @c up vector lies in.
     */
    void lookat(const Vector &origin, const Vector &target, const Vector &up) {
        const auto direction = (target - origin).normalized();
        if (up.cross(direction).isZero()) {
            lightwave_throw("lookat: direction (%s) and up vector (%s) must not be colinear", direction, up);
        }
        const auto left = up.cross(direction).normalized();
        const auto orthogonalUp = direction.cross(left).normalized();

        Matrix4x4 matrix;
        matrix.setColumn(0, Vector4(left, 0));
        matrix.setColumn(1, Vector4(orthogonalUp, 0));
        matrix.setColumn(2, Vector4(direction, 0));
        matrix.setColumn(3, Vector4(origin, 1));
        
        m_transform = matrix * m_transform;

        matrix.setColumn(3, Vector4(0, 0, 0, 1));
        matrix = matrix.transpose();
        matrix.setColumn(3, Vector4(-origin, 1));

        m_inverse = m_inverse * matrix;
    }

    /// @brief Returns the determinant of this transformation. 
    float determinant() const {
        return m_transform.submatrix<3, 3>(0, 0).determinant();
    }

    std::string toString() const override {
        return tfm::format(
            "Transform[\n"
            "  matrix = %s,\n"
            "  inverse = %s,\n"
            "]",
            indent(m_transform),
            indent(m_inverse)
        );
    }

    /*
     *  Added functionality
     */

    // "New Divide" starts playing...

    /// @brief Removes the scaling factor from the transformation and returns it as a vector
    /// @return The scaling factors in each direction
    Vector popScale() {
        // Get first three columns of 3x3 submatrix of transform matrix
        const Vector c_x = m_transform.submatrix<3, 1>(0, 0).column(0);
        const Vector c_y = m_transform.submatrix<3, 1>(0, 1).column(0);
        const Vector c_z = m_transform.submatrix<3, 1>(0, 2).column(0);

        // Extract scale vector
        const Vector scale = Vector(
            c_x.length(),
            c_y.length(),
            c_z.length()
        );

        // Set first three columns of 3x3 submatrix to normalized versions to remove scaling
        m_transform.setColumn(0, Vector4(c_x.normalized(), m_transform(3, 0)));
        m_transform.setColumn(1, Vector4(c_y.normalized(), m_transform(3, 1)));
        m_transform.setColumn(2, Vector4(c_z.normalized(), m_transform(3, 2)));

        // Get first three columns of 3x3 submatrix of inverse matrix
        const Vector ci_x = m_inverse.submatrix<3, 1>(0, 0).column(0);
        const Vector ci_y = m_inverse.submatrix<3, 1>(0, 1).column(0);
        const Vector ci_z = m_inverse.submatrix<3, 1>(0, 2).column(0);

        // Set first three columns of 3x3 submatrix of inverse to normalized versions to remove scaling
        m_inverse.setColumn(0, Vector4(ci_x.normalized(), m_inverse(3, 0)));
        m_inverse.setColumn(1, Vector4(ci_y.normalized(), m_inverse(3, 1)));
        m_inverse.setColumn(2, Vector4(ci_z.normalized(), m_inverse(3, 2)));

        return scale;
    }

    /// @brief Transforms a normal vector using the matrix adjoint (transpose of inverse)
    Vector applyNormal(const Vector &normal) const {
        assert_normalized(normal, {
            logger(EError, "The vector given to applyNormal is not normalized");
        });
        
        const Matrix4x4 adjoint = m_inverse.transpose();
        const Vector4 result = adjoint * Vector4(normal, 0);
        return Vector(result.x(), result.y(), result.z()).normalized();
    }
};

}

#pragma once

struct Frustum
{
    Eigen::Hyperplane<float, 3> planes[6];

    Frustum() {}

    Frustum(const Hedgehog::Math::CMatrix44& matrix)
    {
        set(matrix);
    }

    void set(const Hedgehog::Math::CMatrix44& matrix)
    {
        planes[0].coeffs()[0] = matrix(3, 0) + matrix(0, 0);
        planes[0].coeffs()[1] = matrix(3, 1) + matrix(0, 1);
        planes[0].coeffs()[2] = matrix(3, 2) + matrix(0, 2);
        planes[0].coeffs()[3] = matrix(3, 3) + matrix(0, 3);

        planes[1].coeffs()[0] = matrix(3, 0) - matrix(0, 0);
        planes[1].coeffs()[1] = matrix(3, 1) - matrix(0, 1);
        planes[1].coeffs()[2] = matrix(3, 2) - matrix(0, 2);
        planes[1].coeffs()[3] = matrix(3, 3) - matrix(0, 3);

        planes[2].coeffs()[0] = matrix(3, 0) - matrix(1, 0);
        planes[2].coeffs()[1] = matrix(3, 1) - matrix(1, 1);
        planes[2].coeffs()[2] = matrix(3, 2) - matrix(1, 2);
        planes[2].coeffs()[3] = matrix(3, 3) - matrix(1, 3);

        planes[3].coeffs()[0] = matrix(3, 0) + matrix(1, 0);
        planes[3].coeffs()[1] = matrix(3, 1) + matrix(1, 1);
        planes[3].coeffs()[2] = matrix(3, 2) + matrix(1, 2);
        planes[3].coeffs()[3] = matrix(3, 3) + matrix(1, 3);

        planes[4].coeffs()[0] = matrix(3, 0) + matrix(2, 0);
        planes[4].coeffs()[1] = matrix(3, 1) + matrix(2, 1);
        planes[4].coeffs()[2] = matrix(3, 2) + matrix(2, 2);
        planes[4].coeffs()[3] = matrix(3, 3) + matrix(2, 3);

        planes[5].coeffs()[0] = matrix(3, 0) - matrix(2, 0);
        planes[5].coeffs()[1] = matrix(3, 1) - matrix(2, 1);
        planes[5].coeffs()[2] = matrix(3, 2) - matrix(2, 2);
        planes[5].coeffs()[3] = matrix(3, 3) - matrix(2, 3);

        for (int i = 0; i < 6; i++)
            planes[i].normalize();
    }

    bool intersects(const Hedgehog::Math::CVector& point) const
    {
        for (auto& plane : planes)
        {
            if (point.dot(plane.normal()) + plane.offset() <= 0)
                return false;
        }

        return true;
    }

    bool intersects(const Hedgehog::Math::CVector& center, const float radius) const
    {
        for (auto& plane : planes)
        {
            if (center.dot(plane.normal()) + plane.offset() <= -radius)
                return false;
        }

        return true;
    }
};
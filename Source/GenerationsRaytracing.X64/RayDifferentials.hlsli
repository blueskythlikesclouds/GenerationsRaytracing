#ifndef RAY_DIFFERENTIALS_H
#define RAY_DIFFERENTIALS_H

struct RayDiff
{
    float3 dOdx;
    float3 dOdy;
    float3 dDdx;
    float3 dDdy;
};

void ComputeRayDifferentials(float3 nonNormalizedCameraRaydir, float3 cameraRight, float3 cameraUp, float2 viewportDims, out float3 dDdx, out float3 dDdy)
{
    // Igehy Equation 8
    float dd = dot(nonNormalizedCameraRaydir, nonNormalizedCameraRaydir);
    float divd = 2.0f / (dd * sqrt(dd));
    float dr = dot(nonNormalizedCameraRaydir, cameraRight);
    float du = dot(nonNormalizedCameraRaydir, cameraUp);
    dDdx = ((dd * cameraRight) - (dr * nonNormalizedCameraRaydir)) * divd / viewportDims.x;
    dDdy = -((dd * cameraUp) - (du * nonNormalizedCameraRaydir)) * divd / viewportDims.y;
}

RayDiff PropagateRayDifferentials(RayDiff rayDiff, float3 O, float3 D, float t, float3 N)
{
    float3 dodx = rayDiff.dOdx + t * rayDiff.dDdx; // Part of Igehy Equation 10.
    float3 dody = rayDiff.dOdy + t * rayDiff.dDdy;

    float rcpDN = 1.0f / dot(D, N); // Igehy Equations 10 and 12.
    float dtdx = -dot(dodx, N) * rcpDN;
    float dtdy = -dot(dody, N) * rcpDN;
    dodx += D * dtdx;
    dody += D * dtdy;

    rayDiff.dOdx = dodx;
    rayDiff.dOdy = dody;

    return rayDiff;
}

void ComputeBarycentricDifferentials(RayDiff rayDiff, float3 rayDir, float3 edge01, float3 edge02,
    float3 faceNormalW, out float2 dBarydx, out float2 dBarydy)
{
    float3 Nu = cross(edge02, faceNormalW); // Igehy "Normal-Interpolated Triangles", page 182 SIGGRAPH 1999.
    float3 Nv = cross(edge01, faceNormalW);
    float3 Lu = Nu / (dot(Nu, edge01)); // Plane equations for the triangle edges, scaled in order to make the dot with the opposive vertex = 1.
    float3 Lv = Nv / (dot(Nv, edge02));

    dBarydx.x = dot(Lu, rayDiff.dOdx); // du / dx.
    dBarydx.y = dot(Lv, rayDiff.dOdx); // dv / dx.
    dBarydy.x = dot(Lu, rayDiff.dOdy); // du / dy.
    dBarydy.y = dot(Lv, rayDiff.dOdy); // dv / dy.
}

void InterpolateDifferentials(float2 dBarydx, float2 dBarydy, float2 uv0, float2 uv1, float2 uv2, out float2 dx, out float2 dy)
{
    float2 delta1 = uv1 - uv0;
    float2 delta2 = uv2 - uv0;
    dx = dBarydx.x * delta1 + dBarydx.y * delta2;
    dy = dBarydy.x * delta1 + dBarydy.y * delta2;
}

#endif
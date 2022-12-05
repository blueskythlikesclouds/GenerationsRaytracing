float4 main(in uint vertexId : SV_VertexID) : SV_Position
{
    return float4(float2((vertexId << 1) & 2, vertexId & 2) * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
}
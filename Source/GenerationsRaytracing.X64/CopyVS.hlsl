void main(in uint vertexId : SV_VertexID, out float4 position : SV_Position, out float2 texCoord : TEXCOORD)
{
    texCoord = float2((vertexId << 1) & 2, vertexId & 2);
    position = float4(texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
}
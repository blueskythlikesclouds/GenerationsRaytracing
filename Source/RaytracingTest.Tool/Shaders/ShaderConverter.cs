using Microsoft.VisualBasic;
using RaytracingTest.Tool.Shaders.Constants;
using RaytracingTest.Tool.Shaders.Instructions;
using System.Xml.Linq;

namespace RaytracingTest.Tool.Shaders;

public static class ShaderConverter
{
    private static void WriteBarycentricLerp(StringBuilder stringBuilder, string buffer)
    {
        stringBuilder.AppendFormat("{0}[indices.x] * uv.x + {0}[indices.y] * uv.y + {0}[indices.z] * uv.z", buffer);
    }

    private static void WriteConstant(StringBuilder stringBuilder, string outName, Constant constant, ShaderType shaderType, ShaderMapping shaderMapping)
    {
        if (constant.Size == 4)
        {
            stringBuilder.AppendFormat("\t{0}.{1}[0] = float4(1, 0, 0, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[1] = float4(0, 1, 0, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[2] = float4(0, 0, 1, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[3] = float4(0, 0, 0, 1);\n", outName, constant.Name);
        }
        else if (constant.Name == "g_MtxPalette")
        {
            stringBuilder.AppendFormat("\t{0}.{1}[0] = float4(1, 0, 0, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[1] = float4(0, 1, 0, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[2] = float4(0, 0, 1, 0);\n", outName, constant.Name);
        }
        else if (constant.Name == "g_aLightField")
        {
            stringBuilder.AppendFormat(
                "\tfloat3 globalIllumination = TraceGlobalIllumination(payload, WorldRayOrigin() + WorldRayDirection() * RayTCurrent(), normalize(mul(ObjectToWorld3x4(), float4(g_NormalBuffer[indices.x] * uv.x + g_NormalBuffer[indices.y] * uv.y + g_NormalBuffer[indices.z] * uv.z, 0.0))).xyz);\n");

            stringBuilder.AppendFormat(
                "\tfloat shadow = TraceShadow(WorldRayOrigin() + WorldRayDirection() * RayTCurrent());");

            for (int i = 0; i < constant.Size; i++)
                stringBuilder.AppendFormat("\t{0}.{1}[{2}] = float4(globalIllumination, shadow);\n", outName, constant.Name, i);
        }
        else if (constant.Size <= 1)
        {
            stringBuilder.AppendFormat("\t{0}.{1} = ", outName, constant.Name);

            if (shaderMapping.CanMapFloat4(constant.Register, shaderType))
            {
                stringBuilder.AppendFormat("material.float4Parameters[{0}]", shaderMapping.MapFloat4(constant.Register, shaderType));
            }
            else
            {
                switch (constant.Name)
                {
                    case "g_EyeDirection":
                        stringBuilder.Append("float4(WorldRayDirection(), 0)");
                        break;

                    case "g_GI0Scale":
                    case "g_ForceAlphaColor":
                        stringBuilder.Append("1");
                        break;

                    case "mrgGlobalLight_Diffuse":
                    case "mrgGlobalLight_Specular":
                        stringBuilder.Append("float4(g_Globals.lightColor, 0)");
                        break;

                    case "g_EyePosition":
                        stringBuilder.Append("float4(WorldRayOrigin(), 0)");
                        break;

                    case "mrgGlobalLight_Direction":
                        stringBuilder.Append("float4(g_Globals.lightDirection, 0)");
                        break;

                    case "mrgGIAtlasParam":
                        stringBuilder.Append("float4(0, 0, 1, 1)");
                        break;

                    case "g_VerticalLightDirection":
                        stringBuilder.Append("float4(0, -1, 0, 0)");
                        break;

                    case "mrgPlayableParam":
                        stringBuilder.Append("float4(-10000, 64, 0, 0)");
                        break;

                    default:
                        stringBuilder.Append("0");
                        break;
                }
            }

            stringBuilder.AppendLine(";");
        }
    }

    private static void WriteConstants(StringBuilder stringBuilder, string outName, Shader shader, ShaderMapping shaderMapping)
    {
        foreach (var constant in shader.Constants)
        {
            if (constant.Type != ConstantType.Sampler)
                WriteConstant(stringBuilder, outName, constant, shader.Type, shaderMapping);
        }
    }

    private static void WriteShaderFunction(StringBuilder stringBuilder, string shaderName, Shader shader, ShaderMapping shaderMapping)
    {
        string inName = shader.Type == ShaderType.Pixel ? "ps" : "ia";
        string outName = shader.Type == ShaderType.Pixel ? "om" : "vs";

        string inNameUpperCase = inName.ToUpperInvariant();
        string outNameUpperCase = outName.ToUpperInvariant();

        var variableMap = new Dictionary<string, string>();

        stringBuilder.AppendFormat("struct {1}_{0}Params\n{{\n", inNameUpperCase, shaderName);

        foreach (var (semantic, name) in shader.InSemantics)
        {
            stringBuilder.AppendFormat("\tfloat4 {0}; // {1}\n", name, semantic);
            variableMap.Add(name, $"{inName}Params.{name}");
        }

        foreach (var constant in shader.Constants)
        {
            switch (constant.Type)
            {
                case ConstantType.Float4:
                    stringBuilder.AppendFormat("\tfloat4 {0}", constant.Name);

                    if (constant.Size > 1)
                    {
                        stringBuilder.AppendFormat("[{0}]", constant.Size);

                        for (int j = 0; j < constant.Size; j++)
                            variableMap.Add($"c{(constant.Register + j)}", $"{inName}Params.{constant.Name}[{j}]");
                    }

                    else
                        variableMap.Add($"c{constant.Register}", $"{inName}Params.{constant.Name}");

                    stringBuilder.AppendFormat(";\n");
                    break;

                case ConstantType.Bool:
                    stringBuilder.AppendFormat("\tbool {0};\n", constant.Name);
                    variableMap.Add($"b{constant.Register}", $"{inName}Params.{constant.Name}");
                    break;
            }
        }

        if (shader.Samplers.Count > 0)
        {
            foreach (var constant in shader.Constants)
            {
                if (constant.Type != ConstantType.Sampler)
                    continue;

                for (int j = 0; j < constant.Size; j++)
                {
                    string token = $"s{(constant.Register + j)}";
                    string name = constant.Name;

                    if (j > 0)
                        name += $"{j}";

                    stringBuilder.AppendFormat("\tTexture{0}<float4> {1};\n", shader.Samplers[token], name);
                    variableMap.Add(token, $"{inName}Params.{name}");
                }
            }
        }

        stringBuilder.AppendLine("};\n");     
        
        stringBuilder.AppendFormat("struct {1}_{0}Params\n{{\n", outNameUpperCase, shaderName);

        if (shader.Type == ShaderType.Pixel)
        {
            stringBuilder.AppendLine("\tfloat4 oC0; // COLOR0");
            stringBuilder.AppendLine("\tfloat4 oC1; // COLOR1");
            stringBuilder.AppendLine("\tfloat4 oC2; // COLOR2");
            stringBuilder.AppendLine("\tfloat4 oC3; // COLOR3");
            stringBuilder.AppendLine("\tfloat oDepth; // DEPTH");

            variableMap.Add("oC0", $"{outName}Params.oC0");
            variableMap.Add("oC1", $"{outName}Params.oC1");
            variableMap.Add("oC2", $"{outName}Params.oC2");
            variableMap.Add("oC3", $"{outName}Params.oC3");
            variableMap.Add("oDepth", $"{outName}Params.oDepth");
        }
        else
        {
            foreach (var (semantic, name) in shader.OutSemantics)
            {
                stringBuilder.AppendFormat("\tfloat4 {0}; // {1}\n", name, semantic);
                variableMap.Add(name, $"{outName}Params.{name}");
            }
        }

        stringBuilder.AppendLine("};\n");

        stringBuilder.AppendFormat("void {1}_{0}(inout {1}_{2}Params {3}Params, inout {1}_{4}Params {5}Params)\n{{\n",
            shader.Type == ShaderType.Pixel ? "PS" : "VS",
            shaderName,
            inNameUpperCase,
            inName,
            outNameUpperCase,
            outName);

        if (shader.Definitions.Count > 0)
        {
            stringBuilder.AppendFormat("\tfloat4 C[{0}];\n\n", shader.Definitions.Max(x => x.Key) + 1);

            foreach (var definition in shader.Definitions)
                stringBuilder.AppendFormat("\tC[{0}] = float4({1});\n", definition.Key, definition.Value);

            stringBuilder.AppendLine();
        }

        if (shader.DefinitionsInt.Count > 0)
        {
            foreach (var definition in shader.DefinitionsInt)
                stringBuilder.AppendFormat("\tint4 i{0} = int4({1});\n", definition.Key, definition.Value);

            stringBuilder.AppendLine();
        }

        int registerCount = shader.Instructions
            .Where(x => x.Arguments != null)
            .SelectMany(x => x.Arguments)
            .Where(x => x.Token[0] == 'r')
            .Select(x => int.Parse(x.Token.AsSpan()[1..]) + 1)
            .Append(0)
            .Max();

        for (int j = 0; j < registerCount; j++)
            stringBuilder.AppendFormat("\tfloat4 r{0} = float4(0, 0, 0, 0);\n", j);

        if (shader.Instructions.Any(x => x.Arguments != null && x.Arguments.Any(y => y.Token[0] == 'a')))
            stringBuilder.AppendLine("\tuint4 a0 = uint4(0, 0, 0, 0);");

        stringBuilder.AppendLine();

        int indent = 1;

        foreach (var instruction in shader.Instructions)
        {
            if (instruction.Arguments != null)
            {
                foreach (var argument in instruction.Arguments)
                {
                    if (variableMap.TryGetValue(argument.Token, out string constantName))
                        argument.Token = constantName;

                    else if (argument.Token[0] == 'c')
                        argument.Token = $"C[{argument.Token.Substring(1)}]";
                }
            }

            string instrLine = instruction.ToString();

            if (instrLine.Contains('}')) --indent;

            for (int j = 0; j < indent; j++)
                stringBuilder.Append("\t");

            instrLine = instrLine.Replace("][", " + ");

            stringBuilder.AppendFormat("{0}\n", instrLine);

            if (instrLine.Contains('{')) ++indent;
        }

        stringBuilder.AppendLine("}\n");
    }

    public static void ConvertToClosestHitShader(StringBuilder stringBuilder, string shaderName, Shader vertexShader, Shader pixelShader, ShaderMapping shaderMapping)
    {
        stringBuilder.AppendFormat("//\n// {0}\n//\n\n", shaderName);

        WriteShaderFunction(stringBuilder, shaderName, vertexShader, shaderMapping);
        WriteShaderFunction(stringBuilder, shaderName, pixelShader, shaderMapping);

        stringBuilder.AppendLine("[shader(\"closesthit\")]");
        stringBuilder.AppendFormat("void {0}(inout Payload payload : SV_RayPayload, Attributes attributes : SV_IntersectionAttributes)\n{{\n", shaderName);

        stringBuilder.Append("""
                uint4 mesh = g_MeshBuffer[InstanceID() + GeometryIndex()];

                uint3 indices;
                indices.x = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 0];
                indices.y = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 1];
                indices.z = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 2];

                float3 uv = float3(1.0 - attributes.uv.x - attributes.uv.y, attributes.uv.x, attributes.uv.y);

                Material material = g_MaterialBuffer[mesh.z];


            """);

        stringBuilder.AppendFormat("\t{0}_IAParams iaParams = ({0}_IAParams)0;\n", shaderName);

        foreach (var (semantic, name) in vertexShader.InSemantics)
        {
            switch (semantic)
            {
                case "POSITION":
                    stringBuilder.AppendFormat("\tiaParams.{0}.xyz = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();\n", name);
                    break;

                case "NORMAL":
                    stringBuilder.AppendFormat("\tiaParams.{0}.xyz = mul(ObjectToWorld3x4(), float4(", name);
                    WriteBarycentricLerp(stringBuilder, "g_NormalBuffer");
                    stringBuilder.AppendLine(", 0));");
                    break;    
                
                case "TANGENT":
                    stringBuilder.AppendFormat("\tiaParams.{0}.xyz = mul(ObjectToWorld3x4(), float4((", name);
                    WriteBarycentricLerp(stringBuilder, "g_TangentBuffer");
                    stringBuilder.AppendLine(").xyz, 0));");
                    break;

                case "BINORMAL":
                    stringBuilder.AppendFormat("\tiaParams.{0}.xyz = mul(ObjectToWorld3x4(), float4(cross(", name);
                    WriteBarycentricLerp(stringBuilder, "g_NormalBuffer");
                    stringBuilder.Append(", (");
                    WriteBarycentricLerp(stringBuilder, "g_TangentBuffer");
                    stringBuilder.Append(").xyz) * (");
                    WriteBarycentricLerp(stringBuilder, "g_TangentBuffer");
                    stringBuilder.AppendLine(").w, 0));");
                    break;

                case "TEXCOORD":
                case "TEXCOORD0":
                    stringBuilder.AppendFormat("\tiaParams.{0}.xy = ", name);
                    WriteBarycentricLerp(stringBuilder, "g_TexCoordBuffer");
                    stringBuilder.AppendLine(";");
                    break;    
                
                case "COLOR":
                    stringBuilder.AppendFormat("\tiaParams.{0} = ", name);
                    WriteBarycentricLerp(stringBuilder, "g_ColorBuffer");
                    stringBuilder.AppendLine(";");
                    break;
            }
        }

        WriteConstants(stringBuilder, "iaParams", vertexShader, shaderMapping);

        stringBuilder.AppendFormat("\n\t{0}_VSParams vsParams = ({0}_VSParams)0;\n", shaderName);

        stringBuilder.AppendFormat("\t{0}_VS(iaParams, vsParams);\n\n", shaderName);  
        
        stringBuilder.AppendFormat("\t{0}_PSParams psParams = ({0}_PSParams)0;\n", shaderName);

        foreach (var (semantic, name) in pixelShader.InSemantics)
        {
            if (vertexShader.OutSemantics.TryGetValue(semantic, out string targetName))
                stringBuilder.AppendFormat("\tpsParams.{0} = vsParams.{1};\n", name, targetName);
        }

        WriteConstants(stringBuilder, "psParams", pixelShader, shaderMapping);

        foreach (var constant in pixelShader.Constants)
        {
            if (constant.Type != ConstantType.Sampler)
                continue;

            for (int i = 0; i < constant.Size; i++)
            {
                int register = constant.Register + i;
                string token = $"s{register}";
                string name = constant.Name;

                if (i > 0)
                    name += $"{i}";

                stringBuilder.AppendFormat("\tpsParams.{0} = g_BindlessTexture{1}[NonUniformResourceIndex(material.textureIndices[{2}])];\n", 
                    name, 
                    pixelShader.Samplers[token], 
                    shaderMapping.CanMapSampler(register) ? shaderMapping.MapSampler(register) : 0);
            }
        }

        stringBuilder.AppendFormat("\n\t{0}_OMParams omParams = ({0}_OMParams)0;\n", shaderName);

        stringBuilder.AppendFormat("\t{0}_PS(psParams, omParams);\n\n", shaderName);

        stringBuilder.AppendFormat("\tpayload.color = omParams.oC0;\n");

        stringBuilder.AppendLine("}\n");
    }

    public static StringBuilder BeginShaderConversion()
    {
        var stringBuilder = new StringBuilder();

        stringBuilder.Append("""
            #define _rep(x, y) for (int i##y = 0; i##y < x; ++i##y)
            #define rep(x) _rep(x, __LINE__)

            #define FLT_MAX asfloat(0x7f7fffff)

            struct Payload
            {
            	float4 color;
                uint random;
                uint depth;
                bool miss;
            };

            struct Attributes
            {
            	float2 uv;
            };
 
            struct cbGlobals
            {
                float3 position;
                float tanFovy;
                float4x4 rotation;
                float aspectRatio;
                uint sampleCount;
                float3 lightDirection;
                float3 lightColor;
            };

            ConstantBuffer<cbGlobals> g_Globals : register(b0, space0);

            struct Material
            {
                float4 float4Parameters[16];
                uint textureIndices[16];
            };

            RaytracingAccelerationStructure g_BVH : register(t0, space0);

            StructuredBuffer<Material> g_MaterialBuffer : register(t1, space0);
            Buffer<uint4> g_MeshBuffer : register(t2, space0);

            Buffer<float3> g_NormalBuffer : register(t3, space0);
            Buffer<float4> g_TangentBuffer : register(t4, space0);
            Buffer<float2> g_TexCoordBuffer : register(t5, space0);
            Buffer<float4> g_ColorBuffer : register(t6, space0);

            Buffer<uint> g_IndexBuffer : register(t7, space0);

            SamplerState g_DefaultSampler : register(s0, space0);
            SamplerComparisonState g_ComparisonSampler : register(s1, space0);

            Texture2D<float4> g_BindlessTexture2D[] : register(t0, space1);
            TextureCube<float4> g_BindlessTextureCube[] : register(t0, space2);
 
            // http://intro-to-dxr.cwyman.org/

            // Generates a seed for a random number generator from 2 inputs plus a backoff
            uint initRand(uint val0, uint val1, uint backoff = 16)
            {
                uint v0 = val0, v1 = val1, s0 = 0;
                [unroll]
                for (uint n = 0; n < backoff; n++)
                {
                    s0 += 0x9e3779b9;
                    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
                    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
                }
                return v0;
            }
            void nextRandUint(inout uint s)
            {
                s = (1664525u * s + 1013904223u);
            }
            // Takes our seed, updates it, and returns a pseudorandom float in [0..1]
            float nextRand(inout uint s)
            {
                nextRandUint(s);
                return float(s & 0x00FFFFFF) / float(0x01000000);
            }
            // Utility function to get a vector perpendicular to an input vector 
            //    (from "Efficient Construction of Perpendicular Vectors Without Branching")
            float3 getPerpendicularVector(float3 u)
            {
                float3 a = abs(u);
                uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
                uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
                uint zm = 1 ^ (xm | ym);
                return cross(u, float3(xm, ym, zm));
            }

            // Get a cosine-weighted random vector centered around a specified normal direction.
            float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
            {
                // Get 2 random numbers to select our sample with
                float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));
                // Cosine weighted hemisphere sample from RNG
                float3 bitangent = getPerpendicularVector(hitNorm);
                float3 tangent = cross(bitangent, hitNorm);
                float r = sqrt(randVal.x);
                float phi = 2.0f * 3.14159265f * randVal.y;
                // Get our cosine-weighted hemisphere lobe sample direction
                return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(max(0.0, 1.0f - randVal.x));
            }
 
            float4 TraceGlobalIllumination(inout Payload payload, float3 position, float3 normal)
            {
                if (payload.depth >= 4)
                    return 0;

                RayDesc ray;
                ray.Origin = position;
                ray.Direction = getCosHemisphereSample(payload.random, normal);
                ray.TMin = 0.01f;
                ray.TMax = FLT_MAX;

                Payload payload1 = (Payload)0;
                payload1.random = payload.random;
                payload1.depth = payload.depth + 1;

                TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

                payload.random = payload1.random;
                return payload1.color;
            }

            float TraceShadow(float3 position)
            {
                RayDesc ray;
                ray.Origin = position;
                ray.Direction = -g_Globals.lightDirection;
                ray.TMin = 0.01f;
                ray.TMax = FLT_MAX;

                Payload payload = (Payload)0;
                TraceRay(g_BVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 1, 0, 1, 0, ray, payload);

                return payload.miss ? 1.0 : 0.0;
            }


            """);

        return stringBuilder;
    }
}
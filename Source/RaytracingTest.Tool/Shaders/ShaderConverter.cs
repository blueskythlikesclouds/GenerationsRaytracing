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
            for (int i = 0; i < constant.Size; i++)
                stringBuilder.AppendFormat("\t{0}.{1}[{2}] = float4(globalIllumination.rgb, shadow);\n", outName, constant.Name, i);
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
                    case "g_BackGroundScale":
                    case "g_ShadowMapSampler":
                    case "g_VerticalShadowMapSampler":
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

                    case "g_ReflectionMapSampler":
                    case "g_ReflectionMapSampler2":
                        stringBuilder.Append("reflection");
                        break;

                    case "g_FramebufferSampler":
                        stringBuilder.Append("refraction");
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

    private static void WriteShaderFunction(StringBuilder stringBuilder, string functionName, Shader shader, ShaderMapping shaderMapping)
    {
        string inName = shader.Type == ShaderType.Pixel ? "ps" : "ia";
        string outName = shader.Type == ShaderType.Pixel ? "om" : "vs";

        string inNameUpperCase = inName.ToUpperInvariant();
        string outNameUpperCase = outName.ToUpperInvariant();

        var variableMap = new Dictionary<string, string>();

        stringBuilder.AppendFormat("struct {1}_{0}Params\n{{\n", inNameUpperCase, functionName);

        foreach (var (semantic, name) in shader.InSemantics)
        {
            stringBuilder.AppendFormat("\tfloat4 {0}; // {1}\n", name, semantic);
            variableMap.Add(name, $"{inName}Params.{name}");
        }

        foreach (var constant in shader.Constants)
        {
            char token = 'c';

            if (constant.Type == ConstantType.Sampler && constant.Name.StartsWith("g_"))
            {
                constant.Type = ConstantType.Float4;
                token = 's';
            }

            switch (constant.Type)
            {
                case ConstantType.Float4:
                    stringBuilder.AppendFormat("\tfloat4 {0}", constant.Name);

                    if (constant.Size > 1)
                    {
                        stringBuilder.AppendFormat("[{0}]", constant.Size);

                        for (int j = 0; j < constant.Size; j++)
                            variableMap.Add($"{token}{(constant.Register + j)}", $"{inName}Params.{constant.Name}[{j}]");
                    }

                    else
                        variableMap.Add($"{token}{constant.Register}", $"{inName}Params.{constant.Name}");

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
        
        stringBuilder.AppendFormat("struct {1}_{0}Params\n{{\n", outNameUpperCase, functionName);

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
            functionName,
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

                if (instruction.Arguments.Length == 3 &&
                    instruction.Arguments[2].Token.Contains(".g_") &&
                    instruction.Arguments[2].Token.Contains("Sampler"))
                {
                    instruction.OpCode = "mov";
                    instruction.Arguments[1]  = instruction.Arguments[2];
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

    private static void WriteRaytracingFunction(StringBuilder stringBuilder, string functionName, string shaderType, Shader vertexShader, Shader pixelShader, ShaderMapping shaderMapping)
    {
        stringBuilder.AppendFormat("[shader(\"{0}\")]\n", shaderType);
        stringBuilder.AppendFormat("void {0}_{1}(inout Payload payload : SV_RayPayload, Attributes attributes : SV_IntersectionAttributes)\n{{\n", functionName, shaderType);

        stringBuilder.AppendLine("\tuint4 mesh = g_MeshBuffer[InstanceID() + GeometryIndex()];");

        stringBuilder.Append(""" 
                uint3 indices;
                indices.x = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 0];
                indices.y = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 1];
                indices.z = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 2];

                float3 uv = float3(1.0 - attributes.uv.x - attributes.uv.y, attributes.uv.x, attributes.uv.y);

                Material material = g_MaterialBuffer[mesh.z];


            """);

        stringBuilder.AppendFormat("\t{0}_IAParams iaParams = ({0}_IAParams)0;\n", functionName);

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
                case "TEXCOORD1":
                case "TEXCOORD2":
                case "TEXCOORD3":
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

        bool isClosestHit = shaderType == "closesthit";
        bool hasGlobalIllumination = isClosestHit && (vertexShader.Constants.Any(x => x.Name == "g_aLightField") || pixelShader.Constants.Any(x => x.Name == "g_aLightField"));
        bool hasReflection = isClosestHit && pixelShader.Constants.Any(x => x.Name == "g_ReflectionMapSampler" || x.Name == "g_ReflectionMapSampler2");
        bool hasRefraction = isClosestHit && pixelShader.Constants.Any(x => x.Name == "g_FramebufferSampler");
        bool hasShadow = isClosestHit && pixelShader.Constants.Any(x => x.Name == "g_ShadowMapSampler" || x.Name == "g_VerticalShadowMapSampler");

        if (hasGlobalIllumination || hasReflection)
            stringBuilder.Append("\tfloat3 normal = normalize(mul(ObjectToWorld3x4(), float4(g_NormalBuffer[indices.x] * uv.x + g_NormalBuffer[indices.y] * uv.y + g_NormalBuffer[indices.z] * uv.z, 0.0))).xyz;\n");

        stringBuilder.AppendFormat("\tfloat4 globalIllumination = {0};\n", hasGlobalIllumination ? "TraceGlobalIllumination(payload, normal)" : "0");
        stringBuilder.AppendFormat("\tfloat4 reflection = {0};\n", hasReflection ? "TraceReflection(payload, normal)" : "0");
        stringBuilder.AppendFormat("\tfloat4 refraction = {0};\n", hasRefraction ? "TraceRefraction(payload)" : "0");
        stringBuilder.AppendFormat("\tfloat shadow = {0};\n", hasShadow ? "TraceShadow(payload.random)" : "1");

        WriteConstants(stringBuilder, "iaParams", vertexShader, shaderMapping);

        stringBuilder.AppendFormat("\n\t{0}_VSParams vsParams = ({0}_VSParams)0;\n", functionName);

        stringBuilder.AppendFormat("\t{0}_VS(iaParams, vsParams);\n\n", functionName);

        stringBuilder.AppendFormat("\t{0}_PSParams psParams = ({0}_PSParams)0;\n", functionName);

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

        stringBuilder.AppendFormat("\n\t{0}_OMParams omParams = ({0}_OMParams)0;\n", functionName);

        stringBuilder.AppendFormat("\t{0}_PS(psParams, omParams);\n\n", functionName);

        if (shaderType == "closesthit")
        {
            stringBuilder.Append("""
                payload.color = omParams.oC0; 
            }


            """);
        }
        else if (shaderType == "anyhit")
        {
            stringBuilder.Append(""" 
                if (mesh.w == 2)
                {
                    if (omParams.oC0.w < 0.5)
                        IgnoreHit();
                }
                else 
                {
                    if (omParams.oC0.w < NextRandom(payload.random))
                        IgnoreHit();
                }
            }


            """);
        }
    }

    public static void WriteAllFunctions(StringBuilder stringBuilder, string functionName, Shader vertexShader, Shader pixelShader, ShaderMapping shaderMapping)
    {
        stringBuilder.AppendFormat("//\n// {0}\n//\n\n", functionName);

        WriteShaderFunction(stringBuilder, functionName, vertexShader, shaderMapping);
        WriteShaderFunction(stringBuilder, functionName, pixelShader, shaderMapping);

        WriteRaytracingFunction(stringBuilder, functionName, "closesthit", vertexShader, pixelShader, shaderMapping);
        WriteRaytracingFunction(stringBuilder, functionName, "anyhit", vertexShader, pixelShader, shaderMapping);
    }

    public static StringBuilder BeginShaderConversion()
    {
        var stringBuilder = new StringBuilder();

        stringBuilder.Append("""
            #define _rep(x, y) for (int i##y = 0; i##y < x; ++i##y)
            #define rep(x) _rep(x, __LINE__)

            #include "ShaderLibrary.hlsli"


            """);

        return stringBuilder;
    }
}
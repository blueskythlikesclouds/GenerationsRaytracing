using GenerationsRaytracing.Tool.Shaders.Constants;
using GenerationsRaytracing.Tool.Shaders.Instructions;

namespace GenerationsRaytracing.Tool.Shaders;

public static class RaytracingShaderConverter
{
    private static void WriteConstant(StringBuilder stringBuilder, string outName, Constant constant, ShaderType shaderType, ShaderMapping shaderMapping)
    {
        if (constant.Name == "g_MtxProjection")
        {
            if (constant.Size >= 1) stringBuilder.AppendFormat("\t{0}.g_MtxProjection[0] = params.Projection[0];\n", outName);
            if (constant.Size >= 2) stringBuilder.AppendFormat("\t{0}.g_MtxProjection[1] = params.Projection[1];\n", outName);
            if (constant.Size >= 3) stringBuilder.AppendFormat("\t{0}.g_MtxProjection[2] = params.Projection[2];\n", outName);
            if (constant.Size >= 4) stringBuilder.AppendFormat("\t{0}.g_MtxProjection[3] = params.Projection[3];\n", outName);
        }
        else if (constant.Name == "g_MtxInvProjection")
        {
            if (constant.Size >= 1) stringBuilder.AppendFormat("\t{0}.g_MtxInvProjection[0] = params.InvProjection[0];\n", outName);
            if (constant.Size >= 2) stringBuilder.AppendFormat("\t{0}.g_MtxInvProjection[1] = params.InvProjection[1];\n", outName);
            if (constant.Size >= 3) stringBuilder.AppendFormat("\t{0}.g_MtxInvProjection[2] = params.InvProjection[2];\n", outName);
            if (constant.Size >= 4) stringBuilder.AppendFormat("\t{0}.g_MtxInvProjection[3] = params.InvProjection[3];\n", outName);
        }
        else if (constant.Name == "g_MtxView")
        {
            if (constant.Size >= 1) stringBuilder.AppendFormat("\t{0}.g_MtxView[0] = params.View[0];\n", outName);
            if (constant.Size >= 2) stringBuilder.AppendFormat("\t{0}.g_MtxView[1] = params.View[1];\n", outName);
            if (constant.Size >= 3) stringBuilder.AppendFormat("\t{0}.g_MtxView[2] = params.View[2];\n", outName);
            if (constant.Size >= 4) stringBuilder.AppendFormat("\t{0}.g_MtxView[3] = params.View[3];\n", outName);
        }
        else if (constant.Size == 4)
        {
            stringBuilder.AppendFormat("\t{0}.{1}[0] = float4(1, 0, 0, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[1] = float4(0, 1, 0, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[2] = float4(0, 0, 1, 0);\n", outName, constant.Name);
            stringBuilder.AppendFormat("\t{0}.{1}[3] = float4(0, 0, 0, 1);\n", outName, constant.Name);
        }
        else if (constant.Name == "g_aLightField")
        {
            for (int i = 0; i < constant.Size; i++)
                stringBuilder.AppendFormat("\t{0}.g_aLightField[{1}] = float4(params.GlobalIllumination, params.Shadow);\n", outName, i);
        }
        else if (constant.Name == "mrgTexcoordOffset" && shaderMapping.CanMapFloat4(constant.Register, shaderType))
        {
            stringBuilder.AppendFormat("\t{0}.mrgTexcoordOffset", outName);

            if (constant.Size >= 2)
                stringBuilder.Append("[0]");

            stringBuilder.AppendFormat(" = params.Material.Parameters[{0}];\n", shaderMapping.MapFloat4(constant.Register, shaderType));
        }
        else if (constant.Size <= 1)
        {
            stringBuilder.AppendFormat("\t{0}.{1} = ", outName, constant.Name);

            if (shaderMapping.CanMapFloat4(constant.Register, shaderType))
            {
                stringBuilder.AppendFormat("params.Material.Parameters[{0}]", shaderMapping.MapFloat4(constant.Register, shaderType));
            }
            else if (constant.Name.Contains("sampEnv") || constant.Name.Contains("sampRef"))
            {
                stringBuilder.Append("float4(params.Reflection, 1.0)");
            }
            else
            {
                switch (constant.Name)
                {
                    case "g_EyePosition":
                        stringBuilder.Append("float4(params.EyePosition, 0)");
                        break;

                    case "g_EyeDirection":
                        stringBuilder.Append("float4(params.EyeDirection, 0)");
                        break;

                    case "mrgGIAtlasParam":
                        stringBuilder.Append("float4(1, 1, 0, 0)");
                        break;

                    case "mrgHasBone":
                        stringBuilder.Append("false");
                        break;

                    case "g_ShadowMapSampler":
                    case "g_VerticalShadowMapSampler":
                    case "g_DepthSampler":
                        stringBuilder.Append("1");
                        break;

                    case "g_FramebufferSampler":
                        stringBuilder.Append("float4(params.Refraction, 1.0)");
                        break;

                    case "g_ReflectionMapSampler":
                    case "g_ReflectionMapSampler2":
                        stringBuilder.Append("float4(params.Reflection, 1.0)");
                        break;

                    case "mrgTexcoordOffset":
                    case "mrgTexcoordIndex":
                    case "g_ShadowMapParams":
                        stringBuilder.Append("0");
                        break;

                    default:
                        stringBuilder.Append(constant.Name);
                        break;
                }
            }

            stringBuilder.AppendLine(";");
        }
    }

    public static void WriteConstants(StringBuilder stringBuilder, string outName, Shader shader, ShaderMapping shaderMapping)
    {
        foreach (var constant in shader.Constants)
        {
            if (constant.Type != ConstantType.Sampler)
                WriteConstant(stringBuilder, outName, constant, shader.Type, shaderMapping);
        }
    }

    public static bool IsOutputColorClamp(Shader shader, Argument argument)
    {
        int register = int.Parse(argument.Token.AsSpan()[1..]);

        if (!shader.Definitions.TryGetValue(register, out string definition))
            return false;

        var split = definition.Split(',', StringSplitOptions.TrimEntries);
        string value;

        switch (argument.Swizzle.GetSwizzle(0))
        {
            case Swizzle.X: value = split[0]; break;
            case Swizzle.Y: value = split[1]; break;
            case Swizzle.Z: value = split[2]; break;
            case Swizzle.W: value = split[3]; break;
            default: return false;
        }

        return value == "4";
    }

    public static void WriteShaderFunction(StringBuilder stringBuilder, Shader shader)
    {
        string inName = shader.Type == ShaderType.Pixel ? "ps" : "ia";
        string inNameUpperCase = inName.ToUpperInvariant();

        var variableMap = new Dictionary<string, string>();
        var swizzleMap = new Dictionary<string, int>();

        stringBuilder.AppendFormat("struct {1}_{0}Params\n{{\n", inNameUpperCase, shader.Name);

        foreach (var (semantic, name) in shader.InSemantics)
        {
            stringBuilder.AppendFormat("\tfloat4 {0}; // {1}\n", name, semantic);
            variableMap.Add(name, $"{inName}Params.{name}");
        }

        foreach (var constant in shader.Constants)
        {
            char token = 'c';

            if (constant.Type == ConstantType.Sampler && (constant.Name.StartsWith("g_") || constant.Name.Contains("sampEnv") || constant.Name.Contains("sampRef")))
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
                    string type = shader.Samplers[token];
                    string name = constant.Name;

                    if (j > 0)
                        name += $"{j}";

                    stringBuilder.AppendFormat("\tTexture{0}<float4> {1};\n", type, name);

                    variableMap.Add(token, $"{inName}Params.{name}");
                    swizzleMap.Add(token, type == "2D" ? 2 : 3);
                }
            }
        }

        stringBuilder.AppendLine("};\n");

        if (shader.Type == ShaderType.Vertex)
        {
            stringBuilder.AppendFormat("struct {0}_VSParams\n{{\n", shader.Name);

            foreach (var (semantic, name) in shader.OutSemantics)
            {
                stringBuilder.AppendFormat("\tfloat4 {0}; // {1}\n", name, semantic);
                variableMap.Add(name, $"vsParams.{name}");
            }

            stringBuilder.AppendLine("};\n");

            stringBuilder.AppendFormat("void {0}_VS(inout {0}_IAParams iaParams, inout {0}_VSParams vsParams)\n{{\n", shader.Name);
        }
        else
        {
            variableMap.Add("oC0", "omParams.oC0");
            variableMap.Add("oC1", "omParams.oC1");
            variableMap.Add("oC2", "omParams.oC2");
            variableMap.Add("oC3", "omParams.oC3");
            variableMap.Add("oDepth", "omParams.oDepth");

            stringBuilder.AppendFormat("void {0}_PS(inout {0}_PSParams psParams, inout OMParams omParams, inout float3 normal)\n{{\n", shader.Name);
        }

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
        bool seenDotProductWithGlobalLight = false;

        foreach (var instruction in shader.Instructions)
        {
            if (instruction.Arguments != null)
            {
                if (shader.Type == ShaderType.Pixel && instruction.OpCode == "min")
                {
                    if (IsOutputColorClamp(shader, instruction.Arguments[1]))
                    {
                        instruction.OpCode = "mov";
                        instruction.Arguments[1] = instruction.Arguments[2];
                    }
                    else if (IsOutputColorClamp(shader, instruction.Arguments[2]))
                    {
                        instruction.OpCode = "mov";
                        instruction.Arguments[2] = instruction.Arguments[1];
                    }
                }

                if (shader.Type == ShaderType.Pixel && instruction.OpCode.StartsWith("texld") &&
                    swizzleMap.TryGetValue(instruction.Arguments[2].Token, out int swizzleSize))
                    instruction.Arguments[1].Swizzle.Resize(swizzleSize);

                foreach (var argument in instruction.Arguments)
                {
                    if (variableMap.TryGetValue(argument.Token, out string constantName))
                        argument.Token = constantName;

                    else if (argument.Token[0] == 'c')
                        argument.Token = $"C[{argument.Token.Substring(1)}]";
                }

                if (shader.Type == ShaderType.Pixel && instruction.OpCode == "dp3")
                {
                    var globalLightArgument = instruction.Arguments.FirstOrDefault(x => x.Token.Contains("mrgGlobalLight_Direction"));

                    if (globalLightArgument != null && !seenDotProductWithGlobalLight)
                    {
                        stringBuilder.AppendFormat("\tnormal = ({0}).xyz;\n",
                            instruction.Arguments[1] == globalLightArgument
                                ? instruction.Arguments[2]
                                : instruction.Arguments[1]);

                        seenDotProductWithGlobalLight = true;
                    }
                }

                if (shader.Type == ShaderType.Pixel &&
                    instruction.Arguments.Length == 3 &&
                    ((instruction.Arguments[2].Token.Contains(".g_") && instruction.Arguments[2].Token.Contains("Sampler")) || 
                     instruction.Arguments[2].Token.Contains("sampEnv") || instruction.Arguments[2].Token.Contains("sampRef")))
                {
                    instruction.OpCode = "mov";
                    instruction.Arguments[1]  = instruction.Arguments[2];
                }
            }

            string instrLine = instruction.Convert(true);

            if (instrLine.Contains('}')) --indent;

            for (int j = 0; j < indent; j++)
                stringBuilder.Append("\t");

            instrLine = instrLine.Replace("][", " + ");

            stringBuilder.AppendFormat("{0}\n", instrLine);

            if (instrLine.Contains('{')) ++indent;
        }

        stringBuilder.AppendLine("}\n");
    }

    public static void WriteRaytracingFunction(StringBuilder stringBuilder, string shaderName, Shader vertexShader, Shader pixelShader, ShaderMapping shaderMapping)
    {
        stringBuilder.AppendFormat("OMParams {0}(in ShaderParams params, inout float3 normal)\n{{\n", shaderName);

        stringBuilder.AppendFormat("\t{0}_IAParams iaParams = ({0}_IAParams)0;\n", vertexShader.Name);

        foreach (var (semantic, name) in vertexShader.InSemantics)
        {
            switch (semantic)
            {
                case "POSITION":
                    stringBuilder.AppendFormat("\tiaParams.{0} = float4(params.Vertex.Position, 1.0);\n", name);
                    break;

                case "NORMAL":
                    stringBuilder.AppendFormat("\tiaParams.{0} = float4(params.Vertex.Normal, 0.0);\n", name);
                    break;

                case "TANGENT":
                    stringBuilder.AppendFormat("\tiaParams.{0} = float4(params.Vertex.Tangent, 0.0);\n", name);
                    break;

                case "BINORMAL":
                    stringBuilder.AppendFormat("\tiaParams.{0} = float4(params.Vertex.Binormal, 0.0);\n", name);
                    break;

                case "TEXCOORD":
                case "TEXCOORD0":
                case "TEXCOORD1":
                case "TEXCOORD2":
                case "TEXCOORD3":
                    stringBuilder.AppendFormat("\tiaParams.{0} = float4(params.Vertex.TexCoord, 0.0, 0.0);\n", name);
                    break;

                case "COLOR":
                    stringBuilder.AppendFormat("\tiaParams.{0} = params.Vertex.Color;\n", name);
                    break;
            }
        }

        WriteConstants(stringBuilder, "iaParams", vertexShader, shaderMapping);

        stringBuilder.AppendFormat("\n\t{0}_VSParams vsParams = ({0}_VSParams)0;\n", vertexShader.Name);

        stringBuilder.AppendFormat("\t{0}_VS(iaParams, vsParams);\n\n", vertexShader.Name);

        stringBuilder.AppendFormat("\t{0}_PSParams psParams = ({0}_PSParams)0;\n", pixelShader.Name);

        foreach (var (semantic, name) in pixelShader.InSemantics)
        {
            if (name == "vPos")
                stringBuilder.AppendFormat("\tpsParams.{0}.xy = (float2)DispatchRaysIndex().xy;\n", name);

            else if (vertexShader.OutSemantics.TryGetValue(semantic, out string targetName))
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

                stringBuilder.AppendFormat("\tpsParams.{0} = g_BindlessTexture{1}[NonUniformResourceIndex(params.Material.TextureIndices[{2}])];\n",
                    name,
                    pixelShader.Samplers[token],
                    shaderMapping.CanMapSampler(register) ? shaderMapping.MapSampler(register) : 0);
            }
        }

        stringBuilder.AppendLine("\n\tOMParams omParams = (OMParams)0;");

        stringBuilder.AppendFormat("\t{0}_PS(psParams, omParams, normal);\n\treturn omParams;\n}}\n\n", pixelShader.Name);
    }

    public static void WriteRaytracingFunctions(StringBuilder stringBuilder, string shaderName, Shader vertexShader, Shader pixelShader, ShaderMapping shaderMapping)
    {
        WriteRaytracingFunction(stringBuilder, shaderName, vertexShader, pixelShader, shaderMapping);

        bool hasGlobalIllumination = vertexShader.Constants.Any(x => x.Name == "g_aLightField") ||
                                     pixelShader.Constants.Any(x => x.Name == "g_aLightField");

        bool hasReflection = pixelShader.Constants.Any(x =>
            x.Name.StartsWith("g_ReflectionMap") ||
            x.Name.Contains("sampEnv") ||
            x.Name.Contains("sampRef"));

        bool hasRefraction = pixelShader.Constants.Any(x => x.Name == "g_FramebufferSampler");

        //
        // Closest hit
        //
        stringBuilder.AppendLine("[shader(\"closesthit\")]");
        stringBuilder.AppendFormat("void {0}_closesthit(inout Payload payload : SV_RayPayload, Attributes attributes : SV_IntersectionAttributes)\n{{\n", shaderName);

        stringBuilder.AppendLine("\tShaderParams params = (ShaderParams)0;");
        stringBuilder.AppendLine("\tparams.Vertex = GetVertex(attributes);");
        stringBuilder.AppendLine("\tparams.Material = GetMaterial();\n");

        stringBuilder.Append("""
                params.Projection[0] = payload.Depth > 0 ? float4(1, 0, 0, 0) : g_MtxProjection[0];
                params.Projection[1] = payload.Depth > 0 ? float4(0, 1, 0, 0) : g_MtxProjection[1];
                params.Projection[2] = payload.Depth > 0 ? float4(0, 0, 0, 0) : g_MtxProjection[2];
                params.Projection[3] = payload.Depth > 0 ? float4(0, 0, 0, 1) : g_MtxProjection[3];
                params.InvProjection[0] = payload.Depth > 0 ? float4(1, 0, 0, 0) : g_MtxInvProjection[0];
                params.InvProjection[1] = payload.Depth > 0 ? float4(0, 1, 0, 0) : g_MtxInvProjection[1];
                params.InvProjection[2] = payload.Depth > 0 ? float4(0, 0, 0, 0) : g_MtxInvProjection[2];
                params.InvProjection[3] = payload.Depth > 0 ? float4(0, 0, -Z_MAX, 1) : g_MtxInvProjection[3];
                params.View[0] = payload.Depth > 0 ? float4(1, 0, 0, 0) : g_MtxView[0];
                params.View[1] = payload.Depth > 0 ? float4(0, 1, 0, 0) : g_MtxView[1];
                params.View[2] = payload.Depth > 0 ? float4(0, 0, 0, 0) : g_MtxView[2];
                params.View[3] = payload.Depth > 0 ? float4(0, 0, -RayTCurrent(), 1) : g_MtxView[3];
                params.EyePosition = payload.Depth > 0 ? WorldRayOrigin() : g_EyePosition.xyz;
                params.EyeDirection = payload.Depth > 0 ? WorldRayDirection() : g_EyeDirection.xyz;


            """);

        stringBuilder.AppendLine("\tfloat3 normal = params.Vertex.Normal;");
        stringBuilder.AppendFormat("\t{0}(params, normal);\n", shaderName);

        if (hasGlobalIllumination)
        {
            stringBuilder.AppendLine("\n\tparams.GlobalIllumination = TraceGlobalIlluminationWithReProjection(payload, params.Vertex.Position, normal);");
            stringBuilder.AppendLine("\tparams.Shadow = dot(normal, -mrgGlobalLight_Direction.xyz) > 0.0 ? TraceShadow(payload.Random) : 0.0;");
        }

        if (hasReflection)
            stringBuilder.AppendLine("\n\tparams.Reflection = TraceReflection(payload, normal);");  
        
        if (hasRefraction)
            stringBuilder.AppendLine("\n\tparams.Refraction = TraceRefraction(payload, normal);");

        stringBuilder.AppendFormat("\n\tpayload.Color = {0}(params, normal).oC0.rgb;\n", shaderName);
        stringBuilder.AppendLine("\tpayload.T = RayTCurrent();");
        stringBuilder.AppendLine("}\n");

        // 
        // Any hit
        //
        stringBuilder.AppendLine("[shader(\"anyhit\")]");
        stringBuilder.AppendFormat("void {0}_anyhit(inout Payload payload : SV_RayPayload, Attributes attributes : SV_IntersectionAttributes)\n{{\n", shaderName);

        stringBuilder.AppendLine("\tShaderParams params = (ShaderParams)0;");
        stringBuilder.AppendLine("\tparams.Vertex = GetVertex(attributes);");
        stringBuilder.AppendLine("\tparams.Material = GetMaterial();");

        stringBuilder.AppendFormat("\tOMParams omParams = {0}(params, params.Vertex.Normal);\n\n", shaderName);

        stringBuilder.Append("""   
                if (omParams.oC0.w < (GetGeometry().PunchThrough != 0 ? 0.5 : NextRandom(payload.Random)))
                    IgnoreHit();
            }


            """);
    }

    public static StringBuilder BeginShaderConversion()
    {
        var stringBuilder = new StringBuilder();

        stringBuilder.Append("""
            #define _rep(x, y) for (int i##y = 0; i##y < x; ++i##y)
            #define rep(x) _rep(x, __LINE__)

            #include "ShaderLibrary.hlsli"

            struct OMParams
            {
                float4 oC0;
                float4 oC1;
                float4 oC2;
                float4 oC3;
                float4 oDepth;
            };


            """);

        return stringBuilder;
    }
}
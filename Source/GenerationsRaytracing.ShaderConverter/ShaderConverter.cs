using GenerationsRaytracing.ShaderConverter.Constants;
using GenerationsRaytracing.ShaderConverter.Instructions;
using SharpGen.Runtime;

namespace GenerationsRaytracing.ShaderConverter;

public class ShaderConverter
{
    private static void PopulateSemantics(Dictionary<string, string> semantics)
    {
        semantics.Add("SV_Position", "svPos");

        semantics.Add("TEXCOORD", "texCoord");
        for (int i = 1; i < 16; i++)
            semantics.Add($"TEXCOORD{i}", $"texCoord{i}");

        semantics.Add("COLOR", "color");
        semantics.Add("COLOR1", "color1");
    }

    public static unsafe uint GetHashCode(void* data, int length)
    {
        uint hash = 2166136261;
        for (int i = 0; i < length; i++)
            hash = (hash ^ ((byte*)data)[i]) * 16777619;

        hash += hash << 13;
        hash ^= hash >> 7;
        hash += hash << 3;
        hash ^= hash >> 17;
        hash += hash << 5;

        return hash;
    }

    public static unsafe bool IsCsdShader(void* data, int dataSize)
    {
        uint hash;

        return (dataSize == 544 || dataSize == 508 || dataSize == 712 || dataSize == 676) &&
               ((hash = GetHashCode(data, dataSize)) == 1675967623u || hash == 1353058734u ||
                hash == 2754048365u || hash == 4044681422u || hash == 3025790305u || hash == 2388726924u ||
                hash == 602606931u || hash == 781526840u);
    }

    public static unsafe string Convert(Span<byte> data, out bool isPixelShader)
    {
        fixed (byte* ptr = data)
            return Convert(ptr, data.Length, out isPixelShader);
    }

    public static unsafe byte[] Convert(Span<byte> data)
    {
        fixed (byte* ptr = data)
            return Convert(ptr, data.Length);
    }

    public static unsafe string Convert(void* data, int dataSize, out bool isPixelShader)
    {
	    string disassembly = Compiler.Disassemble(new IntPtr(data), new PointerSize(dataSize),
		    DisasmFlags.DisableDebugInfo | DisasmFlags.InstructionOnly, null).AsString();

        int i;

        var lines = disassembly.Split(new[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
        for (i = 0; i < lines.Length; i++)
            lines[i] = lines[i].Trim();

        isPixelShader = false;

        var inSemantics = new Dictionary<string, string>();
        var outSemantics = new Dictionary<string, string>();
        var samplers = new Dictionary<string, string>();

        var definitions = new Dictionary<int, string>();
        var definitionsInt = new Dictionary<int, string>();

        for (i = 0; i < lines.Length; i++)
        {
            string line = lines[i];

            if (line.StartsWith("//"))
                continue;

            if (line.StartsWith("vs_"))
                PopulateSemantics(outSemantics);

            else if (line.StartsWith("ps_"))
            {
                PopulateSemantics(inSemantics);
                isPixelShader = true;
            }

            else if (line.StartsWith("defi"))
            {
                int firstSeparatorIndex = line.IndexOf(',');
                definitionsInt.Add(int.Parse(line.Substring(6, firstSeparatorIndex - 6)), line.Substring(firstSeparatorIndex + 2));
            }

            else if (line.StartsWith("def"))
            {
                int firstSeparatorIndex = line.IndexOf(',');
                string value = line.Substring(firstSeparatorIndex + 2);

                // Special case: Debug/stereo shaders take dot product of the sampled
                // depth value using this vector. This is not going to work due to
                // depth reads in DX11 loading only into the X component, unlike RAWZ/INTZ.
                // We fix this by forcing the dot product to return the X value.
                value = value.Replace("0.00389099144, 1.51991853e-005, 0.99609381", "1, 0, 0");

                definitions.Add(int.Parse(line.Substring(5, firstSeparatorIndex - 5)), value);
            }

            else if (line.StartsWith("dcl"))
            {
                int underscoreIndex = line.IndexOf('_');
                int spaceIndex = line.IndexOf(' ');
                int dotIndex = line.IndexOf('.');

                int separatorIndex = line.IndexOf('_', underscoreIndex + 1);
                if (separatorIndex == -1)
                    separatorIndex = spaceIndex;

                string semantic = underscoreIndex == -1
                    ? "SV_Position"
                    : line.Substring(underscoreIndex + 1, separatorIndex - underscoreIndex - 1).ToUpperInvariant();

                string token;

                if (dotIndex == -1)
                    token = line.Substring(spaceIndex + 1);
                else
                    token = line.Substring(spaceIndex + 1, dotIndex - spaceIndex - 1);

                switch (semantic)
                {
                    case "POSITION" when !isPixelShader && token[0] == 'o':
                        semantic = "SV_Position";
                        break;

                    case "CUBE":
                        semantic = "Cube";
                        break;

                    case "VOLUME":
                        semantic = "3D";
                        break;
                }

                switch (token[0])
                {
                    case 'v':
                        inSemantics[semantic] = token;
                        break;

                    case 'o':
                        outSemantics[semantic] = token;
                        break;

                    case 's':
                        samplers.Add(token, semantic);
                        break;
                }
            }

            else
                break;
        }

        var instructions = new List<Instruction>();
        for (; i < lines.Length; i++)
        {
            if (!lines[i].StartsWith("//"))
                instructions.Add(new Instruction(lines[i]));
        }

        var constants = ConstantParser.Parse(data, dataSize);

        // Auto-populate a constant table if nothing was found in the file.
        if (constants == null)
        {
            constants = new List<Constant>();

            int float4Size = isPixelShader ? 224 : 256;

            for (int j = 0; j < float4Size;)
            {
                int k;
                for (k = j; k < float4Size; k++)
                {
                    if (definitions.ContainsKey(k))
                        break;
                }

                int size = k - j;
                if (size > 0)
                {
                    constants.Add(new Constant
                    {
                        Name = $"g_C{j}",
                        Register = j,
                        Size = size,
                        Type = ConstantType.Float4
                    });
                }

                j = k + 1;
            }

            const int boolSize = 16;

            for (int j = 0; j < boolSize; j++)
            {
                constants.Add(new Constant
                {
                    Name = $"g_B{j}",
                    Register = j,
                    Size = 1,
                    Type = ConstantType.Bool
                });
            }

            foreach (var sampler in samplers)
            {
                string register = sampler.Key.Substring(1);

                constants.Add(new Constant
                {
                    Name = $"g_S{register}",
                    Register = int.Parse(register),
                    Size = 1,
                    Type = ConstantType.Sampler
                });
            }
        }

        constants = constants.OrderBy(x => x.Type).ThenBy(x => x.Register).ToList();

        var constantMap = new Dictionary<string, string>();
        var stringBuilder = new StringBuilder();

        stringBuilder.AppendLine("#define _rep(x, y) for (int i##y = 0; i##y < x; ++i##y)");
        stringBuilder.AppendLine("#define rep(x) _rep(x, __LINE__)\n");

        stringBuilder.AppendLine("#define FLT_MAX asfloat(0x7f7fffff)\n");

        stringBuilder.AppendFormat("cbuffer cbGlobals{0} : register(b{1}) {{\n", isPixelShader ? "PS" : "VS", isPixelShader ? 1 : 0);

        foreach (var constant in constants)
        {
            switch (constant.Type)
            {
                case ConstantType.Float4:
                    stringBuilder.AppendFormat("\tfloat4 {0}", constant.Name);

                    if (constant.Size > 1)
                    {
                        stringBuilder.AppendFormat("[{0}]", constant.Size);

                        for (int j = 0; j < constant.Size; j++)
                            constantMap.Add($"c{(constant.Register + j)}", $"{constant.Name}[{j}]");
                    }

                    else
                        constantMap.Add($"c{constant.Register}", constant.Name);

                    stringBuilder.AppendFormat(" : packoffset(c{0});\n", constant.Register);
                    break;

                case ConstantType.Bool:
                    stringBuilder.AppendFormat("#define {0} (1 << {1})\n", constant.Name, constant.Register);
                    constantMap.Add($"b{constant.Register}", $"g_Booleans & {constant.Name}");
                    break;

                case ConstantType.Sampler:
                    int index = constant.Register / 4;
	                char swizzle = SwizzleSet.SwizzleChars[constant.Register % 4];

	                stringBuilder.AppendFormat("\tuint {0}_tI : packoffset(c{1}.{2});\n", constant.Name, 224 + index, swizzle);
	                stringBuilder.AppendFormat("\tuint {0}_sI : packoffset(c{1}.{2});\n", constant.Name, 228 + index, swizzle);
	                break;
            }
        }

        if (isPixelShader)
        {
            stringBuilder.AppendLine("\tbool g_EnableAlphaTest : packoffset(c232.x);");
            stringBuilder.AppendLine("\tfloat g_AlphaThreshold : packoffset(c232.y);");
            stringBuilder.AppendLine("\tuint g_Booleans : packoffset(c232.z);");
            stringBuilder.AppendLine("\tuint g_ExposureTextureId : packoffset(c232.w);");
        }
        else
        {
            stringBuilder.AppendLine("\tuint g_Booleans : packoffset(c196.x);");
            stringBuilder.AppendLine("\tbool g_Enable10BitNormal : packoffset(c196.y);");
        }

        stringBuilder.AppendLine("}\n");

        stringBuilder.AppendLine("void main(");

        bool isMetaInstancer = !isPixelShader && constants.Any(x => x.Name == "g_InstanceTypes");
        var varsToCheck10BitNormal = new HashSet<string>();

        foreach (var semantic in inSemantics)
        {
            string type = "float";

            if (semantic.Key == "BLENDINDICES" || (isMetaInstancer && semantic.Key == "TEXCOORD2"))
                type = "uint";

            if (semantic.Key == "NORMAL" || semantic.Key == "TANGENT" || semantic.Key == "BINORMAL")
                varsToCheck10BitNormal.Add(semantic.Value);

            stringBuilder.AppendFormat("\tin {0}4 {1} : {2},\n", type, semantic.Value, semantic.Key);
        }

        if (!isPixelShader)
        {
            int count = 0;

            foreach (var semantic in outSemantics)
            {
                stringBuilder.AppendFormat("\tout float4 {0} : {1}", semantic.Value, semantic.Key);

                if ((++count) < outSemantics.Count)
                    stringBuilder.AppendFormat(",\n");
            }
        }
        else
        {
            if (disassembly.Contains("oDepth"))
                stringBuilder.AppendLine("\tout float oDepth : SV_Depth,");

            stringBuilder.Append("\tout float4 oC0 : SV_Target0");
            stringBuilder.Append(",\n\tout float4 oC1 : SV_Target1");

            if (disassembly.Contains("oC2"))
                stringBuilder.Append(",\n\tout float4 oC2 : SV_Target2");

            if (disassembly.Contains("oC3"))
                stringBuilder.Append(",\n\tout float4 oC3 : SV_Target3");
        }

        stringBuilder.AppendLine(")\n{");

        if (samplers.Count > 0)
        {
	        foreach (var constant in constants)
	        {
		        if (constant.Type != ConstantType.Sampler)
			        continue;

		        string token = $"s{constant.Register}";
		        string name = constant.Name;

		        stringBuilder.AppendFormat(
			        "\tTexture{0}<float4> {1} = ResourceDescriptorHeap[{1}_tI];\n", samplers[token], name);

		        string type = "SamplerState";

		        if (instructions.Any(x => x.OpCode == "texldp" && x.Arguments[2].Token == token))
			        type = "SamplerComparisonState";

		        stringBuilder.AppendFormat("\t{0} {1}_s = SamplerDescriptorHeap[{1}_sI];\n\n", type, name);
		        constantMap.Add(token, name);
	        }
        }

		if (definitions.Count > 0)
        {
            stringBuilder.AppendFormat("\tfloat4 C[{0}];\n\n", definitions.Max(x => x.Key) + 1);

            foreach (var definition in definitions)
                stringBuilder.AppendFormat("\tC[{0}] = float4({1});\n", definition.Key, definition.Value);

            stringBuilder.AppendLine();
        }

        if (definitionsInt.Count > 0)
        {
            foreach (var definition in definitionsInt)
                stringBuilder.AppendFormat("\tint4 i{0} = int4({1});\n", definition.Key, definition.Value);

            stringBuilder.AppendLine();
        }

        for (int j = 0; j < 32; j++)
            stringBuilder.AppendFormat("\tfloat4 r{0} = float4(0, 0, 0, 0);\n", j);

        if (!isPixelShader)
            stringBuilder.AppendLine("\tuint4 a0 = uint4(0, 0, 0, 0);");

        stringBuilder.AppendLine();

        int indent = 1;

        foreach (var instruction in instructions)
        {
            if (instruction.Arguments != null)
            {
                foreach (var argument in instruction.Arguments)
                {
                    if (argument.Token == "vPos")
                        argument.Token = $"({argument.Token} - float4(0.5, 0.5, 0.0, 0.0))";

                    else if (constantMap.TryGetValue(argument.Token, out string constantName))
                        argument.Token = constantName;

                    else if (argument.Token[0] == 'c')
                        argument.Token = $"C[{argument.Token.Substring(1)}]";

                    else if (varsToCheck10BitNormal.Contains(argument.Token))
                        argument.Token = $"(g_Enable10BitNormal ? {argument.Token} * 2.0 - 1.0 : {argument.Token})";
                }
            }

            string instrLine = instruction.ToString();

            if (instrLine.Contains('}')) --indent;

            for (int j = 0; j < indent; j++)
                stringBuilder.Append("\t");

            instrLine = instrLine.Replace("][", " + ");

            if (instrLine.Contains("g_MtxPalette"))
                instrLine = instrLine.Replace("[", "[min(74, ").Replace("]", ")]");

            stringBuilder.AppendFormat("{0}\n", instrLine);

            if (instrLine.Contains('{')) ++indent;
        }

        if (isPixelShader)
        {
            stringBuilder.AppendLine("\n\tif (g_EnableAlphaTest) {");
            stringBuilder.AppendLine("\t\tclip(oC0.w - g_AlphaThreshold);");
            stringBuilder.AppendLine("\t}");

            if (!disassembly.Contains("oC1"))
                stringBuilder.AppendLine("\toC1 = oC0;");

            stringBuilder.AppendLine("\n\tif (g_ExposureTextureId != 0) {");
            stringBuilder.AppendLine("\t\tTexture2D<float> exposure = ResourceDescriptorHeap[g_ExposureTextureId];");
            stringBuilder.AppendLine("\t\toC0.rgb /= max(0.0001, exposure[uint2(0, 0)] * 0.5);");
            stringBuilder.AppendLine("\t}");
        }

        // Prevent half-pixel correction in CSD shaders
        else if (IsCsdShader(data, dataSize))
            stringBuilder.AppendLine("\to0.xy += float2(g_ViewportSize.z, -g_ViewportSize.w) * o0.w;");

        stringBuilder.AppendLine("}");

        return stringBuilder.ToString();
    }

    public static unsafe byte[] Convert(void* data, int dataSize)
    {
        return Compile(Convert(data, dataSize, out bool isPixelShader), isPixelShader);
    }

    public static byte[] Compile(string convertedShader, bool isPixelShader)
    {
		var result = DxcCompiler.Compile(convertedShader, 
			new[] { isPixelShader ? "-T ps_6_6" : "-T vs_6_6", "-HV 2021", "-all-resources-bound" });

        return result.GetObjectBytecode().IsEmpty
            ? throw new Exception(result.GetErrors())
            : result.GetObjectBytecodeArray();
    }
}
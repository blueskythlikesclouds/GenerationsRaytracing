using GenerationsRaytracing.Tool.Shaders.Constants;
using GenerationsRaytracing.Tool.Shaders.Instructions;

namespace GenerationsRaytracing.Tool.Shaders;

public static unsafe class ShaderParser
{
    public static Shader Parse(byte[] function)
    {
        fixed (byte* ptr = function)
            return Parse(ptr, function.Length);
    }

    public static Shader Parse(void* function, int functionSize)
    {
        var shader = new Shader();

        string disassembly;
        {
            ID3DBlob blob;
            Compiler.Disassemble(function, functionSize, 0x50, null, out blob);
            disassembly = Marshal.PtrToStringAnsi(blob.GetBufferPointer());
        }

        int i;

        var lines = disassembly.Split(new[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
        for (i = 0; i < lines.Length; i++)
            lines[i] = lines[i].Trim();

        for (i = 0; i < lines.Length; i++)
        {
            string line = lines[i];

            if (line.StartsWith("//"))
                continue;

            if (line.StartsWith("vs_"))
                shader.Type = ShaderType.Vertex;

            else if (line.StartsWith("ps_"))
                shader.Type = ShaderType.Pixel;

            else if (line.StartsWith("defi"))
            {
                int firstSeparatorIndex = line.IndexOf(',');

                shader.DefinitionsInt.Add(int.Parse(line.Substring(6, firstSeparatorIndex - 6)),
                    line.Substring(firstSeparatorIndex + 2));
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

                shader.Definitions.Add(int.Parse(line.Substring(5, firstSeparatorIndex - 5)), value);
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
                        shader.InSemantics[semantic] = token;
                        break;

                    case 'o':
                        shader.OutSemantics[semantic] = token;
                        break;

                    case 's':
                        shader.Samplers.Add(token, semantic);
                        break;
                }
            }

            else
                break;
        }

        for (; i < lines.Length; i++)
        {
            if (!lines[i].StartsWith("//"))
                shader.Instructions.Add(new Instruction(lines[i]));
        }

        shader.Constants = ConstantParser.Parse(function, functionSize);

        // Auto-populate a constant table if nothing was found in the file.
        if (shader.Constants == null)
        {
            shader.Constants = new List<Constant>();

            int float4Size = shader.Type == ShaderType.Pixel ? 224 : 256;

            for (int j = 0; j < float4Size;)
            {
                int k;
                for (k = j; k < float4Size; k++)
                {
                    if (shader.Definitions.ContainsKey(k))
                        break;
                }

                int size = k - j;
                if (size > 0)
                {
                    shader.Constants.Add(new Constant
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
                shader.Constants.Add(new Constant
                {
                    Name = $"g_B{j}",
                    Register = j,
                    Size = 1,
                    Type = ConstantType.Bool
                });
            }

            foreach (var sampler in shader.Samplers)
            {
                string register = sampler.Key.Substring(1);

                shader.Constants.Add(new Constant
                {
                    Name = $"g_S{register}",
                    Register = int.Parse(register),
                    Size = 1,
                    Type = ConstantType.Sampler
                });
            }
        }

        shader.Constants.Sort((x, y) => x.Register.CompareTo(y.Register));

        return shader;
    }
}
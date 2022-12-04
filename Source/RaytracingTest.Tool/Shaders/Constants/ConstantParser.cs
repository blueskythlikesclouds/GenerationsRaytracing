namespace RaytracingTest.Tool.Shaders.Constants;

public static class ConstantParser
{
    public static unsafe List<Constant> Parse(byte[] function)
    {
        fixed (byte* ptr = function)
            return Parse(ptr, function.Length);
    }

    public static unsafe List<Constant> Parse(void* function, int functionSize)
    {
        int* instr = (int*)function;
        byte* end = (byte*)function + functionSize;

        while (instr < end)
        {
            if (*instr++ != 0x42415443)
                continue;

            var constants = new List<Constant>();

            byte* ctab = (byte*)instr;

            int constantCount = *(int*)(ctab + 12);
            int constantsOffset = *(int*)(ctab + 16);

            var stringBuilder = new StringBuilder();

            for (int i = 0; i < constantCount; i++)
            {
                byte* constantOffset = ctab + constantsOffset + i * 20;
                byte* nameOffset = ctab + *(int*)constantOffset;

                stringBuilder.Clear();
                for (int j = 0; nameOffset[j] != 0; j++)
                    stringBuilder.Append((char)nameOffset[j]);

                var constant = new Constant
                {
                    Name = stringBuilder.ToString(),
                    Register = *(short*)(constantOffset + 6),
                    Size = *(short*)(constantOffset + 8),
                };

                switch (*(short*)(ctab + *(int*)(constantOffset + 12) + 2))
                {
                    case 1:
                        constant.Type = ConstantType.Bool;
                        break;

                    case 2:
                        constant.Type = ConstantType.Int4;
                        break;

                    case 3:
                        constant.Type = ConstantType.Float4;
                        break;

                    default:
                        constant.Type = ConstantType.Sampler;
                        break;
                }

                constants.Add(constant);
            }

            return constants;
        }

        return null;
    }
}
namespace GenerationsRaytracing.Tool.Mirage;

public class ShaderData : IBinarySerializable
{
    public string CodeName { get; set; }
    public List<string> ParameterNames { get; }

    public void Read(BinaryObjectReader reader)
    {
        reader.ReadOffset(() => CodeName = reader.ReadString(StringBinaryFormat.NullTerminated));

        int count = reader.ReadInt32();
        reader.ReadOffset(() =>
        {
            for (int i = 0; i < count; i++)
            {
                string value = string.Empty;
                reader.ReadOffset(() => value = reader.ReadString(StringBinaryFormat.NullTerminated));
                ParameterNames.Add(value);
            }
        });
    }

    public void Write(BinaryObjectWriter writer)
    {
        writer.WriteStringOffset(StringBinaryFormat.NullTerminated, CodeName, -1, 1);
        writer.Write(ParameterNames.Count);
        writer.WriteOffset(() =>
        {
            foreach (string parameterFileName in ParameterNames)
                writer.WriteStringOffset(StringBinaryFormat.NullTerminated, parameterFileName, -1, 1);
        }, 4);
    }

    public ShaderData()
    {
        ParameterNames = new List<string>();
    }
}
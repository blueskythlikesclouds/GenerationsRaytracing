namespace RaytracingTest.Tool.Mirage;

[Flags]
public enum VertexShaderSubPermutations
{
    None = 1 << 0,
    ConstTexCoord = 1 << 1,
    All = 0x3
}

public class VertexShaderPermutationData : IBinarySerializable
{
    public VertexShaderSubPermutations SubPermutations { get; set; }
    public string Name { get; set; }
    public string ShaderName { get; set; }

    public void Read(BinaryObjectReader reader)
    {
        SubPermutations = (VertexShaderSubPermutations)reader.ReadUInt32();
        reader.ReadOffset(() => Name = reader.ReadString(StringBinaryFormat.NullTerminated));
        reader.ReadOffset(() => ShaderName = reader.ReadString(StringBinaryFormat.NullTerminated));
    }

    public void Write(BinaryObjectWriter writer)
    {
        writer.Write((uint)SubPermutations);
        writer.WriteStringOffset(StringBinaryFormat.NullTerminated, Name, -1, 1);
        writer.WriteStringOffset(StringBinaryFormat.NullTerminated, ShaderName, -1, 1);
    }

    public VertexShaderPermutationData(VertexShaderSubPermutations subPermutations, string name, string shaderName)
    {
        SubPermutations = subPermutations;
        Name = name;
        ShaderName = shaderName;
    }

    public VertexShaderPermutationData()
    {
    }
}
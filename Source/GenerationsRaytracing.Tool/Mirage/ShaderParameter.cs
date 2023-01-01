namespace GenerationsRaytracing.Tool.Mirage;

public class ShaderParameter : IBinarySerializable
{
    public string Name { get; set; }
    public byte Index { get; set; }
    public byte Size { get; set; }

    public void Read(BinaryObjectReader reader)
    {
        reader.ReadOffset(() => Name = reader.ReadString(StringBinaryFormat.NullTerminated));
        Index = reader.ReadByte();
        Size = reader.ReadByte();
    }

    public void Write(BinaryObjectWriter writer)
    {
        writer.WriteStringOffset(StringBinaryFormat.NullTerminated, Name, -1, 1);
        writer.Write(Index);
        writer.Write(Size);
        writer.Align(4);
    }

    public ShaderParameter()
    {
    }

    public ShaderParameter(string name, byte index, byte size)
    {
        Name = name;
        Index = index;
        Size = size;
    }

    public ShaderParameter(string name, byte index) : this(name, index, 1)
    {
    }
}
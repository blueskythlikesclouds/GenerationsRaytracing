namespace GenerationsRaytracing.Tool.IO.Extensions;

public static class BinarySerializableEx
{
    public static void Load(this IBinarySerializable binarySerializable, BinaryObjectReader reader)
    {
        reader.Seek(12, SeekOrigin.Begin);
        reader.ReadOffset(() =>
        {
            using var token = reader.WithOffsetOrigin();
            reader.ReadObject(binarySerializable);
        });
    }

    public static void Load(this IBinarySerializable binarySerializable, Stream stream)
    {
        using var reader = new BinaryObjectReader(stream, StreamOwnership.Retain, Endianness.Big, Encoding.UTF8);
        binarySerializable.Load(reader);
    }

    public static void Load(this IBinarySerializable binarySerializable, string filePath)
    {
        using var stream = File.OpenRead(filePath);
        binarySerializable.Load(stream);
    }

    public static void Load(this IBinarySerializable binarySerializable, byte[] bytes)
    {
        using var stream = new MemoryStream(bytes);
        binarySerializable.Load(stream);
    }

    public static T Load<T>(params string[] filePaths) where T : IBinarySerializable, new()
    {
        var obj = new T();
        foreach (string filePath in filePaths)
            obj.Load(filePath);
        return obj;
    }
}
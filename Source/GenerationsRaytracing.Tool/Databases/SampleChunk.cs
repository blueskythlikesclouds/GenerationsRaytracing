namespace GenerationsRaytracing.Tool.Databases;

internal static class SampleChunk
{
    public static void Write(BinaryObjectWriter writer, int version, Action action)
    {
        writer.Seek(24, SeekOrigin.Begin);
        writer.PushOffsetOrigin();
        action();
        writer.Flush();
        writer.PopOffsetOrigin();
        writer.Seek(0, SeekOrigin.End);
        writer.Align(4);
        long relocPos = writer.Position;
        writer.Write(writer.OffsetHandler.OffsetPositions.Count());
        foreach (long position in writer.OffsetHandler.OffsetPositions)
            writer.Write((uint)(position - 0x18));

        writer.Write(0);
        long length = writer.Position;
        writer.Seek(0, SeekOrigin.Begin);
        writer.Write((uint)length);
        writer.Write(version);
        writer.Write((uint)(relocPos - 0x18));
        writer.Write(0x18);
        writer.Write((uint)relocPos);
        writer.Write((uint)(length - 4));
    }

    public static void Write(Stream destination, int version, Action<BinaryObjectWriter> action)
    {
        using var writer =
            new BinaryObjectWriter(destination, StreamOwnership.Retain, Endianness.Big, Encoding.UTF8);

        Write(writer, version, () => action(writer));
    }
}
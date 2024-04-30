using GenerationsRaytracing.ShaderConverter;
using GenerationsRaytracing.Tool.Databases;
using System.Runtime.CompilerServices;

public class DefaultShaderConverter
{
	private readonly ConcurrentDictionary<ulong, byte[]> _shaders = new();

    private ulong ComputeHash(Span<byte> data)
    {
        int dataOffset = 4;
        int dataSize = data.Length - 4;

        if (Unsafe.As<byte, ushort>(ref data[4]) == 0xFFFE)
        {
            int commentSize = Unsafe.As<byte, ushort>(ref data[6]);
            commentSize += 1;
            commentSize *= 4;

            dataOffset += commentSize;
            dataSize -= commentSize;
        }

        return xxHash3.ComputeHash(data.Slice(dataOffset, dataSize), dataSize);
    }

    private ulong ConvertUnchecked(Span<byte> data)
    {
        ulong hash = ComputeHash(data);

        if (!_shaders.ContainsKey(hash))
            _shaders.TryAdd(hash, ShaderConverter.Convert(data));

        return hash;
    }

    private static readonly byte[] _makeShadowMapTransparentPatch =
        [0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x03, 0xE0, 0x01, 0x00, 0xE4, 0x90, 0x00, 0x00, 0x00, 0x00];

    public void Convert(Span<byte> data)
    {
        ulong hash = ConvertUnchecked(data);

        if (data.Length == 1172)
        {
            int instructionOffset = 0;

            if (hash == 0xDD13000F18B99C6A)
            {
                instructionOffset = 0x1E4;
            }
            else if (hash == 0x761B21E9E3E1DA07)
            {
                instructionOffset = 0x474;
            }

            if (instructionOffset != 0)
            {
                Unsafe.CopyBlock(
                    ref data[instructionOffset],
                    ref _makeShadowMapTransparentPatch[0],
                    (uint)_makeShadowMapTransparentPatch.Length);

                ConvertUnchecked(data);
            }
        }
    }

	public void ConvertArchiveDatabase(string sourceFilePath)
	{
        var archiveDatabase = new ArchiveDatabase(sourceFilePath);
        
        Parallel.ForEach(archiveDatabase.Contents
            .Where(x => x.Name.EndsWith(".wpu") || x.Name.EndsWith(".wvu")), x =>
        {
            Convert(x.Data);
            Console.WriteLine(x.Name);
        });
	}

    public void ConvertExecutable(string sourceFilePath)
    {
        var data = File.ReadAllBytes(sourceFilePath);

        Convert(data.AsSpan(0xFDA158, 0x10C));
        Convert(data.AsSpan(0xFDA268, 0xC10));
        Convert(data.AsSpan(0xFDAE78, 0xC0));
    }

    public void ConvertDll(string sourceFilePath)
    {
        var data = File.ReadAllBytes(sourceFilePath);

        for (int i = 0; i < data.Length; i += 4)
        {
            uint header = Unsafe.As<byte,uint>(ref data[i]);
            if (header == 0xFFFE0300 || header == 0xFFFF0300)
            {
                for (int j = i + 4; j < data.Length; j += 4)
                {
                    uint footer = Unsafe.As<byte, uint>(ref data[j]);
                    if (footer == 0xFFFF)
                    {
                        Convert(data.AsSpan(i, j - i + 4));
                        i = j + 4;
                        break;
                    }
                }
            }
        }
    }

	public void Save(string destinationDirectoryPath)
	{
        byte[] uncompressedBlock;

        using (var stream = new MemoryStream())
        using (var writer = new BinaryWriter(stream))
        {
            foreach (var kvp in _shaders.OrderBy(x => x.Value.Length))
            {
                writer.Write(kvp.Key);
                writer.Write(kvp.Value.Length);
                writer.Write(kvp.Value);
            }

            uncompressedBlock = stream.ToArray();
        }

        var compressedBlock = new byte[LZ4Codec.MaximumOutputSize(uncompressedBlock.Length)];
        Array.Resize(ref compressedBlock, LZ4Codec.Encode(uncompressedBlock, compressedBlock, LZ4Level.L12_MAX));

        using (var stream = File.Create(Path.Combine(destinationDirectoryPath, "pregenerated_shader_cache.bin")))
        using (var writer = new BinaryWriter(stream))
        {
            writer.Write(0);
            writer.Write(uncompressedBlock.Length);
            writer.Write(compressedBlock.Length);
            writer.Write(compressedBlock);
        }
    }
}
using GenerationsRaytracing.ShaderConverter;
using GenerationsRaytracing.Tool.Databases;
using K4os.Compression.LZ4;
using Standart.Hash.xxHash;

if (args.Length < 2)
    return;

var shaders = new Dictionary<ulong, byte[]>();

foreach (var shaderArchiveFilePath in Directory.GetFiles(args[0], "shader_*.ar.00"))
{
    var archiveDatabase = new ArchiveDatabase(shaderArchiveFilePath);

    Parallel.ForEach(archiveDatabase.Contents, x =>
    {
        if (!x.Name.EndsWith(".wpu") && !x.Name.EndsWith(".wvu")) 
            return;

        var shader = ShaderConverter.Convert(x.Data);
        ulong hash = xxHash3.ComputeHash(x.Data, x.Data.Length);

        lock (shaders)
	        shaders[hash] = shader;

        Console.WriteLine(x.Name);
    });
}

byte[] uncompressedBlock;

using (var stream = new MemoryStream())
using (var writer = new BinaryWriter(stream))
{
    foreach (var kvp in shaders.OrderBy(x => x.Value.Length))
    {
        writer.Write(kvp.Key);
        writer.Write(kvp.Value.Length);
        writer.Write(kvp.Value);
    }

    uncompressedBlock = stream.ToArray();
}

var compressedBlock = new byte[LZ4Codec.MaximumOutputSize(uncompressedBlock.Length)];
Array.Resize(ref compressedBlock, LZ4Codec.Encode(uncompressedBlock, compressedBlock, LZ4Level.L12_MAX));

using (var stream = File.Create(Path.Combine(args[1], "pregenerated_shader_cache.bin")))
using (var writer = new BinaryWriter(stream))
{
    writer.Write(0);
    writer.Write(uncompressedBlock.Length);
    writer.Write(compressedBlock.Length);
    writer.Write(compressedBlock);
}
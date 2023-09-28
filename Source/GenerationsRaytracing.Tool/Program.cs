using GenerationsRaytracing.Tool.Databases;
using GenerationsRaytracing.Tool.Shaders;

if (args.Length < 2)
    return;

if (args[1].Contains("bb3"))
{
    foreach (var shaderArchiveFilePath in Directory.GetFiles(args[0], "shader_*.ar.00"))
    {
        var archiveDatabase = new ArchiveDatabase(shaderArchiveFilePath);

        Parallel.ForEach(archiveDatabase.Contents, x =>
        {
            if (!x.Name.EndsWith(".wpu") && !x.Name.EndsWith(".wvu")) 
                return;

            x.Data = ShaderConverter.Convert(x.Data);
            Console.WriteLine(x.Name);
        });

        archiveDatabase.Save(Path.Combine(args[1], Path.GetFileName(shaderArchiveFilePath)), 16, 5 * 1024 * 1024);
    }
}
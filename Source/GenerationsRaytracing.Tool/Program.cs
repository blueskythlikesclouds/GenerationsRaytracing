if (args.Length < 2)
    return;

if (args[0].Contains("bb3"))
{ 
    var defaultShaderConverter = new DefaultShaderConverter();

    defaultShaderConverter.ConvertExecutable(Path.Combine(args[0], "..", "..", "SonicGenerations.exe"));

    foreach (var archiveFilePath in Directory.EnumerateFiles(args[0], "shader_*.ar.00"))
        defaultShaderConverter.ConvertArchiveDatabase(archiveFilePath);

    string betterFxPipelineDirectoryPath = Path.Combine(args[0], "..", "..", "mods", "Better FxPipeline");

    defaultShaderConverter.ConvertDll(Path.Combine(betterFxPipelineDirectoryPath, "BetterFxPipeline.dll"));
    defaultShaderConverter.ConvertArchiveDatabase(Path.Combine(betterFxPipelineDirectoryPath, "disk", "bb3", "BetterFxPipeline.ar.00"));

    defaultShaderConverter.Save(args[1]);
}
else
{ 
    RaytracingShaderCompiler.Compile(args[0], args[1]);
}
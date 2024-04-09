if (args.Length < 2)
    return;

if (args[0].Contains("bb3"))
    DefaultShaderConverter.Convert(args[0], args[1]);
else
    RaytracingShaderCompiler.Compile(args[0], args[1]);
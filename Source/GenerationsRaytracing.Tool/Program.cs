using GenerationsRaytracing.ShaderConverter;
using GenerationsRaytracing.Tool.Databases;
using K4os.Compression.LZ4;
using Standart.Hash.xxHash;

if (args.Length < 2)
    return;

if (args[0].Contains("bb3"))
    DefaultShaderConverter.Convert(args[0], args[1]);
else
    RaytracingShaderCompiler.Compile(args[0], args[1]);
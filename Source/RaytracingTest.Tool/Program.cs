using RaytracingTest.Tool.Databases;
using RaytracingTest.Tool.IO.Extensions;
using RaytracingTest.Tool.Mirage;
using RaytracingTest.Tool.Shaders;
using RaytracingTest.Tool.Shaders.Constants;

if (args.Length < 2)
{
    Console.WriteLine("Usage: RaytracingTest.Tool [bb3] [project]");
    return;
}

if (args[1].Contains("GenerationsRaytracing"))
{
    foreach (var shaderArchiveFilePath in Directory.GetFiles(args[0], "shader_*.ar.00"))
    {
        var archiveDatabase = new ArchiveDatabase(shaderArchiveFilePath);

        Parallel.ForEach(archiveDatabase.Contents, x =>
        {
            if (!x.Name.EndsWith(".wpu") && !x.Name.EndsWith(".wvu")) 
                return;

            x.Data = DefaultShaderConverter.Convert(x.Data);
            Console.WriteLine(x.Name);
        });

        archiveDatabase.Save(Path.Combine(args[1], Path.GetFileName(shaderArchiveFilePath)), 16, 5 * 1024 * 1024);
    }
}
else
{
    var archiveDatabase = new ArchiveDatabase();
    archiveDatabase.Load(Path.Combine(args[0], "shader_r.ar.00"));
    archiveDatabase.Load(Path.Combine(args[0], "shader_r_add.ar.00"));
    
    var stringBuilder = RaytracingShaderConverter.BeginShaderConversion();
    
    using var shaderMappingWriter = new BinaryWriter(File.Create(Path.Combine(args[1], "ShaderMapping.bin")));
    
    foreach (var (databaseData, shaderListData) in archiveDatabase.GetMany<ShaderListData>("shader-list"))
    {
        var defaultPixelShaderPermutation = 
            shaderListData.PixelShaderPermutations.First(x => x.Name.Equals("default"));
    
        var noneVertexShaderPermutation =
            defaultPixelShaderPermutation.VertexShaderPermutations.First(x => x.Name.Equals("none"));
    
        var (_, vertexShaderData) = archiveDatabase.Get<ShaderData>(noneVertexShaderPermutation.ShaderName + "_ConstTexCoord.vertexshader");
        var (_, pixelShaderData) = archiveDatabase.Get<ShaderData>(defaultPixelShaderPermutation.ShaderName + "_NoGI_ConstTexCoord.pixelshader");
    
        var vertexShaderCodeData = archiveDatabase.Get(vertexShaderData.CodeName + ".wvu");
        var pixelShaderCodeData = archiveDatabase.Get(pixelShaderData.CodeName + ".wpu");
    
        if (vertexShaderCodeData == null || pixelShaderCodeData == null)
        {
            vertexShaderData = archiveDatabase.Get<ShaderData>(noneVertexShaderPermutation.ShaderName + ".vertexshader").Data;
            pixelShaderData = archiveDatabase.Get<ShaderData>(defaultPixelShaderPermutation.ShaderName + "_NoGI.pixelshader").Data;
    
            vertexShaderCodeData = archiveDatabase.Get(vertexShaderData.CodeName + ".wvu");
            pixelShaderCodeData = archiveDatabase.Get(pixelShaderData.CodeName + ".wpu");
        }
    
        var shaderMapping = new ShaderMapping();
    
        foreach (string parameterName in vertexShaderData.ParameterNames)
        {
            var (_, parameterData) = archiveDatabase.Get<ShaderParameterData>(parameterName + ".vsparam");
            if (parameterName == "global")
            {
                foreach (var parameter in parameterData.Vectors)
                {
                    switch (parameter.Name)
                    {
                        case "diffuse":
                        case "ambient":
                        case "specular":
                        case "emissive":
                        case "opacity_reflection_refraction_spectype":
                        case "power_gloss_level":
                        case "mrgLuminanceRange":
                        case "mrgFresnelParam":
                            shaderMapping.Float4VertexShaderParameters.Add(parameter);
                            break;
                    }
                }
            }
            else
            {
                shaderMapping.Float4VertexShaderParameters.AddRange(parameterData.Vectors);
            }
        }
    
        foreach (string parameterName in pixelShaderData.ParameterNames)
        {
            var (_, parameterData) = archiveDatabase.Get<ShaderParameterData>(parameterName + ".psparam");
            if (parameterName == "global")
            {
                foreach (var parameter in parameterData.Vectors)
                {
                    switch (parameter.Name)
                    {
                        case "diffuse":
                        case "ambient":
                        case "specular":
                        case "emissive":
                        case "opacity_reflection_refraction_spectype":
                        case "power_gloss_level":
                        case "mrgLuminanceRange":
                        case "mrgFresnelParam":
                            shaderMapping.Float4PixelShaderParameters.Add(parameter);
                            break;
                    }
                }
            }
            else
            {
                shaderMapping.Float4PixelShaderParameters.AddRange(parameterData.Vectors);
                shaderMapping.SamplerParameters.AddRange(parameterData.Samplers);
            }
        }
    
        var vertexShader = ShaderParser.Parse(vertexShaderCodeData.Data);
        var pixelShader = ShaderParser.Parse(pixelShaderCodeData.Data);
    
        string shaderName = Path.GetFileNameWithoutExtension(databaseData.Name);
        string raytracingFunctionName = shaderName.Replace('[', '_').Replace(']', '_');
        RaytracingShaderConverter.WriteAllFunctions(stringBuilder, raytracingFunctionName, vertexShader, pixelShader, shaderMapping);
    
        foreach (var (name, index) in shaderMapping.Float4Indices.OrderBy(x => x.Value)) 
            stringBuilder.AppendFormat("// {0}: {1}\n", name, index);
    
        stringBuilder.AppendLine();
    
        foreach (var (parameter, index) in shaderMapping.SamplerIndices.OrderBy(x => x.Value))
            stringBuilder.AppendFormat("// {0}: {1}\n", parameter.Name, index);
    
        stringBuilder.AppendLine();
    
        shaderMappingWriter.Write(shaderName);
        shaderMappingWriter.Write(raytracingFunctionName + "_closesthit");
        shaderMappingWriter.Write(raytracingFunctionName + "_anyhit");
        shaderMappingWriter.Write(shaderMapping.Float4Indices.Count);
    
        foreach (var (name, _) in shaderMapping.Float4Indices.OrderBy(x => x.Value)) 
            shaderMappingWriter.Write(name);
    
        shaderMappingWriter.Write(shaderMapping.SamplerIndices.Count);
    
        foreach (var (parameter, _) in shaderMapping.SamplerIndices.OrderBy(x => x.Value))
            shaderMappingWriter.Write(parameter.Name);
    }
    
    File.WriteAllText(Path.Combine(args[1], "ShaderLibrary.hlsl"), stringBuilder.ToString());
}
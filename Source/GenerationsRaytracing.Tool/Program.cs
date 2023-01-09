using GenerationsRaytracing.Tool;
using GenerationsRaytracing.Tool.Databases;
using GenerationsRaytracing.Tool.Mirage;
using GenerationsRaytracing.Tool.Shaders;
using GenerationsRaytracing.Tool.Shaders.Constants;

if (args.Length < 2)
{
    Console.WriteLine("Usage: RaytracingTest.Tool [bb3] [project]");
    return;
}

if (args[1].Contains("bb3"))
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

    //Console.WriteLine("VS: {0} PS: {1}", DefaultShaderConverter.MaxVertexConstant, DefaultShaderConverter.MaxPixelConstant);
}
else
{
    var materialParameters = new HashSet<string>
    {
        "diffuse",
        "ambient",
        "specular",
        "emissive",
        "opacity_reflection_refraction_spectype",
        "power_gloss_level",
        "mrgLuminanceRange",
        "mrgFresnelParam",
        "mrgTexcoordOffset"
    };

    var vertexConstants = new Dictionary<string, int>();
    var pixelConstants = new Dictionary<string, int>();

    var archiveDatabase = new ArchiveDatabase();
    archiveDatabase.Load(Path.Combine(args[0], "shader_r.ar.00"));
    archiveDatabase.Load(Path.Combine(args[0], "shader_r_add.ar.00"));
    
    var stringBuilder = RaytracingShaderConverter.BeginShaderConversion();

    var vertexShaders = new Dictionary<string, (Shader Shader, ShaderData ShaderData)>();
    var pixelShaders = new Dictionary<string, (Shader Shader, ShaderData ShaderData)>();
    var shaderLists = new List<(string ShaderName, Shader VertexShader, Shader PixelShader, ShaderMapping ShaderMapping)>();
    
    foreach (var (databaseData, shaderListData) in archiveDatabase.GetMany<ShaderListData>("shader-list"))
    {
        if (databaseData.Name.StartsWith("csd") || 
            databaseData.Name.Contains("ShadowMap") ||
            databaseData.Name.Contains("Font") ||
            databaseData.Name.Contains("RenderBuffer") ||
            databaseData.Name.Contains("GrassInstance"))
            continue;

        var defaultPixelShaderPermutation = 
            shaderListData.PixelShaderPermutations.First(x => x.Name.Equals("default"));
    
        var noneVertexShaderPermutation =
            defaultPixelShaderPermutation.VertexShaderPermutations.First(x => x.Name.Equals("none"));

        string vertexShaderName = Utilities.FixupShaderName(noneVertexShaderPermutation.ShaderName);

        if (!vertexShaders.TryGetValue(vertexShaderName, out var vertexShader))
        {
            vertexShader.ShaderData = archiveDatabase.Get<ShaderData>(noneVertexShaderPermutation.ShaderName + "_ConstTexCoord.vertexshader").Data;
            var vertexShaderCodeData = archiveDatabase.Get(vertexShader.ShaderData.CodeName + ".wvu");

            if (vertexShaderCodeData == null)
            {
                vertexShader.ShaderData = archiveDatabase.Get<ShaderData>(noneVertexShaderPermutation.ShaderName + ".vertexshader").Data;
                vertexShaderCodeData = archiveDatabase.Get(vertexShader.ShaderData.CodeName + ".wvu");
            }

            vertexShader.Shader = ShaderParser.Parse(vertexShaderCodeData.Data);
            vertexShader.Shader.Name = vertexShaderName;

            vertexShaders.Add(vertexShader.Shader.Name, vertexShader);

            foreach (var constant in vertexShader.Shader.Constants.Where(x => x.Type == ConstantType.Float4))
                vertexConstants[constant.Name] = constant.Register;
        }

        string pixelShaderName = Utilities.FixupShaderName(defaultPixelShaderPermutation.ShaderName.Replace('@', '_'));

        if (!pixelShaders.TryGetValue(pixelShaderName, out var pixelShader))
        {
            pixelShader.ShaderData = archiveDatabase.Get<ShaderData>(defaultPixelShaderPermutation.ShaderName + "_NoLight_NoGI_ConstTexCoord.pixelshader").Data;
            var pixelShaderCodeData = archiveDatabase.Get(pixelShader.ShaderData.CodeName + ".wpu");

            if (pixelShaderCodeData == null)
            {
                pixelShader.ShaderData = archiveDatabase.Get<ShaderData>(defaultPixelShaderPermutation.ShaderName + "_NoLight_NoGI.pixelshader").Data;
                pixelShaderCodeData = archiveDatabase.Get(pixelShader.ShaderData.CodeName + ".wpu");
            }

            pixelShader.Shader = ShaderParser.Parse(pixelShaderCodeData.Data);
            pixelShader.Shader.Name = pixelShaderName;

            pixelShaders.Add(pixelShader.Shader.Name, pixelShader);

            foreach (var constant in pixelShader.Shader.Constants.Where(x => x.Type == ConstantType.Float4))
                pixelConstants[constant.Name] = constant.Register;
        }

        var shaderMapping = new ShaderMapping();

        foreach (string parameterName in vertexShader.ShaderData.ParameterNames)
        {
            var (_, parameterData) = archiveDatabase.Get<ShaderParameterData>(parameterName + ".vsparam");
            if (parameterName == "global")
            {
                foreach (var parameter in parameterData.Vectors)
                {
                    if (materialParameters.Contains(parameter.Name))
                        shaderMapping.Float4VertexShaderParameters.Add(parameter);
                }
            }
            else
            {
                shaderMapping.Float4VertexShaderParameters.AddRange(parameterData.Vectors);
            }
        }

        foreach (string parameterName in pixelShader.ShaderData.ParameterNames)
        {
            var (_, parameterData) = archiveDatabase.Get<ShaderParameterData>(parameterName + ".psparam");
            if (parameterName == "global")
            {
                foreach (var parameter in parameterData.Vectors)
                {
                    if (materialParameters.Contains(parameter.Name))
                        shaderMapping.Float4PixelShaderParameters.Add(parameter);
                }
            }
            else
            {
                shaderMapping.Float4PixelShaderParameters.AddRange(parameterData.Vectors);
                shaderMapping.SamplerParameters.AddRange(parameterData.Samplers);
            }
        }

        shaderLists.Add((Path.GetFileNameWithoutExtension(databaseData.Name), vertexShader.Shader, pixelShader.Shader, shaderMapping));
    }

    foreach (var (_, vertexShader) in vertexShaders)
        RaytracingShaderConverter.WriteShaderFunction(stringBuilder, vertexShader.Shader);  
    
    foreach (var (_, pixelShader) in pixelShaders)
        RaytracingShaderConverter.WriteShaderFunction(stringBuilder, pixelShader.Shader);

    using var shaderMappingWriter = new BinaryWriter(File.Create(Path.Combine(args[1], "ShaderMapping.bin")));

    var raytracingShaderIndices = new Dictionary<string, int>();
    var shaderListPairs = new List<(string Name, int Index)>();

    foreach (var (shaderName, vertexShader, pixelShader, shaderMapping) in shaderLists)
    {
        string raytracingShaderName = Utilities.FixupShaderName(vertexShader.Name + "_" + pixelShader.Name);

        if (!raytracingShaderIndices.TryGetValue(raytracingShaderName, out int index))
        {
            RaytracingShaderConverter.WriteRaytracingFunctions(
                stringBuilder, 
                raytracingShaderName, 
                raytracingShaderIndices[raytracingShaderName] = index = (ushort)raytracingShaderIndices.Count, 
                vertexShader, 
                pixelShader, 
                shaderMapping);

            shaderMappingWriter.Write(raytracingShaderName);
            shaderMappingWriter.Write((byte)shaderMapping.Float4Indices.Count);

            foreach (var (name, _) in shaderMapping.Float4Indices.OrderBy(x => x.Value))
                shaderMappingWriter.Write(name);

            shaderMappingWriter.Write((byte)shaderMapping.SamplerIndices.Count);

            foreach (var (parameter, _) in shaderMapping.SamplerIndices.OrderBy(x => x.Value))
                shaderMappingWriter.Write(parameter.Name);
        }

        shaderListPairs.Add((shaderName, index));
    }
    shaderMappingWriter.Write(string.Empty);

    if (raytracingShaderIndices.Count > 256)
        throw new Exception("Raytracing shader count isn't going to fit to a single byte.");

    foreach (var (name, index) in shaderListPairs)
    {
        shaderMappingWriter.Write(name);
        shaderMappingWriter.Write((byte)index);
    }
    shaderMappingWriter.Write(string.Empty);

    File.WriteAllText(Path.Combine(args[1], "ShaderLibraryAutoGenerated.hlsl"), stringBuilder.ToString());

    foreach (var (name, register) in vertexConstants.OrderBy(x => x.Value))
        Console.WriteLine("float4 {0} : packoffset(c{1});", name, register);  
    
    foreach (var (name, register) in pixelConstants.OrderBy(x => x.Value))
        Console.WriteLine("float4 {0} : packoffset(c{1});", name, register);
}

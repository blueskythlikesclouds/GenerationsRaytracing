using RaytracingTest.Tool.Databases;
using RaytracingTest.Tool.IO.Extensions;
using RaytracingTest.Tool.Mirage;
using RaytracingTest.Tool.Shaders;
using RaytracingTest.Tool.Shaders.Constants;

var archiveDatabase = new ArchiveDatabase();
archiveDatabase.Load(@"C:\Program Files (x86)\Steam\steamapps\common\Sonic Generations\disk\bb3\shader_r.ar.00");
archiveDatabase.Load(@"C:\Program Files (x86)\Steam\steamapps\common\Sonic Generations\disk\bb3\shader_r_add.ar.00");

var stringBuilder = ShaderConverter.BeginShaderConversion();
var constants = new List<(string Name, int Register)>();

using var shaderMappingWriter = new BinaryWriter(File.Create(@"C:\Repositories\RaytracingTest\Source\RaytracingTest\ShaderMapping.bin"));

foreach (var (databaseData, shaderListData) in archiveDatabase.GetMany<ShaderListData>("shader-list"))
{
    var defaultPixelShaderPermutation = 
        shaderListData.PixelShaderPermutations.First(x => x.Name.Equals("default"));

    var noneVertexShaderPermutation =
        defaultPixelShaderPermutation.VertexShaderPermutations.First(x => x.Name.Equals("none"));

    var (_, vertexShaderData) = archiveDatabase.Get<ShaderData>(noneVertexShaderPermutation.ShaderName + "_ConstTexCoord.vertexshader");
    var (_, pixelShaderData) = archiveDatabase.Get<ShaderData>(defaultPixelShaderPermutation.ShaderName + "_NoLight_NoGI_ConstTexCoord.pixelshader");

    var vertexShaderCodeData = archiveDatabase.Get(vertexShaderData.CodeName + ".wvu");
    var pixelShaderCodeData = archiveDatabase.Get(pixelShaderData.CodeName + ".wpu");

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
    ShaderConverter.WriteAllFunctions(stringBuilder, raytracingFunctionName, vertexShader, pixelShader, shaderMapping);

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

    foreach (var constant in vertexShader.Constants.Concat(pixelShader.Constants).Where(x => x.Type != ConstantType.Sampler))
    {
        if (constants.All(x => x.Name != constant.Name))
            constants.Add((constant.Name, constant.Register));
    }
}

foreach (var constant in constants.OrderBy(x => x.Register))
    Console.WriteLine(constant.Name);

File.WriteAllText("ShaderLibrary.hlsl", stringBuilder.ToString());
using GenerationsRaytracing.Tool.Mirage;

namespace GenerationsRaytracing.Tool.Shaders;

public class ShaderMapping
{
    public List<ShaderParameter> Float4VertexShaderParameters { get; set; } = new();
    public List<ShaderParameter> Float4PixelShaderParameters { get; set; } = new();
    public Dictionary<string, int> Float4Indices { get; set; } = new();

    public List<ShaderParameter> SamplerParameters { get; set; } = new();
    public Dictionary<ShaderParameter, int> SamplerIndices { get; set; } = new();

    public bool CanMapFloat4(int register, ShaderType type)
    {
        return (type == ShaderType.Pixel ? Float4PixelShaderParameters : Float4VertexShaderParameters).Any(x => x.Index == register);
    }

    public int MapFloat4(int register, ShaderType type)
    {
        var parameter = (type == ShaderType.Pixel ? Float4PixelShaderParameters : Float4VertexShaderParameters).First(x => x.Index == register);

        if (!Float4Indices.TryGetValue(parameter.Name, out int index)) 
            index = (Float4Indices[parameter.Name] = Float4Indices.Count);

        return index;
    }

    public bool CanMapSampler(int register)
    {
        return SamplerParameters.Any(x => (x.Index & 0xF) == register);
    }

    public int MapSampler(int register)
    {
        var parameter = SamplerParameters.First(x => (x.Index & 0xF) == register);

        if (!SamplerIndices.TryGetValue(parameter, out int index))
            index = (SamplerIndices[parameter] = SamplerIndices.Count);

        return index;
    }
}
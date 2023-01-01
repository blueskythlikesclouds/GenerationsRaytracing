namespace GenerationsRaytracing.Tool.Mirage.Extensions;

public static class ShaderParameterDataEx
{
    public static bool IsEmpty(this ShaderParameterData shaderParameterData)
    {
        return shaderParameterData.Vectors.Count == 0 && shaderParameterData.Booleans.Count == 0 &&
               shaderParameterData.Integers.Count == 0 && shaderParameterData.Samplers.Count == 0;
    }
}
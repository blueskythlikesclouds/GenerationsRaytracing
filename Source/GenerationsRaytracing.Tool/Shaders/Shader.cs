using GenerationsRaytracing.Tool.Mirage;
using GenerationsRaytracing.Tool.Shaders.Constants;
using GenerationsRaytracing.Tool.Shaders.Instructions;

namespace GenerationsRaytracing.Tool.Shaders;

public class Shader
{
    public Dictionary<string, string> InSemantics { get; set; } = new();
    public Dictionary<string, string> OutSemantics { get; set; } = new();
    public Dictionary<string, string> Samplers { get; set; } = new();
    public Dictionary<int, string> Definitions { get; set; } = new();
    public Dictionary<int, string> DefinitionsInt { get; set; } = new();
    public List<Instruction> Instructions { get; set; } = new();
    public List<Constant> Constants { get; set; } = new();

    public ShaderType Type { get; set; }
    public List<ShaderParameter> Parameters { get; set; }
}
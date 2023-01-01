namespace GenerationsRaytracing.Tool.Mirage;

public class ShaderParameterData : IBinarySerializable
{
    public List<ShaderParameter> Vectors { get; }
    public List<ShaderParameter> Integers { get; }
    public List<ShaderParameter> Booleans { get; }
    public List<ShaderParameter> Samplers { get; }

    public void Read(BinaryObjectReader reader)
    {
        ReadParameters(Vectors);
        ReadParameters(Integers);
        ReadParameters(Booleans);
        ReadParameters(Samplers);

        void ReadParameters(List<ShaderParameter> parameters)
        {
            int count = reader.ReadInt32();
            reader.ReadOffset(() =>
            {
                for (int i = 0; i < count; i++)
                    parameters.Add(reader.ReadObjectOffset<ShaderParameter>());
            });
        }
    }

    public void Write(BinaryObjectWriter writer)
    {
        WriteParameters(Vectors);
        WriteParameters(Integers);
        WriteParameters(Booleans);
        WriteParameters(Samplers);
        writer.Write(0ul);

        void WriteParameters(List<ShaderParameter> parameters)
        {
            writer.Write(parameters.Count);
            writer.WriteOffset(() =>
            {
                foreach (var parameter in parameters)
                    writer.WriteObjectOffset(parameter, 4);
            }, 4);
        }
    }

    public ShaderParameterData()
    {
        Vectors = new List<ShaderParameter>();
        Integers = new List<ShaderParameter>();
        Booleans = new List<ShaderParameter>();
        Samplers = new List<ShaderParameter>();
    }
}
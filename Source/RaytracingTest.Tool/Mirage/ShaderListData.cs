namespace RaytracingTest.Tool.Mirage;

public class ShaderListData : IBinarySerializable
{
    public List<PixelShaderPermutationData> PixelShaderPermutations { get; }

    public void Read(BinaryObjectReader reader)
    {
        int count = reader.ReadInt32();
        reader.ReadOffset(() =>
        {
            for (int i = 0; i < count; i++)
                PixelShaderPermutations.Add(reader.ReadObjectOffset<PixelShaderPermutationData>());
        });
    }

    public void Write(BinaryObjectWriter writer)
    {
        writer.Write(PixelShaderPermutations.Count);
        writer.WriteOffset(() =>
        {
            foreach (var pixelShaderPermutation in PixelShaderPermutations)
                writer.WriteObjectOffset(pixelShaderPermutation, 4);
        }, 4);
    }

    public ShaderListData()
    {
        PixelShaderPermutations = new List<PixelShaderPermutationData>();
    }
}
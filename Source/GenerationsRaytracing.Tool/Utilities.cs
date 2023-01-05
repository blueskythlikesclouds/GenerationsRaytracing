namespace GenerationsRaytracing.Tool;

public class Utilities
{
    public static string FixupShaderName(string input)
    {
        input = input.Replace("[", "_").Replace("]", "_").Replace("@", "_").Trim('_');

        for (int i = 0; i < input.Length - 1; i++)
        {
            if (input[i] == '_' && input[i + 1] == '_')
                input = input.Remove(i, 1);
        }

        return input;
    }
}
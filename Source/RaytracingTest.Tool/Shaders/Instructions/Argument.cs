namespace RaytracingTest.Tool.Shaders.Instructions;

public class Argument
{
    public bool Not { get; set; }
    public bool Sign { get; set; }
    public string Token { get; set; }
    public SwizzleSet Swizzle { get; set; }
    public bool Abs { get; set; }
    public Argument Index { get; set; }

    public override string ToString()
    {
        var stringBuilder = new StringBuilder();

        if (Not)
            stringBuilder.Append('!');

        if (Sign)
            stringBuilder.Append('-');

        if (Abs)
            stringBuilder.Append("abs(");

        stringBuilder.Append(Token);

        if (Index != null)
            stringBuilder.AppendFormat("[{0}]", Index);

        stringBuilder.Append(Swizzle);

        if (Abs)
            stringBuilder.Append(')');

        return stringBuilder.ToString();
    }

    public Argument(string argument)
    {
        argument = argument.Trim();

        if (argument.StartsWith("!"))
        {
            argument = argument.Substring(1);
            Not = true;
        }

        if (argument.StartsWith("-"))
        {
            argument = argument.Substring(1);
            Sign = true;
        }

        int absIndex = argument.LastIndexOf("_abs");
        int periodIndex = argument.LastIndexOf(".");
        int indexPos = argument.IndexOf(']');

        Token = argument;

        if (absIndex != -1 && absIndex > indexPos)
        {
            Abs = true;
            Token = argument.Substring(0, absIndex);
        }

        if (periodIndex != -1 && periodIndex > indexPos)
        {
            Swizzle = new SwizzleSet(argument.Substring(periodIndex + 1));

            if (absIndex == -1)
                Token = argument.Substring(0, periodIndex);
        }
        else
        {
            Swizzle = SwizzleSet.Empty;
        }

        indexPos = Token.IndexOf('[');
        if (indexPos != -1)
        {
            Index = new Argument(Token.Substring(indexPos + 1, Token.IndexOf(']') - indexPos - 1));
            Token = Token.Substring(0, indexPos);
        }
    }
}
namespace RaytracingTest.Tool.Shaders.Instructions;

public class Instruction
{
    public string OpCode { get; set; }
    public Argument[] Arguments { get; set; }
    public bool Saturate { get; set; }
    public ComparisonType ComparisonType { get; set; }

    public override string ToString()
    {
        var stringBuilder = new StringBuilder();

        switch (OpCode)
        {
            case "abs":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = abs({1});", Arguments[0], Arguments[1]);
                break;

            case "add":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1} + {2};", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "break":
                if (Arguments != null)
                    EmitIfCondition(stringBuilder);

                stringBuilder.Append("break;");
                break;

            case "cmp":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[3].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = select({1} >= 0, {2}, {3});", Arguments[0], Arguments[1], Arguments[2], Arguments[3]);
                break;

            case "dp2add":
                Arguments[1].Swizzle.Resize(2);
                Arguments[2].Swizzle.Resize(2);
                Arguments[3].Swizzle.Resize(1);

                stringBuilder.AppendFormat("{0} = dot({1}, {2}) + {3};", Arguments[0], Arguments[1], Arguments[2], Arguments[3]);
                break;

            case "dp3":
                Arguments[1].Swizzle.Resize(3);
                Arguments[2].Swizzle.Resize(3);

                stringBuilder.AppendFormat("{0} = dot({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "dp4":
                Arguments[1].Swizzle.Resize(4);
                Arguments[2].Swizzle.Resize(4);

                stringBuilder.AppendFormat("{0} = dot({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "dsx":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = ddx({1});", Arguments[0], Arguments[1]);
                break;

            case "dsy":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = ddy({1});", Arguments[0], Arguments[1]);
                break;

            case "exp":
                Arguments[1].Swizzle.Resize(1);

                stringBuilder.AppendFormat("{0} = exp2({1});", Arguments[0], Arguments[1]);
                break;

            case "frc":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = frac({1});", Arguments[0], Arguments[1]);
                break;

            case "log":
                Arguments[1].Swizzle.Resize(1);

                stringBuilder.AppendFormat("{0} = max(log2(abs({1})), -FLT_MAX);", Arguments[0], Arguments[1]);
                break;

            case "lrp":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[3].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = lerp({1}, {2}, {3});", Arguments[0], Arguments[3], Arguments[2], Arguments[1]);
                break;

            case "mad":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[3].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1} * {2} + {3};", Arguments[0], Arguments[1], Arguments[2], Arguments[3]);
                break;

            case "mov":
            case "mova":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1};", Arguments[0], Arguments[1]);
                break;

            case "mul":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1} * {2};", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "max":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = max({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "min":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = min({1}, {2});", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "nrm":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1} * min(1.0 / sqrt(dot(({1}).xyz, ({1}).xyz)), FLT_MAX);", Arguments[0], Arguments[1]);
                break;

            case "pow":
                Arguments[1].Swizzle.Resize(1);
                Arguments[2].Swizzle.Resize(1);

                stringBuilder.AppendFormat("{0} = exp2(max(log2(abs({1})), -FLT_MAX) * {2});", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "rcp":
                Arguments[1].Swizzle.Resize(1);

                stringBuilder.AppendFormat("{0} = min(1.0 / {1}, FLT_MAX);", Arguments[0], Arguments[1]);
                break;

            case "rep":
                Arguments[0].Swizzle.Resize(1);

                stringBuilder.AppendFormat("rep({0}) {{", Arguments[0]);
                break;

            case "rsq":
                Arguments[1].Swizzle.Resize(1);

                stringBuilder.AppendFormat("{0} = min(1.0 / sqrt(abs({1})), FLT_MAX);", Arguments[0], Arguments[1]);
                break;

            case "sge":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1} >= {2};", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "sincos":
                stringBuilder.AppendFormat("{0} = float2(cos({1}), sin({1})){2};", Arguments[0], Arguments[1], Arguments[0].Swizzle);
                break;

            case "slt":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1} < {2};", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "sub":
                Arguments[1].Swizzle.Convert(Arguments[0].Swizzle);
                Arguments[2].Swizzle.Convert(Arguments[0].Swizzle);

                stringBuilder.AppendFormat("{0} = {1} - {2};", Arguments[0], Arguments[1], Arguments[2]);
                break;

            case "texld":
                stringBuilder.AppendFormat("{0} = {1}.SampleLevel(g_LinearRepeatSampler, {2}, 0);", Arguments[0], Arguments[2], Arguments[1]);
                break;

            case "texldb":
                Arguments[1].Swizzle.Resize(4);

                stringBuilder.AppendFormat("{0} = {1}.SampleBias(g_LinearRepeatSampler, {2}, ({2}).w);", Arguments[0], Arguments[2], Arguments[1]);
                break;

            case "texldd":
                Arguments[1].Swizzle.Resize(4);
                Arguments[3].Swizzle.Resize(4);
                Arguments[4].Swizzle.Resize(4);

                stringBuilder.AppendFormat("{0} = {1}.SampleGrad(g_LinearRepeatSampler, {2}, {3}, {4});", Arguments[0], Arguments[2], Arguments[1], Arguments[3], Arguments[4]);
                break;

            case "texldl":
                Arguments[1].Swizzle.Resize(4);

                stringBuilder.AppendFormat("{0} = {1}.SampleLevel(g_LinearRepeatSampler, {2}, ({2}).w);", Arguments[0], Arguments[2], Arguments[1]);
                break;

            case "texldp":
                Arguments[1].Swizzle.Resize(4);

                //stringBuilder.AppendFormat("{0} = {1}.SampleCmpLevelZero(g_ComparisonSampler, ({2}).xy / ({2}).w, ({2}).z / ({2}).w);", Arguments[0], Arguments[2], Arguments[1]);
                break;

            case "if":
                EmitIfCondition(stringBuilder);

                stringBuilder.Append("{");
                break;

            case "else":
                stringBuilder.Append("} else {");
                break;

            case "endif":
            case "endrep":
                stringBuilder.Append("}");
                break;

            default:
                stringBuilder.AppendFormat("// Unimplemented instruction: {0}", OpCode);

                if (Saturate)
                    stringBuilder.Append("_sat");

                if (Arguments == null)
                    return stringBuilder.ToString();

                for (var i = 0; i < Arguments.Length; i++)
                {
                    if (i != 0)
                        stringBuilder.Append(',');

                    stringBuilder.AppendFormat(" {0}", Arguments[i]);
                }

                return stringBuilder.ToString();
        }

        if (Saturate)
            stringBuilder.AppendFormat(" {0} = saturate({0});", Arguments[0]);

        return stringBuilder.ToString();
    }

    public void EmitIfCondition(StringBuilder stringBuilder)
    {
        stringBuilder.Append("if (");

        switch (ComparisonType)
        {
            case ComparisonType.GreaterThan:
                stringBuilder.AppendFormat("{0} > {1}", Arguments[0], Arguments[1]);
                break;
            case ComparisonType.LessThan:
                stringBuilder.AppendFormat("{0} < {1}", Arguments[0], Arguments[1]);
                break;
            case ComparisonType.GreaterThanOrEqual:
                stringBuilder.AppendFormat("{0} >= {1}", Arguments[0], Arguments[1]);
                break;
            case ComparisonType.LessThanOrEqual:
                stringBuilder.AppendFormat("{0} <= {1}", Arguments[0], Arguments[1]);
                break;
            case ComparisonType.Equal:
                stringBuilder.AppendFormat("{0} == {1}", Arguments[0], Arguments[1]);
                break;
            case ComparisonType.NotEqual:
                stringBuilder.AppendFormat("{0} != {1}", Arguments[0], Arguments[1]);
                break;
            default:
                stringBuilder.Append(Arguments[0]);
                break;
        }

        stringBuilder.Append(") ");
    }

    public Instruction(string instruction)
    {
        var args = instruction.Split(',');

        var opArgsSplit = args[0].Split(' ');
        if (opArgsSplit.Length > 1)
            args[0] = opArgsSplit[1];
        else
            args = null;

        var opCodeSplit = opArgsSplit[0].Split('_');

        OpCode = opCodeSplit[0];

        foreach (string token in opCodeSplit)
        {
            switch (token)
            {
                case "sat":
                    Saturate = true;
                    break;

                case "gt":
                    ComparisonType = ComparisonType.GreaterThan;
                    break;

                case "lt":
                    ComparisonType = ComparisonType.LessThan;
                    break;

                case "ge":
                    ComparisonType = ComparisonType.GreaterThanOrEqual;
                    break;

                case "le":
                    ComparisonType = ComparisonType.LessThanOrEqual;
                    break;

                case "eq":
                    ComparisonType = ComparisonType.Equal;
                    break;

                case "ne":
                    ComparisonType = ComparisonType.NotEqual;
                    break;
            }
        }

        if (args != null)
            Arguments = args.Select(x => new Argument(x)).ToArray();
    }
}
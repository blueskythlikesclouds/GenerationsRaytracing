namespace GenerationsRaytracing.Tool.Shaders.Instructions;

public class SwizzleSet
{
    public static readonly char[] SwizzleChars = {'x', 'y', 'z', 'w'};

    public static SwizzleSet Empty =>
        new SwizzleSet(Swizzle.Empty, Swizzle.Empty, Swizzle.Empty, Swizzle.Empty);

    public static SwizzleSet XYZW =>
        new SwizzleSet(Swizzle.X, Swizzle.Y, Swizzle.Z, Swizzle.W);

    public Swizzle X;
    public Swizzle Y;
    public Swizzle Z;
    public Swizzle W;

    public Swizzle GetSwizzle(int index)
    {
        switch (index)
        {
            case 0: return X;
            case 1: return Y;
            case 2: return Z;
            case 3: return W;
        }

        return Swizzle.Empty;
    }

    public void SetSwizzle(int index, Swizzle swizzle)
    {
        switch (index)
        {
            case 0:
                X = swizzle;
                break;
            case 1:
                Y = swizzle;
                break;
            case 2:
                Z = swizzle;
                break;
            case 3:
                W = swizzle;
                break;
        }
    }

    public int Count
    {
        get
        {
            int count = X != Swizzle.Empty ? 1 : 0;
            count += Y != Swizzle.Empty ? 1 : 0;
            count += Z != Swizzle.Empty ? 1 : 0;
            count += W != Swizzle.Empty ? 1 : 0;

            return count;
        }
    }

    public void Convert(SwizzleSet dstSwizzleSet)
    {
        Expand(4);

        var newSwizzle = Empty;

        if (dstSwizzleSet.Count == 0)
            dstSwizzleSet = XYZW;

        for (int i = 0; i < 4; i++)
        {
            var swizzle = dstSwizzleSet.GetSwizzle(i);
            if (swizzle == Swizzle.Empty)
                break;

            switch (swizzle)
            {
                case Swizzle.X:
                    newSwizzle.SetSwizzle(i, GetSwizzle(0));
                    break;

                case Swizzle.Y:
                    newSwizzle.SetSwizzle(i, GetSwizzle(1));
                    break;

                case Swizzle.Z:
                    newSwizzle.SetSwizzle(i, GetSwizzle(2));
                    break;

                case Swizzle.W:
                    newSwizzle.SetSwizzle(i, GetSwizzle(3));
                    break;
            }
        }

        Replace(newSwizzle);
    }

    public void Shrink(int count)
    {
        int thisCount = Count;

        if (thisCount <= count)
            return;

        for (int i = count; i < 4; i++)
            SetSwizzle(i, Swizzle.Empty);
    }

    public void Expand(int count)
    {
        int thisCount = Count;

        if (thisCount >= count)
            return;

        if (thisCount == 0)
        {
            for (int i = 0; i < count; i++)
                SetSwizzle(i, XYZW.GetSwizzle(i));
        }
        else
        {
            for (int i = thisCount; i < count; i++)
                SetSwizzle(i, GetSwizzle(i - 1));
        }
    }

    public void Resize(int count)
    {
        int thisCount = Count;

        if (thisCount > count)
            Shrink(count);

        else if (thisCount < count)
            Expand(count);
    }

    public void Simplify()
    {
        if (X == Swizzle.X && Y == Swizzle.Y && Z == Swizzle.Z && W == Swizzle.W)
        {
            Replace(Empty);
        }

        else
        {
            bool allSame = true;

            for (int i = 1; i < Count; i++)
            {
                if (GetSwizzle(i) == X)
                    continue;

                allSame = false;
                break;
            }

            if (allSame)
            {
                Y = Swizzle.Empty;
                Z = Swizzle.Empty;
                W = Swizzle.Empty;
            }
        }
    }

    public void Replace(SwizzleSet swizzleSet)
    {
        X = swizzleSet.X;
        Y = swizzleSet.Y;
        Z = swizzleSet.Z;
        W = swizzleSet.W;
    }

    public override string ToString()
    {
        var stringBuilder = new StringBuilder(4);

        for (int i = 0; i < 4; i++)
        {
            var swizzle = GetSwizzle(i);
            if (swizzle == Swizzle.Empty)
                break;

            stringBuilder.Append((char)swizzle);
        }

        if (stringBuilder.Length > 0)
            stringBuilder.Insert(0, '.');

        return stringBuilder.ToString();
    }

    protected bool Equals(SwizzleSet other)
    {
        return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
    }

    public override bool Equals(object obj)
    {
        if (ReferenceEquals(null, obj)) return false;
        if (ReferenceEquals(this, obj)) return true;
        if (obj.GetType() != this.GetType()) return false;
        return Equals((SwizzleSet)obj);
    }

    public override int GetHashCode()
    {
        return HashCode.Combine((int)X, (int)Y, (int)Z, (int)W);
    }

    public static bool operator ==(SwizzleSet left, SwizzleSet right)
    {
        return Equals(left, right);
    }

    public static bool operator !=(SwizzleSet left, SwizzleSet right)
    {
        return !Equals(left, right);
    }

    public SwizzleSet(Swizzle x, Swizzle y, Swizzle z, Swizzle w)
    {
        X = x;
        Y = y;
        Z = z;
        W = w;
    }

    public SwizzleSet(string argument)
    {
        X = Swizzle.Empty;
        Y = Swizzle.Empty;
        Z = Swizzle.Empty;
        W = Swizzle.Empty;

        for (int i = 0; i < argument.Length; i++)
            SetSwizzle(i, (Swizzle)argument[i]);
    }
}
namespace GenerationsRaytracing.Tool.Databases;

public class DatabaseData
{
    public string Name { get; set; }
    public byte[] Data { get; set; }

    public override string ToString() => Name;
}

public class ArchiveDatabase
{
    public List<DatabaseData> Contents { get; }

    public void LoadSingle(string filePath)
    {
        using var source = File.OpenRead(filePath);
        using var reader = new BinaryReader(source, Encoding.UTF8);
        var stringBuilder = new StringBuilder();

        source.Seek(16, SeekOrigin.Begin);
        while (source.Position < source.Length)
        {
            long currentOffset = source.Position;
            long blockSize = reader.ReadUInt32();
            long dataSize = reader.ReadUInt32();
            long dataOffset = reader.ReadUInt32();
            _ = reader.ReadInt64();
            
            char c;
            while ((c = reader.ReadChar()) != 0)
                stringBuilder.Append(c);

            source.Seek(currentOffset + dataOffset, SeekOrigin.Begin);
            Contents.Add(new DatabaseData
            {
                Name = stringBuilder.ToString(),
                Data = reader.ReadBytes((int)dataSize),
            });

            stringBuilder.Clear();

            source.Seek(currentOffset + blockSize, SeekOrigin.Begin);
        }
    }

    public void Load(string filePath)
    {
        if (!filePath.EndsWith(".ar.00", StringComparison.OrdinalIgnoreCase))
        {
            LoadSingle(filePath);
            return;
        }

        filePath = filePath[..^3];

        int splitCount = -1;
        
        string arlFilePath = filePath + "l";
        if (File.Exists(arlFilePath))
        {
            using var source = File.OpenRead(arlFilePath);
            using var reader = new BinaryReader(source);
            if (reader.ReadInt32() == 0x324C5241) // ARL2
                splitCount = reader.ReadInt32();
        }

        for (int i = 0; splitCount == -1 || i < splitCount; i++)
        {
            string currentFilePath = $"{filePath}.{i:D2}";
            if (!File.Exists(currentFilePath))
                break;

            LoadSingle(currentFilePath);
        }
    }

    public ArchiveDatabase()
    {
        Contents = new List<DatabaseData>();
    }

    public ArchiveDatabase(string filePath) : this()
    {
        Load(filePath);
    }
}
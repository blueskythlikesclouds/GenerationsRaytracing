using RaytracingTest.Tool.IO.Extensions;

namespace RaytracingTest.Tool.Databases;

public enum ConflictPolicy
{
    RaiseError,
    Replace,
    Ignore
}

public class DatabaseData
{
    public string Name { get; set; }
    public byte[] Data { get; set; }
    public DateTime Time { get; set; }

    public DatabaseData()
    {
        Time = DateTime.Now;
    }

    public override string ToString() => Name;
}

public class ArchiveDatabase
{
    public List<DatabaseData> Contents { get; }

    public void LoadSingle(string filePath)
    {
        var time = File.GetLastWriteTime(filePath);

        using var reader = new BinaryValueReader(filePath, Endianness.Little, Encoding.UTF8);

        reader.Seek(16, SeekOrigin.Begin);
        while (reader.Position < reader.Length)
        {
            long currentOffset = reader.Position;
            long blockSize = reader.Read<uint>();
            long dataSize = reader.Read<uint>();
            long dataOffset = reader.Read<uint>();
            long ticks = reader.Read<long>();
            string name = reader.ReadString(StringBinaryFormat.NullTerminated);

            reader.Seek(currentOffset + dataOffset, SeekOrigin.Begin);
            Contents.Add(new DatabaseData
            {
                Name = name,
                Data = reader.ReadArray<byte>((int)dataSize),
                Time = ticks != 0 ? new DateTime(ticks) : time
            });

            reader.Seek(currentOffset + blockSize, SeekOrigin.Begin);
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
            using var reader = new BinaryValueReader(arlFilePath, Endianness.Little, Encoding.UTF8);
            if (reader.Read<int>() == 0x324C5241) // ARL2
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

    public void Save(string filePath, int padding = 16, int maxSplitSize = 10 * 1024 * 1024)
    {
        bool splitMode = filePath.EndsWith(".ar.00", StringComparison.OrdinalIgnoreCase);

        if (splitMode)
            filePath = filePath[..^2];

        var fileSizes = new List<long>();

        BinaryValueWriter writer = null;

        for (var i = 0; i < Contents.Count; i++)
        {
            if (writer == null)
            {
                writer = new BinaryValueWriter(
                    splitMode ? $"{filePath}{fileSizes.Count:D2}" : filePath, Endianness.Little, Encoding.UTF8);

                writer.Write(0);
                writer.Write(16);
                writer.Write(20);
                writer.Write(padding);
            }

            var databaseData = Contents[i];

            long currentOffset = writer.Position;
            long dataOffsetUnaligned = currentOffset + 21 + Encoding.UTF8.GetByteCount(databaseData.Name);
            long dataOffset = AlignmentHelper.Align(dataOffsetUnaligned, padding);
            long blockSize = dataOffset + databaseData.Data.Length;

            writer.Write((uint)(blockSize - currentOffset));
            writer.Write(databaseData.Data.Length);
            writer.Write((uint)(dataOffset - currentOffset));
            writer.Write(databaseData.Time.Ticks);
            writer.WriteString(StringBinaryFormat.NullTerminated, databaseData.Name);
            writer.Align(padding);
            writer.WriteBytes(databaseData.Data);

            if ((!splitMode || writer.Length <= maxSplitSize) && i != Contents.Count - 1)
                continue;

            fileSizes.Add(writer.Length);

            writer.Dispose();
            writer = null;
        }

        if (!splitMode)
            return;

        writer = new BinaryValueWriter(filePath[..^1] + "l", Endianness.Little, Encoding.UTF8);

        writer.WriteString(StringBinaryFormat.FixedLength, "ARL2", 4);
        writer.Write((uint)fileSizes.Count);

        foreach (long fileSize in fileSizes)
            writer.Write((uint)fileSize);

        foreach (var databaseData in Contents)
            writer.WriteString(StringBinaryFormat.PrefixedLength8, databaseData.Name);

        writer.Dispose();
    }

    public void Add(DatabaseData databaseData, ConflictPolicy conflictPolicy = ConflictPolicy.RaiseError)
    {
        lock (((ICollection)Contents).SyncRoot)
        {
            var data = Contents.FirstOrDefault(x => x.Name.Equals(databaseData.Name));
            if (data != null)
            {
                switch (conflictPolicy)
                {
                    case ConflictPolicy.RaiseError:
                        throw new ArgumentException("Data with same name already exists", nameof(databaseData));

                    case ConflictPolicy.Replace:
                        Contents.Remove(data);
                        break;

                    case ConflictPolicy.Ignore:
                        return;
                }
            }

            Contents.Add(databaseData);
        }
    }

    public DatabaseData Get(string name)
    {
        return Contents.FirstOrDefault(x => x.Name.Equals(name, StringComparison.OrdinalIgnoreCase));
    }

    public IEnumerable<DatabaseData> GetMany(string extension) 
    {
        foreach (var databaseData in Contents)
        {
            if (databaseData.Name.EndsWith(extension, StringComparison.OrdinalIgnoreCase))
                yield return databaseData;
        }
    }

    public (DatabaseData DatabaseData, T Data) Get<T>(string name) where T : IBinarySerializable, new()
    {
        var databaseData = Get(name);
        if (databaseData == null)
            return (null, default);

        var data = new T();
        data.Load(databaseData.Data);
        return (databaseData, data);
    }

    public IEnumerable<(DatabaseData DatabaseData, T Data)> GetMany<T>(string extension) where T : IBinarySerializable, new()
    {
        foreach (var databaseData in Contents)
        {
            if (!databaseData.Name.EndsWith(extension, StringComparison.OrdinalIgnoreCase)) 
                continue;

            var data = new T();
            data.Load(databaseData.Data);
            yield return (databaseData, data);
        }
    }

    public void Sort()
    {
        Contents.Sort((x, y) => string.Compare(x.Name, y.Name, StringComparison.Ordinal));
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
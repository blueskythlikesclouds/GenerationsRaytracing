namespace RaytracingTest.Tool.Shaders;

[Guid("8BA5FB08-5195-40e2-AC58-0D989C3A0102")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface ID3DBlob
{
    [PreserveSig]
    IntPtr GetBufferPointer();

    [PreserveSig]
    int GetBufferSize();
}

public class Compiler
{
    [PreserveSig]
    [DllImport("D3DCompiler_47.dll", EntryPoint = "D3DDisassemble")]
    public static extern unsafe int Disassemble(
        void* pSrcData,
        int SrcDataSize,
        uint flags,
        void* szComments,
        out ID3DBlob ppDisassembly);

    [PreserveSig]
    [DllImport("D3DCompiler_47.dll", EntryPoint = "D3DCompile")]
    public static extern unsafe int Compile(
        [MarshalAs(UnmanagedType.LPStr)] string pSrcData,
        int SrcDataSize,
        [MarshalAs(UnmanagedType.LPStr)] string pSourceName,
        void* pDefines,
        void* pInclude,
        [MarshalAs(UnmanagedType.LPStr)] string pEntrypoint,
        [MarshalAs(UnmanagedType.LPStr)] string pTarget,
        uint Flags1,
        uint Flags2,
        out ID3DBlob ppCode,
        out ID3DBlob ppErrorMsgs);
}
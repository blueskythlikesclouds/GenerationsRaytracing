namespace GenerationsRaytracing.Tool.Shaders;

[Guid("8BA5FB08-5195-40e2-AC58-0D989C3A0102")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface ID3DBlob
{
    [PreserveSig]
    IntPtr GetBufferPointer();

    [PreserveSig]
    int GetBufferSize();
}

public class Disassembler
{
    [PreserveSig]
    [DllImport("D3DCompiler_47.dll", EntryPoint = "D3DDisassemble")]
    public static extern unsafe int Disassemble(
        void* pSrcData,
        int SrcDataSize,
        uint flags,
        void* szComments,
        out ID3DBlob ppDisassembly);
}
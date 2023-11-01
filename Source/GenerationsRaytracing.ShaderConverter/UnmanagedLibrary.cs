namespace GenerationsRaytracing.ShaderConverter;

public static unsafe class UnmanagedLibrary
{
	[UnmanagedCallersOnly(EntryPoint = "ConvertShader")]
	public static void* ConvertShader(void* data, int dataSize, int* convertedSize)
	{
		var bytes = ShaderConverter.Convert(data, dataSize);
		*convertedSize = bytes.Length;

		return GCHandle.ToIntPtr(GCHandle.Alloc(bytes, GCHandleType.Pinned)).ToPointer();
	}

	[UnmanagedCallersOnly(EntryPoint = "GetPointerFromHandle")]
	public static void* GetPointerFromHandle(void* handle)
	{
		return GCHandle.FromIntPtr((IntPtr)handle).AddrOfPinnedObject().ToPointer();
	}

	[UnmanagedCallersOnly(EntryPoint = "FreeHandle")]
	public static void FreeHandle(void* handle)
	{
		GCHandle.FromIntPtr((IntPtr)handle).Free();
	}
}
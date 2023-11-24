using Vortice.Dxc;

public static class RaytracingShaderCompiler
{
	private static readonly List<(string ShaderName, string ShaderType)> ShaderNameAndTypePairs = new()
	{
		("BillboardParticle_", "SYS_ERROR"),
		("BillboardParticleY_", "SYS_ERROR"),
		("BlbBlend_", "SYS_ERROR"),
		("BlbCommon_", "SYS_ERROR"),
		("BlbIndirect_", "SYS_ERROR"),
		("BlbLuminescence_", "SYS_ERROR"),
		("Blend_", "BLEND"),
		("Chaos_", "SYS_ERROR"),
		("ChaosV_", "SYS_ERROR"),
		("ChrEye_", "CHR_EYE"),
		("ChrSkin_", "CHR_SKIN"),
		("ChrSkinHalf_", "CHR_SKIN_HALF"),
		("ChrSkinIgnore_", "CHR_SKIN_IGNORE"),
		("Cloak_", "SYS_ERROR"),
		("Cloth_", "CLOTH"),
		("Cloud_", "CLOUD"),
		("Common_", "COMMON"),
		("Deformation_", "SYS_ERROR"),
		("DeformationParticle_", "SYS_ERROR"),
		("Dim_", "DIM"),
		("DimIgnore_", "SYS_ERROR"),
		("Distortion_", "DISTORTION"),
		("DistortionOverlay_", "DISTORTION_OVERLAY"),
		("DistortionOverlayChaos_", "SYS_ERROR"),
		("EnmCloud_", "CLOUD"),
		("EnmEmission_", "ENM_EMISSION"),
		("EnmGlass_", "ENM_GLASS"),
		("EnmIgnore_", "ENM_IGNORE"),
		("EnmMetal_", "CHR_SKIN"),
		("FadeOutNormal_", "FADE_OUT_NORMAL"),
		("FakeGlass_", "ENM_GLASS"),
		("FallOff_", "FALLOFF"),
		("FallOffV_", "FALLOFF_V"),
		("Glass_", "GLASS"),
		("GlassRefraction_", "SYS_ERROR"),
		("Ice_", "ICE"),
		("IgnoreLight_", "IGNORE_LIGHT"),
		("IgnoreLightTwice_", "IGNORE_LIGHT_TWICE"),
		("IgnoreLightV_", "IGNORE_LIGHT_V"),
		("Indirect_", "INDIRECT"),
		("IndirectV_", "INDIRECT_V"),
		("IndirectVnoGIs_", "INDIRECT_V_NO_GI_SHADOW"),
		("Lava_", "SYS_ERROR"),
		("Luminescence_", "LUMINESCENCE"),
		("LuminescenceV_", "LUMINESCENCE_V"),
		("MeshParticle_", "SYS_ERROR"),
		("MeshParticleLightingShadow_", "SYS_ERROR"),
		("MeshParticleRef_", "SYS_ERROR"),
		("Mirror_", "MIRROR"),
		("Mirror2_", "MIRROR"),
		("Myst_", "SYS_ERROR"),
		("Parallax_", "SYS_ERROR"),
		("Ring_", "RING"),
		("TimeEater_", "SYS_ERROR"),
		("TimeEaterDistortion_", "SYS_ERROR"),
		("TimeEaterEmission_", "SYS_ERROR"),
		("TimeEaterGlass_", "SYS_ERROR"),
		("TimeEaterIndirect_", "SYS_ERROR"),
		("TimeEaterMetal_", "SYS_ERROR"),
		("TransThin_", "TRANS_THIN"),
		("Water_Add", "WATER_ADD"),
		("Water_Mul", "WATER_MUL"),
		("Water_Opacity", "WATER_OPACITY")
	};

	private static readonly List<string> ShaderTypes = ShaderNameAndTypePairs
		.Select(x => x.ShaderType)
		.Distinct()
		.ToList();

	private static void WriteShaderTypeHeaderFile(string solutionDirectoryPath)
	{
		using var writer = new StreamWriter(
			Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.Shared", "ShaderType.h"));

		writer.WriteLine("#ifndef SHADER_TYPE_H_INCLUDED");
		writer.WriteLine("#define SHADER_TYPE_H_INCLUDED");

		for (int i = 0; i < ShaderTypes.Count; i++)
			writer.WriteLine("#define SHADER_TYPE_{0} {1}", ShaderTypes[i], i);

		writer.WriteLine("#define SHADER_TYPE_MAX {0}", ShaderTypes.Count);

		writer.WriteLine("#ifdef __cplusplus");
		writer.WriteLine("#include <string_view>");
		writer.WriteLine("inline std::pair<std::string_view, size_t> s_shaderTypes[] =");
		writer.WriteLine("{");

		foreach (var (shaderName, shaderType) in ShaderNameAndTypePairs)
			writer.WriteLine("\t{{\"{0}\", SHADER_TYPE_{1}}},", shaderName, shaderType);

		writer.WriteLine("};");

		writer.WriteLine("inline const wchar_t* s_shaderHitGroups[] =");
		writer.WriteLine("{");

		foreach (var shaderType in ShaderTypes)
		{
			writer.WriteLine("\tL\"{0}_PrimaryHitGroup\",", shaderType);
			writer.WriteLine("\tL\"{0}_PrimaryHitGroup_ConstTexCoord\",", shaderType);
			writer.WriteLine("\tL\"ShadowHitGroup\",");
			writer.WriteLine("\tL\"{0}_SecondaryHitGroup\",", shaderType);
		}

		writer.WriteLine("};");

		writer.WriteLine("#endif");
		writer.WriteLine("#endif");
	}

	public static void Compile(string solutionDirectoryPath, string destinationFilePath)
	{
		WriteShaderTypeHeaderFile(solutionDirectoryPath);

		var shaderSources = new List<string>(ShaderTypes.Count);
		var stringBuilder = new StringBuilder();

		foreach (var shaderType in ShaderTypes)
		{
			stringBuilder.AppendLine("#include \"DXILLibrary.hlsli\"");

			stringBuilder.AppendLine("[shader(\"anyhit\")]");
			stringBuilder.AppendFormat(
				"void {0}_PrimaryAnyHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)\n",
				shaderType);
			stringBuilder.AppendLine("{");
			stringBuilder.AppendFormat("\tPrimaryAnyHit(SHADER_TYPE_{0}, payload, attributes);\n",
				shaderType);
			stringBuilder.AppendLine("}");

			stringBuilder.AppendLine("[shader(\"closesthit\")]");
			stringBuilder.AppendFormat(
				"void {0}_PrimaryClosestHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)\n",
				shaderType);
			stringBuilder.AppendLine("{");
			stringBuilder.AppendFormat("\tPrimaryClosestHit(VERTEX_FLAG_MIPMAP | VERTEX_FLAG_MULTI_UV | VERTEX_FLAG_SAFE_POSITION, SHADER_TYPE_{0}, payload, attributes);\n",
				shaderType);
			stringBuilder.AppendLine("}");

			stringBuilder.AppendLine("[shader(\"closesthit\")]");
			stringBuilder.AppendFormat(
				"void {0}_PrimaryClosestHit_ConstTexCoord(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)\n",
				shaderType);
			stringBuilder.AppendLine("{");
			stringBuilder.AppendFormat("\tPrimaryClosestHit(VERTEX_FLAG_MIPMAP | VERTEX_FLAG_SAFE_POSITION, SHADER_TYPE_{0}, payload, attributes);\n",
				shaderType);
			stringBuilder.AppendLine("}");

			stringBuilder.AppendLine("[shader(\"closesthit\")]");
			stringBuilder.AppendFormat(
				"void {0}_SecondaryClosestHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)\n",
				shaderType);
			stringBuilder.AppendLine("{");
			stringBuilder.AppendFormat("\tSecondaryClosestHit(SHADER_TYPE_{0}, payload, attributes);\n",
				shaderType);
			stringBuilder.AppendLine("}");

			stringBuilder.AppendFormat("TriangleHitGroup {0}_PrimaryHitGroup =\n", shaderType);
			stringBuilder.AppendLine("{");
			stringBuilder.AppendFormat("\t\"{0}_PrimaryAnyHit\",\n", shaderType);
			stringBuilder.AppendFormat("\t\"{0}_PrimaryClosestHit\"\n", shaderType);
			stringBuilder.AppendLine("};");

			stringBuilder.AppendFormat("TriangleHitGroup {0}_PrimaryHitGroup_ConstTexCoord =\n", shaderType);
			stringBuilder.AppendLine("{");
			stringBuilder.AppendFormat("\t\"{0}_PrimaryAnyHit\",\n", shaderType);
			stringBuilder.AppendFormat("\t\"{0}_PrimaryClosestHit_ConstTexCoord\"\n", shaderType);
			stringBuilder.AppendLine("};");

			stringBuilder.AppendFormat("TriangleHitGroup {0}_SecondaryHitGroup =\n", shaderType);
			stringBuilder.AppendLine("{");
			stringBuilder.AppendLine("\t\"SecondaryAnyHit\",");
			stringBuilder.AppendFormat("\t\"{0}_SecondaryClosestHit\"\n", shaderType);
			stringBuilder.AppendLine("};");

			shaderSources.Add(stringBuilder.ToString());
			stringBuilder.Clear();
		}

		var args = new[]
		{
			"-T lib_6_6",
			"-HV 2021",
			"-enable-payload-qualifiers",
			"-all-resources-bound",
			$"-I {Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X64")}",
			$"-I {Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.Shared")}",
			$"-I {Path.Combine(solutionDirectoryPath, "..", "Dependencies", "self-intersection-avoidance")}"
		};

		var includeHandler = DxcCompiler.Utils.CreateDefaultIncludeHandler();
		var shaderBlobs = new byte[shaderSources.Count][];

		int progress = 0;

		Parallel.For(0, shaderSources.Count, i =>
		{
			var dxcResult = DxcCompiler.Compile(shaderSources[i], args, includeHandler);

			var byteCode = dxcResult.GetObjectBytecode();
			string errors = dxcResult.GetErrors();

			if (byteCode.IsEmpty)
				throw new Exception(errors);
			
			if (!string.IsNullOrEmpty(errors))
				Console.WriteLine(dxcResult.GetErrors());

			shaderBlobs[i] = byteCode.ToArray();

			Console.WriteLine("({0}/{1}): {2}", Interlocked.Increment(ref progress), ShaderTypes.Count, ShaderTypes[i]);
		});

		using var stream = File.Create(destinationFilePath);
		using var writer = new BinaryWriter(stream);

		foreach (var shaderBlob in shaderBlobs)
		{
			writer.Write(shaderBlob.Length);
			writer.Write(shaderBlob);
		}
	}
}
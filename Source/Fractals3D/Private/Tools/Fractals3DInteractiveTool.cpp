// Copyright PupSik, 2023. All Rights Reserved.

#include "Fractals3DInteractiveTool.h"
#include "InteractiveToolManager.h"
#include "Interfaces/IPluginManager.h"
#include "Fractals3DEditorModeCommands.h"
#include "ToolBuilderUtil.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include "BaseBehaviors/ClickDragBehavior.h"
#include "SourceControlHelpers.h"

#include "SceneManagement.h"

// localization namespace
#define LOCTEXT_NAMESPACE "UFractals3DInteractiveTool"

UInteractiveTool* UFractals3DInteractiveToolBuilder::BuildTool(const FToolBuilderState & SceneState) const
{
	UFractals3DInteractiveTool* NewTool = NewObject<UFractals3DInteractiveTool>(SceneState.ToolManager);
	return NewTool;
}


UFractals3DInteractiveToolProperties::UFractals3DInteractiveToolProperties()
{
	// initialize the points and distance to reasonable values
	FractalName = "Default";
	LastSDF = FractalConfigSDF::Mandelbrot;
}


void UFractals3DInteractiveTool::Setup()
{
	UInteractiveTool::Setup();

	const_cast<FFractals3DEditorModeCommands&>(FFractals3DEditorModeCommands::Get()).SetFractalTool(this);

	// Create the property set and register it with the Tool
	Properties = NewObject<UFractals3DInteractiveToolProperties>(this, "Measurement");
	AddToolPropertySource(Properties);

	Properties->WatchProperty(Properties->FractalName,
		[this](FString FractalName) {
			FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Fractals3D"))->GetBaseDir(), TEXT("Shaders"));
			
			FString ShaderName = Properties->FractalName;
			ShaderName += ".ush";
			FString JsonName = Properties->FractalName;
			JsonName += "_Conf.json";

			FString Shader = FPaths::Combine(PluginShaderDir, ShaderName);
			FString Json = FPaths::Combine(PluginShaderDir, JsonName);

			if (std::filesystem::exists(std::filesystem::path(TCHAR_TO_ANSI(*Shader)))) {
				FString FileData = "";
				FJsonFractalProperties FractalJSON;
				FFileHelper::LoadFileToString(FileData, *Json);

				if (FJsonObjectConverter::JsonObjectStringToUStruct(FileData, &FractalJSON, 0, 0))
				{
					Properties->FractalConfig = FractalJSON.FractalConfig;
					Properties->LastSDF = FractalJSON.LastSDF;
				}
			}
		}
	);

	
}

void UFractals3DInteractiveTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{

}

FString FunctionNameByFoldType(FractalFoldConfig Type)
{
	switch (Type)
	{
		case FractalFoldConfig::PlaneFold: return "planeFold";
		case FractalFoldConfig::AbsFold: return "absFold";
		case FractalFoldConfig::SierpinskiFold: return "sierpinskiFold";
		case FractalFoldConfig::MengerFold: return "mengerFold";
		case FractalFoldConfig::SphereFold: return "sphereFold";
		case FractalFoldConfig::BoxFold: return "boxFold";
		case FractalFoldConfig::RotXFold: return "rotXFold";
		case FractalFoldConfig::RotYFold: return "rotYFold";
		case FractalFoldConfig::RotZFold: return "rotZFold";
		case FractalFoldConfig::ScaleTranslateFold: return "scaleTranslateFold";
		case FractalFoldConfig::ScaleOriginFold: return "scaleOriginFold";
		case FractalFoldConfig::OrbitColoring: return "orbitByType";
	}

	return "Error";
}

FString FunctionArgumentsByFoldType(FractalFoldConfig Type)
{
	switch (Type)
	{
		case FractalFoldConfig::PlaneFold: return "(new_p, float3(0, 0, -1), -1)";
		case FractalFoldConfig::AbsFold: return "(new_p, float3(0.0f, 0.0f, 0.0f))";
		case FractalFoldConfig::SierpinskiFold: return "(new_p)";
		case FractalFoldConfig::MengerFold: return "(new_p)";
		case FractalFoldConfig::SphereFold: return "(new_p, 0.5, 1.0f)";
		case FractalFoldConfig::BoxFold: return "(new_p, library.FoldSize)";
		case FractalFoldConfig::RotXFold: return "(new_p, library.FoldAngles.x)";
		case FractalFoldConfig::RotYFold: return "(new_p, library.FoldAngles.y)";
		case FractalFoldConfig::RotZFold: return "(new_p, library.FoldAngles.z)";
		case FractalFoldConfig::ScaleTranslateFold: return "(new_p, library.FoldingScale, library.Power * library.FoldOffset)";
		case FractalFoldConfig::ScaleOriginFold: return "(new_p, library.FoldingScale)";
		case FractalFoldConfig::OrbitColoring: return "(orbit, new_p)";
	}
	return "Error";
}

FString FunctionBySDFType(FractalConfigSDF Type)
{
	switch (Type)
	{
	case FractalConfigSDF::Cone: return "float2(min(d, library.sdCone(new_p, library.Offset)), length(new_p))";
	case FractalConfigSDF::HexPrism: return "float2(min(d, library.sdHexPrism(new_p, float2(library.Offset, library.Offset)), length(new_p))";
	case FractalConfigSDF::Sphere: return "float2(min(d, library.sdSphere(new_p, library.Offset)), length(new_p))";
	case FractalConfigSDF::Capsule: return "float2(min(d, library.sdCapsule(new_p, library.FoldingScale, library.Offset)), length(new_p))";
	case FractalConfigSDF::Torus: return "float2(min(d, library.sdTorus(new_p, float2(library.Offset, library.Offset))), length(new_p))";
	case FractalConfigSDF::Box: return "float2(min(d, library.sdBox(new_p, float3(library.Offset, library.Offset, library.Offset))), length(new_p))";
	case FractalConfigSDF::Tetrahedron: return "float2(min(d, library.sdTetrahedron(new_p, library.Offset)), length(new_p))";
	case FractalConfigSDF::InfCross: return "float2(min(d, library.sdInfCross(new_p, library.Offset)), length(new_p))";
	case FractalConfigSDF::InfCrossXY: return "float2(min(d, library.sdInfCrossXY(new_p, library.Offset)), length(new_p))";
	case FractalConfigSDF::InfLine: return "float2(min(d, library.sdInfLine(new_p, library.Offset)), length(new_p))";
	case FractalConfigSDF::Julia2: return "library.sdJulia(new_p, outputColor)";
	case FractalConfigSDF::Julia: return "library.sdJulia2(new_p, outputColor)";
	case FractalConfigSDF::Mandelbrot: return "library.sdMondelbrot(new_p, outputColor)";
	}
	return "Error";
}

bool IsOrbit(FractalFoldConfig Type)
{
	return Type == FractalFoldConfig::OrbitColoring;
}

void UFractals3DInteractiveTool::GenerateFractal() const {
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Fractals3D"))->GetBaseDir(), TEXT("Shaders"));

	FString MainShaderFilename = Properties->FractalName;
	MainShaderFilename += ".ush";

	FString SdfShaderFilename = Properties->FractalName;
	SdfShaderFilename += "SDF.ush";

	std::filesystem::remove(std::filesystem::path(TCHAR_TO_ANSI(*FPaths::Combine(PluginShaderDir, MainShaderFilename))));
	std::filesystem::remove(std::filesystem::path(TCHAR_TO_ANSI(*FPaths::Combine(PluginShaderDir, SdfShaderFilename))));

	// Fill MainShader
	FString MainShader = "#include \"/PluginShaders/SDFractalLibrary.ush\"\n"
		"\n"
		"struct SDF {\n"
		"SDFractal library;\n"
		"float3 outputColor;\n"
		"\n"
		"#include \"/PluginShaders/";
	MainShader += Properties->FractalName;
	MainShader += "SDF.ush\"\n"
		"};\n"
		"\n"
		"#include \"/PluginShaders/RayMarchingFractal.ush\"\n"
		;

	// Fill SDF Shader
	FString SdfShader = "float2 sdf(float3 p) {\n"
		"	outputColor = library.orbitInitInf();\n"
		"	float4 new_p = float4(p, 1.0f);\n"
		"	float d = 1e20;\n"
		"	for (int i = 0; i < library.Iterations; i++) {\n"
		"";

	for (int i = 0; i < Properties->FractalConfig.Num(); ++i)
	{
		if (IsOrbit(Properties->FractalConfig[i]))
			SdfShader += "		outputColor = library.orbitByType(outputColor, new_p);\n";
		else
		{
			SdfShader += "		new_p = library.";
			SdfShader += FunctionNameByFoldType(Properties->FractalConfig[i]);
			SdfShader += FunctionArgumentsByFoldType(Properties->FractalConfig[i]);
			SdfShader += ";\n";
		}
	}

	SdfShader += "\n"
		"	}\n";

	SdfShader += "	return ";
	SdfShader += FunctionBySDFType(Properties->LastSDF);
	SdfShader += ";\n}\n";

	std::ofstream foutMainShader(*FPaths::Combine(PluginShaderDir, MainShaderFilename));
	foutMainShader << std::string(TCHAR_TO_ANSI(* MainShader));
	std::ofstream foutSdfShader(*FPaths::Combine(PluginShaderDir, SdfShaderFilename));
	foutSdfShader << std::string(TCHAR_TO_ANSI(* SdfShader));

	FString Buffer;
	FJsonFractalProperties Fractal;
	Fractal.FractalConfig = Properties->FractalConfig;
	Fractal.LastSDF = Properties->LastSDF;

	FJsonObjectConverter::UStructToJsonObjectString(Fractal, Buffer);

	FString ConfigName = Properties->FractalName;
	ConfigName += "_Conf.json";

	std::ofstream foutShaderJson(*FPaths::Combine(PluginShaderDir, ConfigName));
	foutShaderJson << std::string(TCHAR_TO_ANSI(*Buffer));

}


#undef LOCTEXT_NAMESPACE

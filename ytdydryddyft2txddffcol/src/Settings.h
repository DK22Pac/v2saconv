#pragma once
#include <Windows.h>
#include <stdio.h>
#include "inih\INIReader.h"

class _settings
{
public:
	bool m_bEnableLog;
	bool m_bHashedFileNames;

	unsigned int m_nLodLevel;
	float m_fScaleMultiplier;

	// textures
	unsigned int m_nAnisotropyLevel;
	unsigned int m_nBaseTextureLevel;
	bool m_bExtendTextureName;

	// skeleton
	bool m_bExportSkeleton;

	// vertex data
	bool m_bExportVertexColors;
	bool m_bExportNormals;

	// material
	float m_fMaterialDiffuse;
	float m_fMaterialAmbient;
	bool m_bExportVehicleColors;
	bool m_bExportNormalMap;

	// vertex processing
	bool m_bGenerateDayPrelight;
	bool m_bGenerateNightPrelight;
	bool m_bGenerateDPForDynamicObjects;
	bool m_bGenerateNPForDynamicObjects;
	float m_fNightPrelightBaseLevel;
	float m_fNightPrelightMultiplier;
	float m_fPrelightSunX;
	float m_fPrelightSunY;
	float m_fPrelightSunZ;
	unsigned char m_nPrelightSunColorR;
	unsigned char m_nPrelightSunColorG;
	unsigned char m_nPrelightSunColorB;
	float m_fPrelightSunRangeA;
	float m_fPrelightSunRangeB;
	float m_fPrelightNightIntensity;

	// 2dfx
	float m_f2dfxCoronaSizeMultiplier;
	float m_f2dfxPointlightSizeMultiplier;
	float m_f2dfxShadowSizeMultiplier;
	unsigned int m_n2dfxShadowIntensity;
	float m_f2dfxCoronaFarClip;

	// collision
	bool m_bGenerateCollision;
	unsigned char m_nCollisionModelLighting;
	bool m_bOptimizeCollisionMesh;
	bool m_bGenerateCollisionAABBTree;
	bool m_bSkipLargeCol;

	bool m_bGenerateBreakableModel;
	float m_fBreakableLighting;
	float m_fBrokenLighting;

	int m_nFirstUVChannel;
	int m_nSecondUVChannel;
	
	bool m_bExportAsVehicle;
	bool m_bExportVehicleLightMaterials;
	bool m_bExportImVehFtLightMaterials;

	INIReader *m_pReader;

	int readParam(char *key, int defaultVal)
	{
		return m_pReader->GetInteger("GENRL", key, defaultVal);
	}

	float readParam(char *key, float defaultVal)
	{
		return m_pReader->GetReal("GENRL", key, defaultVal);
	}

	_settings()
	{
		m_pReader = new INIReader("GeneralSettings.ini");
		m_bEnableLog = readParam("bEnableLog", 1);
		m_bHashedFileNames = readParam("bHashedFileNames", 0);
		m_nLodLevel = readParam("iLodLevel", 0);
		if(m_nLodLevel > 3)
			m_nLodLevel = 3;
		m_fScaleMultiplier = readParam("fScaleMultiplier", 1.0f);
		m_nBaseTextureLevel = readParam("iBaseTextureLevel", 0);
		m_nAnisotropyLevel = readParam("iAnisotropyLevel", 0);
		m_bExtendTextureName = readParam("bExtendTextureName", 0);
		m_bExportSkeleton = readParam("bExportSkeleton", 0);
		m_bExportVertexColors = readParam("bExportVertexColors", 1);
		m_bExportNormals = readParam("bExportNormals", 1);
		m_bGenerateDayPrelight = readParam("bGenerateDayPrelight", 1);
		m_bGenerateNightPrelight = readParam("bGenerateNightPrelight", 1);
		m_bGenerateDPForDynamicObjects = readParam("bGenerateDPForDynamicObjects", 0);
		m_bGenerateNPForDynamicObjects = readParam("bGenerateNPForDynamicObjects", 0);
		m_fMaterialDiffuse = readParam("fMaterialDiffuse", 1.0f);
		m_fMaterialAmbient = readParam("fMaterialAmbient", 1.0f);
		m_bExportNormalMap = readParam("bExportNormalMap", 0);
		m_bExportVehicleColors = readParam("bExportVehicleColors", 1);
		m_fPrelightSunX = readParam("fPrelightSunX", 1.0f);
		m_fPrelightSunY = readParam("fPrelightSunY", 1.0f);
		m_fPrelightSunZ = readParam("fPrelightSunZ", 0.5f);
		m_nPrelightSunColorR = readParam("iPrelightSunColorR", 255);
		m_nPrelightSunColorG = readParam("iPrelightSunColorR", 255);
		m_nPrelightSunColorB = readParam("iPrelightSunColorR", 255);
		m_fPrelightSunRangeA = readParam("fPrelightSunRangeA", 0.5f);
		m_fPrelightSunRangeB = readParam("fPrelightSunRangeB", 1.0f);
		m_fNightPrelightBaseLevel = readParam("fNightPrelightBaseLevel", 0.4f);
		m_fNightPrelightMultiplier = readParam("fNightPrelightMultiplier", 1.0f);
		m_bGenerateCollision = readParam("bGenerateCollision", 1);
		m_nCollisionModelLighting = readParam("iCollisionModelLighting", 255);
		m_bOptimizeCollisionMesh = readParam("bOptimizeCollisionMesh", 1);
		m_bGenerateCollisionAABBTree = readParam("bGenerateCollisionAABBTree", 1);
		m_bSkipLargeCol = readParam("bSkipLargeCol", 1);
		m_f2dfxCoronaSizeMultiplier = readParam("f2dfxCoronaSizeMultiplier", 1.3f);
		m_f2dfxPointlightSizeMultiplier = readParam("f2dfxPointlightSizeMultiplier", 1.0f);
		m_f2dfxShadowSizeMultiplier = readParam("f2dfxShadowSizeMultiplier", 1.0f);
		m_n2dfxShadowIntensity = readParam("i2dfxShadowIntensity", 100);
		m_f2dfxCoronaFarClip = readParam("f2dfxCoronaFarClip", 150.0f);
		m_bGenerateBreakableModel = readParam("bGenerateBreakableModel", 0);
		m_fBreakableLighting = readParam("fBreakableLighting", 0.3f);
		m_fBrokenLighting = readParam("fBrokenLighting", 0.2f);
		m_nFirstUVChannel = readParam("iFirstUVChannel", 0);
		m_nSecondUVChannel = readParam("iSecondUVChannel", -1);
		m_bExportAsVehicle = readParam("bExportAsVehicle", 0);
		m_bExportVehicleLightMaterials = readParam("bExportVehicleLightMaterials", 1);
		m_bExportImVehFtLightMaterials = readParam("bExportImVehFtLightMaterials", 1);
		delete m_pReader;
	}
};

_settings settings;
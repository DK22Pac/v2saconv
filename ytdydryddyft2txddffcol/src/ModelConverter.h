#pragma once
#include "RageResource.h"
#include "dffapi\Clump.h"
#include "Hash.h"
#include "Settings.h"
#include "dffapi\Memory.h"
#include "TextureConverter.h"
#include "dffapi\Geometry.h"
#include "MatFlags.h"
#include "ColFile.h"
#include "ColMaterials.h"
#include "NormalMap.h"
#include "VehicleConverter.h"
#include <new>
#include <vector>

using namespace std;

class model_converter
{
public:
	struct skeleton_info
	{
		bool exportSkeleton;
		Skeleton *skeleton;
		D3DMATRIX *matrices;

		skeleton_info(): exportSkeleton(0), skeleton(0), matrices(0) {}
	};

	struct material_info
	{
		ShaderParam *pDiffuse;
		ShaderParam *pNormal;
		ShaderParam *pCarCol;
		ShaderParam *pEmissive;

		material_info(): pDiffuse(0), pNormal(0), pCarCol(0), pEmissive(0) {}
	};

	static void set_mat_color(gtaRwMaterial& mat, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		mat.color.r = r;
		mat.color.g = g;
		mat.color.b = b;
		mat.color.a = a;
	}

	static bool is_diffuse_sampler(unsigned int hash)
	{
		if(hash == HASH("DiffuseSampler") || hash == HASH("DiffuseSampler2") || hash == HASH("DiffuseSamplerPhase2")
			|| hash == HASH("DiffuseSamplerPoint") || hash == HASH("DiffuseTexSampler") || hash == HASH("DiffuseTexSampler01")
			|| hash == HASH("DiffuseTexSampler02") || hash == HASH("DiffuseTexSampler03") || hash == HASH("DiffuseTexSampler04")
			|| hash == HASH("gDiffuse") || hash == HASH("DiffuseTexSampler03") || hash == HASH("DiffuseTexSampler04")
			|| hash == HASH("TextureGrassSampler") || hash == HASH("TextureNoWrapSampler") || hash == HASH("textureSamp")
			|| hash == HASH("TextureSampler") || hash == HASH("TextureSampler_layer0") || hash == HASH("TextureSampler2")
			|| hash == HASH("BaseSampler") || hash == HASH("baseTextureSampler"))
		{
			return true;
		}
		return false;
	}

	static bool is_normalmap_sampler(unsigned int hash)
	{
		if(hash == HASH("NormalMapSampler") || hash == HASH("NormalMapSampler1") || hash == HASH("NormalMapSampler2")
			|| hash == HASH("NormalMapTexSampler") || hash == HASH("NormalSampler") || hash == HASH("NormalTextureSampler")
			|| hash == HASH("BumpSampler") || hash == HASH("BumpSampler_layer0") || hash == HASH("BumpSampler2"))
		{
			return true;
		}
		return false;
	}

	static void scale_vector(gtaRwV3d &v)
	{
		v.x *= settings.m_fScaleMultiplier;
		v.y *= settings.m_fScaleMultiplier;
		v.z *= settings.m_fScaleMultiplier;
	}

	static bool models_to_geometry(vector<Model *>& models, vector<int>* parentBones, ShaderGroup& shaderGroup, gtaRwGeometry &geometry, skeleton_info &skelInfo, bool generateBreakable)
	{
		// Собираем информацию о геометрии...
		bool isSkinned = false;
		bool hasPositions = false;
		bool hasNormals = false;
		bool hasColors = false;
		unsigned char numTexCoords = 0;
		unsigned int totalNumIndex = 0;
		unsigned int totalNumVertex = 0;
		unsigned int totalGeometries = 0;

		for(int m = 0; m < models.size(); m++)
		{
			if(models[m]->m_geometries.m_pData)
			{
				if(models[m]->m_bIsSkinned)
					isSkinned = true;
				for(int g = 0; g < models[m]->m_geometries.m_wSize; g++)
				{
					Geometry &rageGeom = *models[m]->m_geometries.m_pData[g];
					VertexDeclaration &geomVertexDesclaration = *rageGeom.m_pVertexBuffer[0]->m_pDeclaration;
					if(geomVertexDesclaration.m_usedElements.m_bPosition)
						hasPositions = true;
					if(geomVertexDesclaration.m_usedElements.m_bNormal)
						hasNormals = true;
					if(geomVertexDesclaration.m_usedElements.m_bColor)
						hasColors = true;
					unsigned char maxNumTexCoords = 0;
					if(geomVertexDesclaration.m_usedElements.m_bTexCoord8)
						maxNumTexCoords = 8;
					else if(geomVertexDesclaration.m_usedElements.m_bTexCoord7)
						maxNumTexCoords = 7;
					else if(geomVertexDesclaration.m_usedElements.m_bTexCoord6)
						maxNumTexCoords = 6;
					else if(geomVertexDesclaration.m_usedElements.m_bTexCoord5)
						maxNumTexCoords = 5;
					else if(geomVertexDesclaration.m_usedElements.m_bTexCoord4)
						maxNumTexCoords = 4;
					else if(geomVertexDesclaration.m_usedElements.m_bTexCoord3)
						maxNumTexCoords = 3;
					else if(geomVertexDesclaration.m_usedElements.m_bTexCoord2)
						maxNumTexCoords = 2;
					else if(geomVertexDesclaration.m_usedElements.m_bTexCoord1)
						maxNumTexCoords = 1;
					if(maxNumTexCoords > numTexCoords)
						numTexCoords = maxNumTexCoords;
					totalNumIndex += rageGeom.m_dwIndexCount;
					totalNumVertex += rageGeom.m_wVertexCount;
					totalGeometries++;
				}
			}
		}
		// Генерируем флаги геометрии
		unsigned int format = rpGEOMETRYMODULATEMATERIALCOLOR;
		if(hasPositions)
			format |= rpGEOMETRYPOSITIONS;
		if(hasNormals)
			format |= rpGEOMETRYNORMALS;
		if(hasColors)
			format |= rpGEOMETRYPRELIT;
		if(numTexCoords == 1)
			format |= rpGEOMETRYTEXTURED;
		else if(numTexCoords >= 2)
		{
			format |= rpGEOMETRYTEXTURED;
			format |= rpGEOMETRYTEXTURED2;
		}

		geometry.Initialise(totalNumIndex / 3, totalNumVertex, 1, format, numTexCoords, 0.0f, 0.0f, 0.0f, 0.0f);
		geometry.Extension.mesh.Initialise(0, totalGeometries, totalNumIndex);

		if(parentBones)
		{
			for(int i = 0; i < parentBones->size(); i++)
			{
				if((*parentBones)[i] != -1)
				{
					isSkinned = true;
					break;
				}
			}
		}
		if(!skelInfo.exportSkeleton || !skelInfo.skeleton)
			isSkinned = false;
		if(isSkinned)
			geometry.Extension.skin.Initialise(skelInfo.skeleton->m_wBoneCount, 0, totalNumVertex, 4);
		if(generateBreakable)
			geometry.Extension.breakable.Initialise(totalNumVertex, totalNumIndex / 3, totalGeometries, false);
		
		// Записываем материалы
		if(shaderGroup.m_shaders.m_pData && shaderGroup.m_shaders.m_wCount > 0)
		{
			geometry.matList.Initialise(shaderGroup.m_shaders.m_wCount);
			for(int mat = 0; mat < shaderGroup.m_shaders.m_wCount; mat++)
			{
				Shader &shader = *shaderGroup.m_shaders.m_pData[mat];
				DWORD *hashes = (DWORD *)((DWORD)shader.m_pParams + shader.m_wParamsDataSize);
				// Собираем информацию для материала
				material_info matInfo;
				for(int param = 0; param < shader.m_nParamsCount; param++)
				{
					if(shader.m_pParams[param].m_pData)
					{
						// Текстура
						if(!shader.m_pParams[param].m_nDataType)
						{
							TextureDesc *texture = (TextureDesc *)shader.m_pParams[param].m_pData;
                            //MessageBox(0, texture->m_pszName, "At texture iter", 0);
							// Диффуз
							if(!matInfo.pDiffuse && is_diffuse_sampler(hashes[param]))
							{
								if(texture->m_pszName && texture->m_pszName[0] != '\0')
									matInfo.pDiffuse = &shader.m_pParams[param];
							}
							// Нормал
							if(!matInfo.pNormal && is_normalmap_sampler(hashes[param]))
							{
								if(texture->m_pszName && texture->m_pszName[0] != '\0')
									matInfo.pNormal = &shader.m_pParams[param];
							}
						}
						else
						{
							// Цвет
							if(!matInfo.pCarCol && hashes[param] == HASH("matDiffuseColor"))
							{
								Vector4 *carColor = (Vector4 *)shader.m_pParams[param].m_pData;
								if(carColor->x == 2.0f)
									matInfo.pCarCol = &shader.m_pParams[param];
							}
							// Свечение
							else if(!matInfo.pEmissive && hashes[param] == HASH("emissiveMultiplier"))
							{
								matInfo.pEmissive = &shader.m_pParams[param];
							}
						}
					}
				}
				if(matInfo.pDiffuse)
				{
					TextureDesc *texture = (TextureDesc *)matInfo.pDiffuse->m_pData;
					char textureName[32];
					strncpy(textureName, texture->m_pszName, 32);
                    //MessageBox(0, texture->m_pszName, "At material init", 0);
					geometry.matList.materials[mat].Initialise(255, 255, 255, 255, true, settings.m_fMaterialAmbient, settings.m_fMaterialDiffuse);
					if(settings.m_bExtendTextureName)
						textureName[63] = '\0';
					else
						textureName[31] = '\0';
					geometry.matList.materials[mat].texture.Initialise(rwFILTERLINEARMIPLINEAR, rwTEXTUREADDRESSWRAP, rwTEXTUREADDRESSWRAP, false, 
						textureName, NULL);
				}
				else
					geometry.matList.materials[mat].Initialise(255, 255, 255, 255, false, settings.m_fMaterialAmbient, settings.m_fMaterialDiffuse);
				if(matInfo.pCarCol)
				{
					Vector4 *carColor = (Vector4 *)matInfo.pCarCol->m_pData;
					if(carColor->y == 1.0f)
						set_mat_color(geometry.matList.materials[mat], 60, 255, 0, 255);
					else if(carColor->y == 2.0f)
						set_mat_color(geometry.matList.materials[mat], 255, 0, 175, 255);
					else if(carColor->y == 3.0f)
						set_mat_color(geometry.matList.materials[mat], 0, 255, 255, 255);
					else if(carColor->y == 4.0f)
						set_mat_color(geometry.matList.materials[mat], 255, 0, 255, 255);
				}
				if(settings.m_bExportNormalMap && matInfo.pNormal)
				{
                    TextureDesc *texture = (TextureDesc *)matInfo.pNormal->m_pData;
					char textureName[32];
					strncpy(textureName, texture->m_pszName, 32);
					geometry.matList.materials[mat].Extension.normalMap.Initialise(true, false, 0.0f, false);
					if(settings.m_bExtendTextureName)
						textureName[63] = '\0';
					else
						textureName[31] = '\0';
					geometry.matList.materials[mat].Extension.normalMap.normalMapTexture.Initialise(rwFILTERLINEARMIPLINEAR, rwTEXTUREADDRESSWRAP,
						rwTEXTUREADDRESSWRAP, false, textureName, NULL);
				}
				if(matInfo.pEmissive)
				{
					if(settings.m_bGenerateNightPrelight)
					{
						if(shader.m_dwNameHash == HASH("emissivenight") || shader.m_dwNameHash == HASH("emissivenight_alpha")
							|| shader.m_dwNameHash == HASH("glass_emissivenight") || shader.m_dwNameHash == HASH("glass_emissivenight_alpha")
							|| shader.m_dwNameHash == HASH("gta_emissivenight") || shader.m_dwNameHash == HASH("gta_emissivenight_alpha")
							|| shader.m_dwNameHash == HASH("gta_glass_emissivenight") || shader.m_dwNameHash == HASH("gta_glass_emissivenight_alpha")
							|| shader.m_dwNameHash == HASH("decal_emissivenight_only")
							|| shader.m_dwNameHash == HASH("gta_normal_spec_reflect_emissivenight") || shader.m_dwNameHash == HASH("gta_normal_spec_reflect_emissivenight_alpha")
							|| shader.m_dwNameHash == HASH("normal_spec_reflect_emissivenight") || shader.m_dwNameHash == HASH("normal_spec_reflect_emissivenight_alpha"))
						{
							geometry.matList.materials[mat].flags = MATERIAL_EMISSIVENIGHT;
							geometry.matList.materials[mat].surfaceProps.specular = *(float *)matInfo.pEmissive->m_pData;
						}
					}
					if(settings.m_bGenerateDayPrelight)
					{
						if(shader.m_dwNameHash == HASH("emissive") || shader.m_dwNameHash == HASH("emissive_additive_alpha")
							|| shader.m_dwNameHash == HASH("emissive_alpha") || shader.m_dwNameHash == HASH("emissive_alpha_tnt")
							|| shader.m_dwNameHash == HASH("emissive_speclum") || shader.m_dwNameHash == HASH("emissive_tnt")
							|| shader.m_dwNameHash == HASH("emissivestrong") || shader.m_dwNameHash == HASH("emissivestrong_alpha")
							|| shader.m_dwNameHash == HASH("glass_emissive") || shader.m_dwNameHash == HASH("glass_emissive_alpha")
							|| shader.m_dwNameHash == HASH("gta_emissive") || shader.m_dwNameHash == HASH("gta_emissive_alpha")
							|| shader.m_dwNameHash == HASH("gta_emissivestrong") || shader.m_dwNameHash == HASH("gta_emissivestrong_alpha")
							|| shader.m_dwNameHash == HASH("gta_glass_emissive") || shader.m_dwNameHash == HASH("gta_glass_emissive_alpha")
							|| shader.m_dwNameHash == HASH("normal_spec_emissive"))
						{
							geometry.matList.materials[mat].flags = MATERIAL_EMISSIVE|MATERIAL_EMISSIVENIGHT;
							geometry.matList.materials[mat].surfaceProps.specular = *(float *)matInfo.pEmissive->m_pData;
						}
					}
				}
				if(settings.m_bExportAsVehicle)
				{
					if(shader.m_dwNameHash == HASH("gta_vehicle_lightsemissive") || shader.m_dwNameHash == HASH("vehicle_lightsemissive"))
						geometry.matList.materials[mat].flags |= MATERIAL_VEHICLELIGHT;
				}
			}
		}
		else
		{
			geometry.matList.Initialise(1);
			geometry.matList.materials[0].Initialise(255, 255, 255, 255, false, settings.m_fMaterialAmbient, settings.m_fMaterialDiffuse);
		}

		// Собираем меш
		unsigned int vertCounter = 0;
		unsigned int indexCount = 0;
		unsigned int meshCounter = 0;
		unsigned int triCount = 0;
		
		for(int m = 0; m < models.size(); m++)
		{
			if(models[m]->m_geometries.m_pData)
			{
				int parentBone = -1;
				if(parentBones)
					parentBone = (*parentBones)[m];
				for(int g = 0; g < models[m]->m_geometries.m_wSize; g++)
				{
					Geometry &rageGeom = *models[m]->m_geometries.m_pData[g];
					VertexDeclaration &geomVertexDesclaration = *rageGeom.m_pVertexBuffer[0]->m_pDeclaration;
					// Меш
					geometry.Extension.mesh.meshes[meshCounter].Initialise(rageGeom.m_dwIndexCount, models[m]->m_pShaderMapping[g]);
					for(int i = 0; i < rageGeom.m_dwIndexCount; i++)
						geometry.Extension.mesh.meshes[meshCounter].indices[i] = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i] + indexCount;
					
					// Добавляем треугольники
					for(int i = 0; i < (rageGeom.m_dwIndexCount / 3); i++)
					{
						geometry.triangles[triCount].vertA = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3] + indexCount;
						geometry.triangles[triCount].vertB = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3 + 1] + indexCount;
						geometry.triangles[triCount].vertC = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3 + 2] + indexCount;
						geometry.triangles[triCount].mtlId = models[m]->m_pShaderMapping[g];

						if(generateBreakable)
						{
							geometry.Extension.breakable.faces[triCount].a = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3] + indexCount;
							geometry.Extension.breakable.faces[triCount].b = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3 + 1] + indexCount;
							geometry.Extension.breakable.faces[triCount].c = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3 + 2] + indexCount;
							geometry.Extension.breakable.faceMaterials[triCount] = meshCounter;
						}
						triCount++;
					}
		
					indexCount += rageGeom.m_wVertexCount;

					if(generateBreakable)
					{
						gtaRwMaterial &mat = geometry.matList.materials[models[m]->m_pShaderMapping[g]];
						geometry.Extension.breakable.matColors[meshCounter].red = settings.m_fBrokenLighting;
						geometry.Extension.breakable.matColors[meshCounter].green = settings.m_fBrokenLighting;
						geometry.Extension.breakable.matColors[meshCounter].blue = settings.m_fBrokenLighting;
						if(mat.textured && mat.texture.name.string)
						{
							geometry.Extension.breakable.texMaskNames[meshCounter][0] = '\0';
							if(strlen(mat.texture.name.string) > 31)
							{
								strncpy(geometry.Extension.breakable.texNames[meshCounter], mat.texture.name.string, 31);
								geometry.Extension.breakable.texNames[meshCounter][31] = '\0';
								strcpy(geometry.Extension.breakable.texMaskNames[meshCounter], &mat.texture.name.string[31]);
							}
							else
							{
								strcpy(geometry.Extension.breakable.texNames[meshCounter], mat.texture.name.string);
								geometry.Extension.breakable.texMaskNames[meshCounter][0] = '\0';
							}
						}
						else
						{
							geometry.Extension.breakable.texNames[meshCounter][0] = '\0';
							geometry.Extension.breakable.texMaskNames[meshCounter][0] = '\0';
						}
					}
					
					unsigned int gElementSizes[] = {2, 4, 6, 8, 4, 8, 12, 16, 4, 4, 4, 0, 0, 0, 0, 0};
					
					if(!geomVertexDesclaration.m_bStoreNormalsDataFirst)
					{
						for(int j = 0; j < rageGeom.m_wVertexCount; j++)
						{
							BYTE *vertex = &rageGeom.m_pVertexBuffer[0]->m_pVertexData[j * geomVertexDesclaration.m_nTotalSize];
							unsigned int offset = 0;
							if(geomVertexDesclaration.m_usedElements.m_bPosition)
							{
								if(parentBone != -1)
								{
									Vector3 res, pos;
									memcpy(&pos, &vertex[offset], 12);
									res.TransformPosition(&pos, &skelInfo.matrices[parentBone]);
									memcpy(&geometry.morphTarget[0].verts[vertCounter + j], &res, 12);
								}
								else
								{
									memcpy(&geometry.morphTarget[0].verts[vertCounter + j], &vertex[offset], 12);
								}
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nPositionType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bBlendWeight)
							{
								if(isSkinned)
								{
									gtaRwRGBA *color = (gtaRwRGBA *)&vertex[offset];
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w0 = (float)color->r / 255.0f;
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w1 = (float)color->g / 255.0f;
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w2 = (float)color->b / 255.0f;
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w3 = (float)color->a / 255.0f;
								}
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nBlendWeightType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bBlendIndices)
							{
								if(isSkinned)
								{
									gtaRwRGBA *color = (gtaRwRGBA *)&vertex[offset];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i0 = rageGeom.m_pBoneMapping[color->r];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i1 = rageGeom.m_pBoneMapping[color->g];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i2 = rageGeom.m_pBoneMapping[color->b];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i3 = rageGeom.m_pBoneMapping[color->a];
								}
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nBlendIndicesType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bNormal)
							{
								memcpy(&geometry.morphTarget[0].normals[vertCounter + j], &vertex[offset], 12);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nNormalType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bColor)
							{
								memcpy(&geometry.preLitLum[vertCounter + j], &vertex[offset], 4);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nColorType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bSpecularColor)
							{
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nSpecularColorType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord1)
							{
								memcpy(&geometry.texCoords[0][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord1Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord2)
							{
								memcpy(&geometry.texCoords[1][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord2Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord3)
							{
								memcpy(&geometry.texCoords[2][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord3Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord4)
							{
								memcpy(&geometry.texCoords[3][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord4Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord5)
							{
								memcpy(&geometry.texCoords[4][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord5Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord6)
							{
								memcpy(&geometry.texCoords[5][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord6Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord7)
							{
								memcpy(&geometry.texCoords[6][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord7Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord8)
							{
								memcpy(&geometry.texCoords[7][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord8Type];
							}
							if(isSkinned && !models[m]->m_bIsSkinned && parentBone != -1)
							{
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w0 = 1.0f;
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w1 = 0.0f;
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w2 = 0.0f;
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w3 = 0.0f;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i0 = parentBone;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i1 = 0;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i2 = 0;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i3 = 0;
							}
						}
					}
					else
					{
						for(int j = 0; j < rageGeom.m_wVertexCount; j++)
						{
							BYTE *vertex = &rageGeom.m_pVertexBuffer[0]->m_pVertexData[j * geomVertexDesclaration.m_nTotalSize];
							unsigned int offset = 0;
							if(geomVertexDesclaration.m_usedElements.m_bPosition)
							{
								if(parentBone != -1)
								{
									Vector3 res, pos;
									memcpy(&pos, &vertex[offset], 12);
									res.TransformPosition(&pos, &skelInfo.matrices[parentBone]);
									memcpy(&geometry.morphTarget[0].verts[vertCounter + j], &res, 12);
								}
								else
								{
									memcpy(&geometry.morphTarget[0].verts[vertCounter + j], &vertex[offset], 12);
								}
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nPositionType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bNormal)
							{
								memcpy(&geometry.morphTarget[0].normals[vertCounter + j], &vertex[offset], 12);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nNormalType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTangent)
							{
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTangentType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTangent)
							{
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTangentType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord1)
							{
								memcpy(&geometry.texCoords[0][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord1Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord2)
							{
								memcpy(&geometry.texCoords[1][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord2Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord3)
							{
								memcpy(&geometry.texCoords[2][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord3Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord4)
							{
								memcpy(&geometry.texCoords[3][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord4Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord5)
							{
								memcpy(&geometry.texCoords[4][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord5Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord6)
							{
								memcpy(&geometry.texCoords[5][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord6Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord7)
							{
								memcpy(&geometry.texCoords[6][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord7Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bTexCoord8)
							{
								memcpy(&geometry.texCoords[7][vertCounter + j], &vertex[offset], 8);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nTexCoord8Type];
							}
							if(geomVertexDesclaration.m_usedElements.m_bBlendWeight)
							{
								if(isSkinned)
								{
									gtaRwRGBA *color = (gtaRwRGBA *)&vertex[offset];
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w0 = (float)color->r / 255.0f;
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w1 = (float)color->g / 255.0f;
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w2 = (float)color->b / 255.0f;
									geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w3 = (float)color->a / 255.0f;
								}
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nBlendWeightType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bBlendIndices)
							{
								if(isSkinned)
								{
									gtaRwRGBA *color = (gtaRwRGBA *)&vertex[offset];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i0 = rageGeom.m_pBoneMapping[color->r];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i1 = rageGeom.m_pBoneMapping[color->g];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i2 = rageGeom.m_pBoneMapping[color->b];
									geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i3 = rageGeom.m_pBoneMapping[color->a];
								}
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nBlendIndicesType];
							}
							if(geomVertexDesclaration.m_usedElements.m_bColor)
							{
								memcpy(&geometry.preLitLum[vertCounter + j], &vertex[offset], 4);
								offset += gElementSizes[geomVertexDesclaration.m_elementTypes.m_nColorType];
							}
							if(isSkinned && !models[m]->m_bIsSkinned && parentBone != -1)
							{
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w0 = 1.0f;
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w1 = 0.0f;
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w2 = 0.0f;
								geometry.Extension.skin.vertexBoneWeights[vertCounter + j].w3 = 0.0f;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i0 = parentBone;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i1 = 0;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i2 = 0;
								geometry.Extension.skin.vertexBoneIndices[vertCounter + j].i3 = 0;
							}
						}
					}
					vertCounter += rageGeom.m_wVertexCount;
					meshCounter++;
				}
			}
		}

		// Обработка скина
		if(isSkinned)
		{
			gtaRwGeometrySkin &skin = geometry.Extension.skin;
			memcpy(skin.skinToBoneMatrices, skelInfo.skeleton->m_apTransformInverted, sizeof(D3DMATRIX) * skelInfo.skeleton->m_wBoneCount);
			for(int i = 0; i < geometry.numVertices; i++)
			{
				unsigned int order[4];
				if(skin.vertexBoneWeights[i].aw[0] > skin.vertexBoneWeights[i].aw[1])
				{
					order[0] = 0;
					order[1] = 1;
				}
				else
				{
					order[0] = 1;
					order[1] = 0;
				}
				if(skin.vertexBoneWeights[i].aw[2] > skin.vertexBoneWeights[i].aw[order[0]])
				{
					order[2] = order[1];
					order[1] = order[0];
					order[0] = 2;
				}
				else if(skin.vertexBoneWeights[i].aw[2] > skin.vertexBoneWeights[i].aw[order[1]])
				{
					order[2] = order[1];
					order[1] = 2;
				}
				else
					order[2] = 2;
				if(skin.vertexBoneWeights[i].aw[3] > skin.vertexBoneWeights[i].aw[order[0]])
				{
					order[3] = order[2];
					order[2] = order[1];
					order[1] = order[0];
					order[0] = 3;
				}
				else if(skin.vertexBoneWeights[i].aw[3] > skin.vertexBoneWeights[i].aw[order[1]])
				{
					order[3] = order[2];
					order[2] = order[1];
					order[1] = 3;
				}
				else if(skin.vertexBoneWeights[i].aw[3] > skin.vertexBoneWeights[i].aw[order[2]])
				{
					order[3] = order[2];
					order[2] = 3;
				}
				else
					order[3] = 3;
				gtaRwBoneWeights weights;
				memcpy(&weights, &skin.vertexBoneWeights[i], sizeof(gtaRwBoneWeights));
				gtaRwBoneIndices indices;
				memcpy(&indices, &skin.vertexBoneIndices[i], sizeof(gtaRwBoneIndices));
				for(int j = 0; j < 4; j++)
				{
					skin.vertexBoneWeights[i].aw[j] = weights.aw[order[j]];
					skin.vertexBoneIndices[i].ai[j] = indices.ai[order[j]];
				}
			}
			geometry.Extension.skin.FindUsedBoneIds(geometry.numVertices, skelInfo.skeleton->m_wBoneCount);
		}
		return true;
	}

	static bool models_to_collision(vector<Model *>& models, vector<int>* parentBones, D3DMATRIX *matrices, ShaderGroup& shaderGroup, ColFile &colFile)
	{
		unsigned int totalNumIndex = 0;
		unsigned int totalGeometries = 0;
		unsigned int totalNumVertex = 0;

		for(int m = 0; m < models.size(); m++)
		{
			if(models[m]->m_geometries.m_pData)
			{
				for(int g = 0; g < models[m]->m_geometries.m_wSize; g++)
				{
					Geometry &rageGeom = *models[m]->m_geometries.m_pData[g];
					totalNumIndex += rageGeom.m_dwIndexCount;
					totalNumVertex += rageGeom.m_wVertexCount;
					totalGeometries++;
				}
			}
		}

		if(totalNumVertex > 32767)
			return false;
		colFile.InitStatic(totalNumIndex / 3, totalNumVertex);
		
		unsigned int vertCounter = 0;
		unsigned int indexCount = 0;
		unsigned int meshCounter = 0;
		unsigned int triCount = 0;
		
		for(int m = 0; m < models.size(); m++)
		{
			if(models[m]->m_geometries.m_pData)
			{
				int parentBone = -1;
				if(parentBones)
					parentBone = (*parentBones)[m];
				for(int g = 0; g < models[m]->m_geometries.m_wSize; g++)
				{
					Geometry &rageGeom = *models[m]->m_geometries.m_pData[g];

					// Находим информацию о материале геометрии
					unsigned int matHash = 0;
					vector<char *> texNames;
					char *texName = NULL;
					if(shaderGroup.m_shaders.m_pData && shaderGroup.m_shaders.m_wCount > 0)
					{
						Shader &shader = *shaderGroup.m_shaders.m_pData[models[m]->m_pShaderMapping[g]];
						DWORD *hashes = (DWORD *)((DWORD)shader.m_pParams + shader.m_wParamsDataSize);
						matHash = shader.m_dwNameHash;
						for(int param = 0; param < shader.m_nParamsCount; param++)
						{
							if(!shader.m_pParams[param].m_nDataType && shader.m_pParams[param].m_pData)
							{
                                TextureDesc *texture = (TextureDesc *)shader.m_pParams[param].m_pData;
								if(!shader.m_pParams[param].m_nType)
									texName = texture->m_pszName;
								else
									texNames.push_back(texture->m_pszName);
							}
						}
					}
					
					// Пропускаем ненужные меши
					if(colMats.skip_material(matHash, texName))
						continue;

					unsigned int matId = colMats.match_material(matHash, texName);

					if(!matId)
					{
						for(int l = 0; l < texNames.size(); l++)
						{
							matId = colMats.match_material(matHash, texNames[l]);
							if(matId)
								break;
						}
					}

					VertexDeclaration &geomVertexDesclaration = *rageGeom.m_pVertexBuffer[0]->m_pDeclaration;

					// Записываем треугольники
					for(int i = 0; i < (rageGeom.m_dwIndexCount / 3); i++)
					{
						colFile.col3.faces[triCount].b = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3] + indexCount;
						colFile.col3.faces[triCount].a = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3 + 1] + indexCount;
						colFile.col3.faces[triCount].c = rageGeom.m_pIndexBuffer[0]->m_pIndexData[i * 3 + 2] + indexCount;
						colFile.col3.faces[triCount].material = matId;
						colFile.col3.faces[triCount].light = settings.m_nCollisionModelLighting;
						triCount++;
					}
		
					indexCount += rageGeom.m_wVertexCount;
					
					for(int j = 0; j < rageGeom.m_wVertexCount; j++)
					{
						Vector3 *vertex = (Vector3 *)&rageGeom.m_pVertexBuffer[0]->m_pVertexData[j * geomVertexDesclaration.m_nTotalSize];
						if(geomVertexDesclaration.m_usedElements.m_bPosition)
						{
							if(parentBone != -1)
							{
								Vector3 res, pos;
								memcpy(&pos, vertex, 12);
								res.TransformPosition(&pos, &matrices[parentBone]);
								colFile.col3.vertices[vertCounter + j].x = res.x * settings.m_fScaleMultiplier * 128.0f;
								colFile.col3.vertices[vertCounter + j].y = res.y * settings.m_fScaleMultiplier * 128.0f;
								colFile.col3.vertices[vertCounter + j].z = res.z * settings.m_fScaleMultiplier * 128.0f;
							}
							else
							{
								colFile.col3.vertices[vertCounter + j].x = vertex->x * settings.m_fScaleMultiplier * 128.0f;
								colFile.col3.vertices[vertCounter + j].y = vertex->y * settings.m_fScaleMultiplier * 128.0f;
								colFile.col3.vertices[vertCounter + j].z = vertex->z * settings.m_fScaleMultiplier * 128.0f;
							}
						}
					}
					vertCounter += rageGeom.m_wVertexCount;
					meshCounter++;
				}
			}
		}

		colFile.col3.numFaces = triCount;
		colFile.numVertices = vertCounter;

		return true;
	}

	static bool bone_has_childs(unsigned int boneIndex, Bone *bones, unsigned int numBones)
	{
		for(int i = 0; i < numBones; i++)
		{
			if(bones[i].m_wParentBoneId == boneIndex)
				return true;
		}
		return false;
	}

	static bool bone_not_last_on_hierarchy_level(unsigned int boneIndex, Bone *bones, unsigned int numBones)
	{
		if(boneIndex == (numBones - 1))
			return false;
		for(int i = boneIndex + 1; i < numBones; i++)
		{
			if(bones[boneIndex].m_wParentBoneId == bones[i].m_wParentBoneId)
				return false;
		}
		return true;
	}

	static unsigned int bone_get_flags(unsigned int boneIndex, Bone *bones, unsigned int numBones)
	{
		unsigned int flags = 0;
		if(!bone_has_childs(boneIndex, bones, numBones))
			flags |= rpHANIMPOPPARENTMATRIX;
		if(bone_not_last_on_hierarchy_level(boneIndex, bones, numBones))
			flags |= rpHANIMPUSHPARENTMATRIX;
		return flags;
	}

	static unsigned int get_last_available_lod_level(Drawable *drawable, unsigned int baseLevel)
	{
		unsigned int maxLodLevel = baseLevel;
		for(int i = baseLevel; i > 0; i--)
		{
			if(drawable->m_pModelCollection[i] && drawable->m_pModelCollection[i]->m_wCount)
				return i;
		}
		return 0;
	}

	static unsigned int get_last_available_lod_level(FragDrawable *drawable, unsigned int baseLevel)
	{
		unsigned int maxLodLevel = baseLevel;
		for(int i = baseLevel; i > 0; i--)
		{
			if(drawable->m_pModelCollection[i] && drawable->m_pModelCollection[i]->m_wCount)
				return i;
		}
		return 0;
	}

	static bool drawable_to_dff(Drawable *drawable, char *dstpath, char *modelname)
	{
		gtaRwClump clump;
		skeleton_info skelInfo;
		skelInfo.skeleton = drawable->m_pSkeleton;
		if(settings.m_bExportSkeleton && drawable->m_pSkeleton && drawable->m_pSkeleton->m_apBones && drawable->m_pSkeleton->m_wBoneCount > 0)
		{
			skelInfo.exportSkeleton = true;
			clump.Initialise(1, drawable->m_pSkeleton->m_wBoneCount + 1, 1);
			clump.frameList.frames[0].Initialise(-1, 0);
			for(int i = 0; i < drawable->m_pSkeleton->m_wBoneCount; i++)
			{
				clump.frameList.frames[i + 1].Initialise((gtaRwV3d *)&drawable->m_pSkeleton->m_apTransform[i]._11, 
					(gtaRwV3d *)&drawable->m_pSkeleton->m_apTransform[i]._21, 
					(gtaRwV3d *)&drawable->m_pSkeleton->m_apTransform[i]._31, (gtaRwV3d *)&drawable->m_pSkeleton->m_apTransform[i]._41, 
					drawable->m_pSkeleton->m_apBones[i].m_wParentBoneId + 1, 0);
				char boneName[24];
				if(drawable->m_pSkeleton->m_apBones[i].m_pszName)
				{
					strncpy(boneName, drawable->m_pSkeleton->m_apBones[i].m_pszName, 24);
					boneName[23] = '\0';
					clump.frameList.frames[i + 1].Extension.nodeName.Initialise(boneName);
				}
				else
				{
					sprintf(boneName, "bone_%d", i);
					clump.frameList.frames[i + 1].Extension.nodeName.Initialise(boneName);
				}
				if(i == 0)
				{
					gtaRwFrameHAnim &hAnim = clump.frameList.frames[i + 1].Extension.hAnim;
					hAnim.Initialise(drawable->m_pSkeleton->m_apBones[i].m_wBoneIndex, drawable->m_pSkeleton->m_wBoneCount, 0, 36);
					for(int j = 0; j < hAnim.numNodes; j++)
					{
						hAnim.nodes[j].Initialise(drawable->m_pSkeleton->m_apBones[j].m_wBoneIndex, j, bone_get_flags(j, drawable->m_pSkeleton->m_apBones, 
							drawable->m_pSkeleton->m_wBoneCount));
					}
				}
				else
					clump.frameList.frames[i + 1].Extension.hAnim.Initialise(drawable->m_pSkeleton->m_apBones[i].m_wBoneIndex);
			}
		}
		else
		{
			clump.Initialise(1, 1, 1);
			clump.frameList.frames[0].Initialise(-1, 0);
			clump.frameList.frames[0].Extension.nodeName.Initialise(modelname);
		}
		clump.atomics[0].Initialise(0, 0, rpATOMICCOLLISIONTEST|rpATOMICRENDER, false);
		vector<Model *> models;
		unsigned int level = get_last_available_lod_level(drawable, settings.m_nLodLevel);
		if(drawable->m_pModelCollection[level] && drawable->m_pModelCollection[level]->m_pData)
		{
			for(int i = 0; i < drawable->m_pModelCollection[level]->m_wCount; i++)
			{
				if(drawable->m_pModelCollection[level]->m_pData[i])
					models.push_back(drawable->m_pModelCollection[level]->m_pData[i]);
			}
		}
		
		models_to_geometry(models, NULL, *drawable->m_pShaderGroup, clump.geometryList.geometries[0], skelInfo, false);
		BoundSphere sphere;
		BoundBox bbox;
		bbox.Set(Vector3(drawable->m_vAabbMin), Vector3(drawable->m_vAabbMax));
		bbox.ToSphere(&sphere);
		memcpy(&clump.geometryList.geometries[0].morphTarget[0].boundingSphere.center, &sphere.center, 12);
		clump.geometryList.geometries[0].morphTarget[0].boundingSphere.radius = sphere.radius;
		add_lights_to_geometry(drawable->m_lights, NULL, NULL, clump.geometryList.geometries[0]);
		clump_post_process(&clump, settings.m_bGenerateDayPrelight, settings.m_bGenerateNightPrelight);
		if(clump.geometryList.geometries[0].numVertices >= 65535)
			LOGL("  WARNING: Mesh has more than 65535 vertices (%u). Edit this model manually to prevent crashes in-game.", clump.geometryList.geometries[0].numVertices);
		gtaRwStream *stream = gtaRwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, dstpath);
		bool result = false;
		if(stream)
		{
			clump.StreamWrite(stream);
			gtaRwStreamClose(stream);
			result = true;
		}
		clump.Destroy();

		if(settings.m_bGenerateCollision)
		{
			char colpath[MAX_PATH];
			strcpy(colpath, dstpath);
			strcpy(&colpath[strlen(colpath) - 3], "col");
			ColFile colFile;
			colFile.SetModelInfo(modelname, -1);
			models.clear();
			unsigned int maxLodLevel = get_last_available_lod_level(drawable, 3);
			if(drawable->m_pModelCollection[maxLodLevel] && drawable->m_pModelCollection[maxLodLevel]->m_pData)
			{
				for(int i = 0; i < drawable->m_pModelCollection[maxLodLevel]->m_wCount; i++)
				{
					if(drawable->m_pModelCollection[maxLodLevel]->m_pData[i])
						models.push_back(drawable->m_pModelCollection[maxLodLevel]->m_pData[i]);
				}
			}
			if(!models_to_collision(models, NULL, NULL, *drawable->m_pShaderGroup, colFile))
				LOGL("  WARNING: This model has more than 32767 vertices. I will generate dummy (empty) collision model.");
			else
			{
				if(settings.m_bOptimizeCollisionMesh)
					colFile.Optimize();
				else
				{
					colFile.col3.min.x = bbox.aabbMin.x;
					colFile.col3.min.y = bbox.aabbMin.y;
					colFile.col3.min.z = bbox.aabbMin.z;
					colFile.col3.max.x = bbox.aabbMax.x;
					colFile.col3.max.y = bbox.aabbMax.y;
					colFile.col3.max.z = bbox.aabbMax.z;
					colFile.col3.center.x = sphere.center.x;
					colFile.col3.center.y = sphere.center.y;
					colFile.col3.center.z = sphere.center.z;
					colFile.col3.radius = sphere.radius;
				}
				if(colFile.numVertices > 32767)
				{
					if(settings.m_bSkipLargeCol)
					{
						LOGL("  WARNING: This model has more than 32767 vertices. I will generate dummy (empty) collision model.");
						colFile.Clear();
					}
					else
						LOGL("  WARNING: This model has more than 32767 vertices. You must fix it manually.");
				}
				else if(colFile.col3.numFaces > 32767)
				{
					if(settings.m_bSkipLargeCol)
					{
						LOGL("  WARNING: This model has more than 32767 faces. I will delete some faces so it will be a valid colmodel.");
						colFile.col3.numFaces = 32767;
					}
					else
						LOGL("  WARNING: This model has more than 32767 faces. You must fix it manually.");
				}
			}
			colFile.Write(colpath);
		}

		if(drawable->m_pShaderGroup && drawable->m_pShaderGroup->m_pTxd)
		{
			gtaRwTexDictionary txd;
			texture_converter::dictionary_to_txd(drawable->m_pShaderGroup->m_pTxd, &txd);
			char txdpath[MAX_PATH];
			strcpy(txdpath, dstpath);
			if(strrchr(txdpath, '.'))
				strrchr(txdpath, '.')[1] = '\0';
			strcat(txdpath, "internal.txd");
			gtaRwStream *txdstream = gtaRwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, txdpath);
			if(txdstream)
			{
				txd.StreamWrite(txdstream);
				gtaRwStreamClose(txdstream);
			}
		}
		return result;
	}

	static bool convert_ydr_to_dff(char *srcpath, char *dstpath, char *modelname)
	{
		ResourceData resData(srcpath);
		Drawable *drawable = new(resData.GetData()) Drawable(&resData);
		return drawable_to_dff(drawable, dstpath, modelname);
	}

	static bool convert_ydd_to_dff(char *srcpath)
	{
		ResourceData resData(srcpath);
		Dictionary<Drawable> *dictionary = new(resData.GetData()) Dictionary<Drawable>(&resData);
		char folderpath[MAX_PATH];
		strcpy(folderpath, srcpath);
		if(strrchr(folderpath, '\\'))
			strrchr(folderpath, '\\')[1] = '\0';
		if(dictionary->m_data.m_wCount > 0 && dictionary->m_data.m_pData && dictionary->m_hashes.m_pData)
		{
			for(int i = 0; i < dictionary->m_data.m_wCount; i++)
			{
				gtaRwClump clump;
				char modelName[32];
				sprintf(modelName, "%X", dictionary->m_hashes.m_pData[i]);
				char dffpath[MAX_PATH];
				strcpy(dffpath, folderpath);
				strcat(dffpath, modelName);
				strcat(dffpath, ".dff");
				drawable_to_dff(dictionary->m_data.m_pData[i], dffpath, modelName);
				clump.Destroy();
			}
		}
		return true;
	}

	static int bone_index_by_id(unsigned int id, Bone *bones, unsigned int numBones)
	{
		for(int i = 0; i < numBones; i++)
		{
			if(bones[i].m_wBoneIndex == id)
				return i;
		}
		return -1;
	}

	static bool convert_yft_to_dff(char *srcpath, char *dstpath, char *modelname)
	{
		ResourceData resData(srcpath);
		
		FragType *fragType = new(resData.GetData()) FragType(&resData);
		gtaRwClump clump;
		if(fragType->m_pDrawable)
		{
			unsigned int numChilds = 0;
			if(fragType->m_pFragData && fragType->m_pFragData->m_pFragChilds)
				numChilds = fragType->m_pFragData->m_pFragChilds->m_nChildsCount;
			skeleton_info skelInfo;
			BoundBox bbox;
			if(settings.m_bExportSkeleton && fragType->m_pDrawable->m_pSkeleton)
				skelInfo.exportSkeleton = true;
			skelInfo.skeleton = fragType->m_pDrawable->m_pSkeleton;
			skelInfo.matrices = new D3DMATRIX[fragType->m_pDrawable->m_pSkeleton->m_wBoneCount];
			for(int i = 0; i < fragType->m_pDrawable->m_pSkeleton->m_wBoneCount; i++)
			{
				D3DMATRIX res;
				int parentBone = i;
				memcpy(&skelInfo.matrices[i], &fragType->m_pDrawable->m_pSkeleton->m_apTransform[parentBone], sizeof(D3DMATRIX));
				while(fragType->m_pDrawable->m_pSkeleton->m_apBones[parentBone].m_wParentBoneId != -1)
				{
					parentBone = fragType->m_pDrawable->m_pSkeleton->m_apParentBoneIndices[parentBone];
					memcpy(&res, &skelInfo.matrices[i], sizeof(D3DMATRIX));
					res._14 = 0.0f; res._24 = 0.0f; res._34 = 0.0f; res._44 = 1.0f;
					MatrixMul((gtaRwMatrix *)&skelInfo.matrices[i], (gtaRwMatrix *)&fragType->m_pDrawable->m_pSkeleton->m_apTransform[parentBone], (gtaRwMatrix *)&res);
				}
			}
			if(settings.m_bExportSkeleton && fragType->m_pDrawable->m_pSkeleton)
			{
				clump.Initialise(1, fragType->m_pDrawable->m_pSkeleton->m_wBoneCount + 1, 1);
				clump.frameList.frames[0].Initialise(-1, 0);
				for(int i = 0; i < fragType->m_pDrawable->m_pSkeleton->m_wBoneCount; i++)
				{
					clump.frameList.frames[i + 1].Initialise((gtaRwV3d *)&fragType->m_pDrawable->m_pSkeleton->m_apTransform[i]._11, 
						(gtaRwV3d *)&fragType->m_pDrawable->m_pSkeleton->m_apTransform[i]._21, 
						(gtaRwV3d *)&fragType->m_pDrawable->m_pSkeleton->m_apTransform[i]._31,
						(gtaRwV3d *)&fragType->m_pDrawable->m_pSkeleton->m_apTransform[i]._41, 
						fragType->m_pDrawable->m_pSkeleton->m_apBones[i].m_wParentBoneId + 1, 0);
					char boneName[24];
					if(fragType->m_pDrawable->m_pSkeleton->m_apBones[i].m_pszName)
					{
						strncpy(boneName, fragType->m_pDrawable->m_pSkeleton->m_apBones[i].m_pszName, 24);
						boneName[23] = '\0';
						clump.frameList.frames[i + 1].Extension.nodeName.Initialise(boneName);
					}
					else
					{
						sprintf(boneName, "bone_%d", i);
						clump.frameList.frames[i + 1].Extension.nodeName.Initialise(boneName);
					}
					if(i == 0)
					{
						gtaRwFrameHAnim &hAnim = clump.frameList.frames[i + 1].Extension.hAnim;
						hAnim.Initialise(fragType->m_pDrawable->m_pSkeleton->m_apBones[i].m_wBoneIndex, fragType->m_pDrawable->m_pSkeleton->m_wBoneCount, 0, 36);
						for(int j = 0; j < hAnim.numNodes; j++)
						{
							hAnim.nodes[j].Initialise(fragType->m_pDrawable->m_pSkeleton->m_apBones[j].m_wBoneIndex, j,
								bone_get_flags(j, fragType->m_pDrawable->m_pSkeleton->m_apBones, fragType->m_pDrawable->m_pSkeleton->m_wBoneCount));
						}
					}
					else
						clump.frameList.frames[i + 1].Extension.hAnim.Initialise(fragType->m_pDrawable->m_pSkeleton->m_apBones[i].m_wBoneIndex);
				}
			}
			else
			{
				clump.Initialise(1, 1, 1);
				clump.frameList.frames[0].Initialise(-1, 0);
				clump.frameList.frames[0].Extension.nodeName.Initialise(modelname);
			}
			vector<Model *> models;
			vector<int> parentBones;
			unsigned int level = get_last_available_lod_level(fragType->m_pDrawable, settings.m_nLodLevel);
			if(fragType->m_pDrawable->m_pModelCollection[level] && fragType->m_pDrawable->m_pModelCollection[level]->m_pData)
			{
				for(int i = 0; i < fragType->m_pDrawable->m_pModelCollection[level]->m_wCount; i++)
				{
					if(fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i] && fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_geometries.m_pData
						&& fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_geometries.m_wCount > 0)
					{
						models.push_back(fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i]);
						if(!fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_bIsSkinned && fragType->m_pDrawable->m_pSkeleton)
							parentBones.push_back(fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_nBone);
						else
							parentBones.push_back(-1);
					}
				}
			}
			bbox.Set(Vector3(fragType->m_pDrawable->m_vAabbMin), Vector3(fragType->m_pDrawable->m_vAabbMax));
			for(int i = 0; i < numChilds; i++)
			{
				FragChild &child = *fragType->m_pFragData->m_pFragChilds->m_ppChilds[i];
				if(child.m_pDrawable)
				{
					unsigned int level = get_last_available_lod_level(child.m_pDrawable, settings.m_nLodLevel);
					if(child.m_pDrawable->m_pModelCollection[level] && child.m_pDrawable->m_pModelCollection[level]->m_pData)
					{
						for(int i = 0; i < child.m_pDrawable->m_pModelCollection[level]->m_wCount; i++)
						{
							if(child.m_pDrawable->m_pModelCollection[level]->m_pData[i] && child.m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_geometries.m_pData
								&& child.m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_geometries.m_wCount > 0)
							{
								models.push_back(child.m_pDrawable->m_pModelCollection[level]->m_pData[i]);
								if(fragType->m_pDrawable->m_pSkeleton)
									parentBones.push_back(bone_index_by_id(child.m_wBoneIndex, fragType->m_pDrawable->m_pSkeleton->m_apBones, fragType->m_pDrawable->m_pSkeleton->m_wBoneCount));
								else
									parentBones.push_back(-1);
							}
						}
						if(child.m_pDrawable->m_vAabbMin.x < bbox.aabbMin.x)
							bbox.aabbMin.x = child.m_pDrawable->m_vAabbMin.x;
						if(child.m_pDrawable->m_vAabbMin.y < bbox.aabbMin.y)
							bbox.aabbMin.y = child.m_pDrawable->m_vAabbMin.y;
						if(child.m_pDrawable->m_vAabbMin.z < bbox.aabbMin.z)
							bbox.aabbMin.z = child.m_pDrawable->m_vAabbMin.z;
						if(child.m_pDrawable->m_vAabbMax.x > bbox.aabbMax.x)
							bbox.aabbMax.x = child.m_pDrawable->m_vAabbMax.x;
						if(child.m_pDrawable->m_vAabbMax.y > bbox.aabbMax.y)
							bbox.aabbMax.y = child.m_pDrawable->m_vAabbMax.y;
						if(child.m_pDrawable->m_vAabbMax.z > bbox.aabbMax.z)
							bbox.aabbMax.z = child.m_pDrawable->m_vAabbMax.z;
					}
				}
			}
			BoundSphere sphere;
			bbox.ToSphere(&sphere);
			if(models.size() > 0)
			{
				clump.atomics[0].Initialise(0, 0, rpATOMICCOLLISIONTEST|rpATOMICRENDER, false);
				models_to_geometry(models, &parentBones, *fragType->m_pDrawable->m_pShaderGroup, clump.geometryList.geometries[0], skelInfo, settings.m_bGenerateBreakableModel);
				memcpy(&clump.geometryList.geometries[0].morphTarget[0].boundingSphere.center, &sphere.center, 12);
				clump.geometryList.geometries[0].morphTarget[0].boundingSphere.radius = sphere.radius;
				add_lights_to_geometry(fragType->m_lights, fragType->m_pDrawable->m_pSkeleton, skelInfo.matrices, clump.geometryList.geometries[0]);
				clump_post_process(&clump, settings.m_bGenerateDPForDynamicObjects, settings.m_bGenerateNPForDynamicObjects);

				//gtaRwErrorSet("Export as vehicle :%d", settings.m_bExportAsVehicle);
				if(settings.m_bExportAsVehicle)
				{
					//gtaRwErrorSet("Exporting as vehicle...");
					gtaRwClump newClump;
					if(vehicle_converter::make_vehicle_clump(clump, newClump, skelInfo.skeleton->m_apTransformInverted, skelInfo.skeleton->m_wBoneCount))
					{
						clump.Destroy();
						memcpy(&clump, &newClump, sizeof(gtaRwClump));
						for(int i = 0; i < clump.geometryList.geometryCount; i++)
						{
							if(clump.geometryList.geometries[0].numVertices >= 65535)
								LOGL("  WARNING: Geometry %d has more than 65535 vertices (%u).", i, clump.geometryList.geometries[0].numVertices);
						}
						//gtaRwErrorSet("Exported as vehicle.");
					}
					else
					{
						//gtaRwErrorSet("Failed to export as vehicle.");
						if(clump.geometryList.geometries[0].numVertices >= 65535)
							LOGL("  WARNING: Mesh has more than 65535 vertices (%u). Edit this model manually to prevent crashes in-game.", clump.geometryList.geometries[0].numVertices);
					}
				}
				else
				{
					if(clump.geometryList.geometries[0].numVertices >= 65535)
						LOGL("  WARNING: Mesh has more than 65535 vertices (%u). Edit this model manually to prevent crashes in-game.", clump.geometryList.geometries[0].numVertices);
				}
			}
			else
			{
				gtaMemFree(clump.atomics);
				clump.atomics = NULL;
				clump.numAtomics = 0;
				clump.geometryList.Destroy();
			}
			if(settings.m_bGenerateCollision)
			{
				char colpath[MAX_PATH];
				strcpy(colpath, dstpath);
				strcpy(&colpath[strlen(colpath) - 3], "col");
				ColFile colFile;
				colFile.SetModelInfo(modelname, -1);
				models.clear();
				parentBones.clear();
				unsigned int maxLodLevel = 0;
				maxLodLevel = get_last_available_lod_level(fragType->m_pDrawable, 3);
				if(fragType->m_pDrawable->m_pModelCollection[maxLodLevel] && fragType->m_pDrawable->m_pModelCollection[maxLodLevel]->m_pData)
				{
					for(int i = 0; i < fragType->m_pDrawable->m_pModelCollection[maxLodLevel]->m_wCount; i++)
					{
						if(fragType->m_pDrawable->m_pModelCollection[maxLodLevel]->m_pData[i])
						{
							models.push_back(fragType->m_pDrawable->m_pModelCollection[maxLodLevel]->m_pData[i]);
							if(!fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_bIsSkinned && fragType->m_pDrawable->m_pSkeleton)
								parentBones.push_back(fragType->m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_nBone);
							else
								parentBones.push_back(-1);
						}
					}
				}
				for(int i = 0; i < numChilds; i++)
				{
					FragChild &child = *fragType->m_pFragData->m_pFragChilds->m_ppChilds[i];
					if(child.m_pDrawable)
					{
						unsigned int level = get_last_available_lod_level(fragType->m_pDrawable, settings.m_nLodLevel);
						if(child.m_pDrawable->m_pModelCollection[level] && child.m_pDrawable->m_pModelCollection[level]->m_pData)
						{
							for(int i = 0; i < child.m_pDrawable->m_pModelCollection[level]->m_wCount; i++)
							{
								if(child.m_pDrawable->m_pModelCollection[level]->m_pData[i] && child.m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_geometries.m_pData
									&& child.m_pDrawable->m_pModelCollection[level]->m_pData[i]->m_geometries.m_wCount > 0)
								{
									models.push_back(child.m_pDrawable->m_pModelCollection[level]->m_pData[i]);
									if(fragType->m_pDrawable->m_pSkeleton)
										parentBones.push_back(bone_index_by_id(child.m_wBoneIndex, fragType->m_pDrawable->m_pSkeleton->m_apBones, fragType->m_pDrawable->m_pSkeleton->m_wBoneCount));
									else
										parentBones.push_back(-1);
								}
							}
						}
					}
				}
				colFile.col3.min.x = bbox.aabbMin.x;
				colFile.col3.min.y = bbox.aabbMin.y;
				colFile.col3.min.z = bbox.aabbMin.z;
				colFile.col3.max.x = bbox.aabbMax.x;
				colFile.col3.max.y = bbox.aabbMax.y;
				colFile.col3.max.z = bbox.aabbMax.z;
				colFile.col3.center.x = sphere.center.x;
				colFile.col3.center.y = sphere.center.y;
				colFile.col3.center.z = sphere.center.z;
				colFile.col3.radius = sphere.radius;
				
				if(models.size() > 0)
				{
					if(!models_to_collision(models, &parentBones, skelInfo.matrices, *fragType->m_pDrawable->m_pShaderGroup, colFile))
						LOGL("  WARNING: This model has more than 32767 vertices. I will generate dummy (empty) collision model.");
					else
					{
						if(settings.m_bOptimizeCollisionMesh)
							colFile.Optimize();
						
						if(colFile.numVertices > 32767)
						{
							if(settings.m_bSkipLargeCol)
							{
								LOGL("  WARNING: This model has more than 32767 vertices. I will generate dummy (empty) collision model.");
								colFile.Clear();
								colFile.SetModelInfo(modelname, -1);
							}
							else
								LOGL("  WARNING: This model has more than 32767 vertices. You must fix it manually.");
						}
						else if(colFile.col3.numFaces > 32767)
						{
							if(settings.m_bSkipLargeCol)
							{
								LOGL("  WARNING: This model has more than 32767 faces. I will delete some faces so it will be a valid colmodel.");
								colFile.col3.numFaces = 32767;
							}
							else
								LOGL("  WARNING: This model has more than 32767 faces. You must fix it manually.");
						}
					}
				}
				colFile.Write(colpath);
			}

			delete[] skelInfo.matrices;
		}
		else
		{
			clump.Initialise(0, 1, 0);
			clump.frameList.frames[0].Initialise(-1, 0);
			clump.frameList.frames[0].Extension.nodeName.Initialise(modelname);

		}
		gtaRwStream *stream = gtaRwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, dstpath);
		bool result = false;
		if(stream)
		{
			clump.StreamWrite(stream);
			gtaRwStreamClose(stream);
			result = true;
		}
		clump.Destroy();

		if(fragType->m_pDrawable && fragType->m_pDrawable->m_pShaderGroup && fragType->m_pDrawable->m_pShaderGroup->m_pTxd)
		{
			gtaRwTexDictionary txd;
			texture_converter::dictionary_to_txd(fragType->m_pDrawable->m_pShaderGroup->m_pTxd, &txd);
			char txdpath[MAX_PATH];
			strcpy(txdpath, dstpath);
			if(strrchr(txdpath, '.'))
				strrchr(txdpath, '.')[1] = '\0';
			strcat(txdpath, "internal.txd");
			gtaRwStream *txdstream = gtaRwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, txdpath);
			if(txdstream)
			{
				txd.StreamWrite(txdstream);
				gtaRwStreamClose(txdstream);
			}
		}
		return result;
	}

	static void add_lights_to_geometry(SimpleCollection<LightAttr>& lights, Skeleton *skeleton, D3DMATRIX *matrices, gtaRwGeometry& geometry)
	{
		if(lights.m_pData > 0 && lights.m_wCount > 0)
		{
			geometry.Extension.effect2d.Initialise(lights.m_wCount);
			for(int i = 0; i < lights.m_wCount; i++)
			{
				unsigned int flags1 = 0;
				flags1 |= LIGHT1_AT_NIGHT;
				if(lights.m_pData[i].m_fCoronaSize == 0.0f)
					flags1 |= LIGHT1_WITHOUT_CORONA;
				Vector3 posn = lights.m_pData[i].m_vPosition;
				if(skeleton && matrices)
				{
					int boneId = bone_index_by_id(lights.m_pData[i].m_wBone, skeleton->m_apBones, skeleton->m_wBoneCount);
					if(boneId != -1)
					{
						Vector3 res;
						res.TransformPosition(&posn, &matrices[boneId]);
						posn = res;
					}
				}
				geometry.Extension.effect2d.effects[i].SetupLight(posn.x, posn.y, posn.z,
					lights.m_pData[i].m_nRed, lights.m_pData[i].m_nGreen, lights.m_pData[i].m_nBlue, settings.m_f2dfxCoronaFarClip, 
					settings.m_f2dfxPointlightSizeMultiplier, lights.m_pData[i].m_fCoronaSize * settings.m_f2dfxCoronaSizeMultiplier, 
					settings.m_f2dfxShadowSizeMultiplier, 0, true, 0, 100, flags1, "coronastar", "shad_exp", 0, 0);
			}
		}
	}

	static void clump_post_process(gtaRwClump *clump, bool genDay, bool genNight)
	{
		Vector3 sun;
		sun.x = settings.m_fPrelightSunX;
		sun.y = settings.m_fPrelightSunY;
		sun.z = settings.m_fPrelightSunZ;
		sun.Normalise();
		float rangeB = settings.m_fPrelightSunRangeB - settings.m_fPrelightSunRangeA;
		for(int i = 0; i < clump->geometryList.geometryCount; i++)
		{
			gtaRwGeometry &geometry = clump->geometryList.geometries[i];

			bool hasNormalMap = GeometryHasNM(geometry);
			if(hasNormalMap)
			{
				GeometrySetupNM(geometry);
				gtaRwAtomic *atomic = AtomicByGeomIndex(*clump, i);
				if(atomic)
					AtomicSetupNM(*atomic);
			}

			if(genDay)
			{
				if(geometry.prelit && geometry.preLitLum)
					gtaMemFree(geometry.preLitLum);
				geometry.prelit = true;
				geometry.preLitLum = (gtaRwRGBA *)gtaMemAlloc(4 * geometry.numVertices);
				if(geometry.normals)
				{
					for(int j = 0; j < geometry.numVertices; j++)
					{
						Vector3 normal = *(Vector3 *)&geometry.morphTarget[0].normals[j];
						normal.Normalise();
						
						geometry.preLitLum[j].r = (((float)settings.m_nPrelightSunColorR / 255.0f) * settings.m_fPrelightSunRangeA + Saturate(Vector3::Dot(&sun, &normal)) * rangeB) * 255.0f;
						geometry.preLitLum[j].g = (((float)settings.m_nPrelightSunColorG / 255.0f) * settings.m_fPrelightSunRangeA + Saturate(Vector3::Dot(&sun, &normal)) * rangeB) * 255.0f;
						geometry.preLitLum[j].b = (((float)settings.m_nPrelightSunColorB / 255.0f) * settings.m_fPrelightSunRangeA + Saturate(Vector3::Dot(&sun, &normal)) * rangeB) * 255.0f;
						geometry.preLitLum[j].a = 255;
					}
				}
				else
				{
					for(int j = 0; j < geometry.numVertices; j++)
					{
						geometry.preLitLum[j].r = (float)settings.m_nPrelightSunColorR * settings.m_fPrelightSunRangeA;
						geometry.preLitLum[j].g = (float)settings.m_nPrelightSunColorG * settings.m_fPrelightSunRangeA;
						geometry.preLitLum[j].b = (float)settings.m_nPrelightSunColorB * settings.m_fPrelightSunRangeA;
						geometry.preLitLum[j].a = 255;
					}
				}

				unsigned char *vertProcessed = new unsigned char[geometry.numVertices];
				memset(vertProcessed, 0, geometry.numVertices);

				// for all meshes
				for(int j = 0; j < geometry.Extension.mesh.numMeshes; j++)
				{
					gtaRwMesh &mesh = geometry.Extension.mesh.meshes[j];
					if(geometry.matList.materials[mesh.material].flags & MATERIAL_EMISSIVE)
					{
						// for all indices
						for(int v = 0; v < mesh.numIndices; v++)
						{
							// if vertex not processed
							if(!vertProcessed[mesh.indices[v]])
							{
								unsigned int emissive = geometry.matList.materials[mesh.material].surfaceProps.specular * settings.m_fNightPrelightMultiplier * 255.0f;
								unsigned int color = geometry.preLitLum[mesh.indices[v]].r + emissive;
								if(color > 255)
									color = 255;
								geometry.preLitLum[mesh.indices[v]].r = color;
								color = geometry.preLitLum[mesh.indices[v]].g + emissive;
								if(color > 255)
									color = 255;
								geometry.preLitLum[mesh.indices[v]].g = color;
								color = geometry.preLitLum[mesh.indices[v]].b + emissive;
								if(color > 255)
									color = 255;
								geometry.preLitLum[mesh.indices[v]].b = color;
								vertProcessed[mesh.indices[v]] = 1;
							}
						}
					}
				}

				delete[] vertProcessed;
			}
			else if(!settings.m_bExportVertexColors && geometry.prelit)
			{
				geometry.prelit = false;
				gtaMemFree(geometry.preLitLum);
				geometry.preLitLum = NULL;
			}

			// calculate night lighting
			if(genNight)
			{
				if(!geometry.prelit)
				{
					geometry.prelit = true;
					geometry.preLitLum = (gtaRwRGBA *)gtaMemAlloc(4 * geometry.numVertices, 0xFFFFFFFF);
					if(geometry.Extension.breakable.enabled)
					{
						for(int j = 0; j < geometry.numVertices; j++)
						{
							geometry.preLitLum[j].r = settings.m_fBreakableLighting * 255.0f;
							geometry.preLitLum[j].g = settings.m_fBreakableLighting * 255.0f;
							geometry.preLitLum[j].b = settings.m_fBreakableLighting * 255.0f;
						}
					}
				}
				geometry.Extension.extraColour.Initialise(geometry.numVertices);
				for(int j = 0; j < geometry.numVertices; j++)
				{
					geometry.Extension.extraColour.nightColors[j].r = settings.m_fNightPrelightBaseLevel * 255.0f;
					geometry.Extension.extraColour.nightColors[j].g = settings.m_fNightPrelightBaseLevel * 255.0f;
					geometry.Extension.extraColour.nightColors[j].b = settings.m_fNightPrelightBaseLevel * 255.0f;
					geometry.Extension.extraColour.nightColors[j].a = 255;
				}

				unsigned char *vertProcessed = new unsigned char[geometry.numVertices];
				memset(vertProcessed, 0, geometry.numVertices);

				// for all meshes
				for(int j = 0; j < geometry.Extension.mesh.numMeshes; j++)
				{
					gtaRwMesh &mesh = geometry.Extension.mesh.meshes[j];
					if(geometry.matList.materials[mesh.material].flags & MATERIAL_EMISSIVENIGHT)
					{
						// for all indices
						for(int v = 0; v < mesh.numIndices; v++)
						{
							// if vertex not processed
							if(!vertProcessed[mesh.indices[v]])
							{
								unsigned int emissive = geometry.matList.materials[mesh.material].surfaceProps.specular * settings.m_fNightPrelightMultiplier * 255.0f;
								unsigned int color = geometry.Extension.extraColour.nightColors[mesh.indices[v]].r + emissive;
								if(color > 255)
									color = 255;
								geometry.Extension.extraColour.nightColors[mesh.indices[v]].r = color;
								color = geometry.Extension.extraColour.nightColors[mesh.indices[v]].g + emissive;
								if(color > 255)
									color = 255;
								geometry.Extension.extraColour.nightColors[mesh.indices[v]].g = color;
								color = geometry.Extension.extraColour.nightColors[mesh.indices[v]].b + emissive;
								if(color > 255)
									color = 255;
								geometry.Extension.extraColour.nightColors[mesh.indices[v]].b = color;
								vertProcessed[mesh.indices[v]] = 1;
							}
						}
					}
				}

				delete[] vertProcessed;
			}

			if(!hasNormalMap && !settings.m_bExportNormals && geometry.normals)
			{
				geometry.normals = false;
				geometry.morphTarget[0].hasNormals = false;
				gtaMemFree(geometry.morphTarget[0].normals);
				geometry.morphTarget[0].normals = NULL;
			}
			for(int m = 0; m < geometry.matList.materialCount; m++)
			{
				geometry.matList.materials[m].flags &= (MATERIAL_VEHICLELIGHT);
				geometry.matList.materials[m].surfaceProps.specular = 0.0f;
			}

			if(settings.m_nFirstUVChannel != -1 && geometry.GetTexCoordsCount() > 0)
			{
				int firstUV = settings.m_nFirstUVChannel;
				if(firstUV >= geometry.GetTexCoordsCount())
					firstUV = geometry.GetTexCoordsCount() - 1;
				if(firstUV != 0)
					memcpy(geometry.texCoords[0], geometry.texCoords[firstUV], 8 * geometry.numVertices);
				if(geometry.GetTexCoordsCount() > 1)
				{
					if(settings.m_nSecondUVChannel != -1)
					{
						int secondUV = settings.m_nSecondUVChannel;
						if(secondUV >= geometry.GetTexCoordsCount())
							secondUV = geometry.GetTexCoordsCount() - 1;
						if(secondUV != 1)
							memcpy(geometry.texCoords[0], geometry.texCoords[secondUV], 8 * geometry.numVertices);
						if(geometry.GetTexCoordsCount() > 2)
						{
							for(int t = 2; t < geometry.GetTexCoordsCount(); t++)
							{
								if(geometry.texCoords[t])
								{
									gtaMemFree(geometry.texCoords[t]);
									geometry.texCoords[t] = NULL;
								}
							}
						
						}
						geometry.numTexCoordSets = 2;
					}
					else
					{
						if(geometry.texCoords[1])
						{
							gtaMemFree(geometry.texCoords[1]);
							geometry.texCoords[1] = NULL;
						}
						if(geometry.GetTexCoordsCount() > 2)
						{
							for(int t = 2; t < geometry.GetTexCoordsCount(); t++)
							{
								if(geometry.texCoords[t])
								{
									gtaMemFree(geometry.texCoords[t]);
									geometry.texCoords[t] = NULL;
								}
							}
						
						}
						geometry.numTexCoordSets = 1;
						geometry.textured2 = false;
					}
				}
				
			}

			if(settings.m_fScaleMultiplier != 1.0f)
			{
				for(int v = 0; v < geometry.numVertices; v++)
				{
					scale_vector(geometry.morphTarget[0].verts[v]);
					if(geometry.Extension.breakable.enabled)
						scale_vector(geometry.Extension.breakable.vertices[v]);
				}
				if(geometry.Extension.effect2d.enabled)
				{
					for(int e = 0; e < geometry.Extension.effect2d.effectCount; e++)
						scale_vector(geometry.Extension.effect2d.effects[e].offset);
				}
				if(geometry.Extension.skin.enabled && geometry.Extension.skin.skinToBoneMatrices)
				{
					for(int i = 0; i < geometry.Extension.skin.numBones; i++)
						scale_vector(geometry.Extension.skin.skinToBoneMatrices[i].pos);
				}
			}

			if(geometry.Extension.breakable.enabled && geometry.numVertices > 0)
			{
				memcpy(geometry.Extension.breakable.vertices, geometry.morphTarget[0].verts, geometry.numVertices * 12);
				if(geometry.GetTexCoordsCount() > 0)
					memcpy(geometry.Extension.breakable.texCoors, geometry.texCoords[0], geometry.numVertices * 8);
				if(geometry.preLitLum)
					memcpy(geometry.Extension.breakable.colors, geometry.preLitLum, geometry.numVertices * 4);
			}
		}
		if(settings.m_fScaleMultiplier != 1.0f)
		{
			for(int f = 0; f < clump->frameList.frameCount; f++)
				scale_vector(clump->frameList.frames[f].pos);
		}
	}
};
#pragma once

#include "dffapi\Clump.h"
#include "Settings.h"

#include <vector>

using namespace std;

class vehicle_converter
{
public:
	struct node
	{
		vector<unsigned int> uniqueIds;
		unsigned int numUsedMeshes;
		unsigned int totalNumIndices;
		int *mtlMapping;
		unsigned int numMaterials;

		struct mesh
		{
			vector<unsigned int> indices;
			unsigned int mtlId;

			void add_index(node& parent_node, unsigned int index)
			{
				for(int i = 0; i < parent_node.uniqueIds.size(); i++)
				{
					if(parent_node.uniqueIds[i] == index)
					{
						indices.push_back(i);
						return;
					}
				}
				indices.push_back(parent_node.uniqueIds.size());
				parent_node.uniqueIds.push_back(index);
			}
		} *meshes;

		void alloc_materials_map(unsigned int NumMaterials)
		{
			mtlMapping = new int[NumMaterials];
			for(int i = 0; i < NumMaterials; i++)
				mtlMapping[i] = -1;
			numMaterials = NumMaterials;
		}

		void add_material(unsigned int index)
		{
			mtlMapping[index] = 1;
		}

		unsigned int create_materials_map()
		{
			unsigned int mtlId = 0;
			for(int i = 0; i < numMaterials; i++)
			{
				if(mtlMapping[i] != -1)
				{
					mtlMapping[i] = mtlId;
					mtlId++;
				}
			}
			return mtlId;
		}

		node()
		{
			numUsedMeshes = 0;
			totalNumIndices = 0;
			meshes = NULL;
			mtlMapping = NULL;
		}

		~node()
		{
			delete[] meshes;
			delete[] mtlMapping;
		}
	};

	static void set_mat_color(gtaRwMaterial& mat, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		mat.color.r = r;
		mat.color.g = g;
		mat.color.b = b;
		mat.color.a = a;
	}

	static void scale_vector(gtaRwV3d &v)
	{
		v.x *= settings.m_fScaleMultiplier;
		v.y *= settings.m_fScaleMultiplier;
		v.z *= settings.m_fScaleMultiplier;
	}

	static bool make_vehicle_clump(gtaRwClump& clump, gtaRwClump& newClump, D3DMATRIX *transform, unsigned int numMatrices)
	{
		if(!clump.geometryList.geometryCount || clump.frameList.frameCount < 2 || !clump.geometryList.geometries[0].Extension.skin.enabled)
			return false;
		gtaRwMatrix *matrices = new gtaRwMatrix[numMatrices];
		memcpy(matrices, transform, numMatrices * sizeof(D3DMATRIX));
		if(settings.m_fScaleMultiplier != 1.0f)
		{
			for(int i = 0; i < numMatrices; i++)
				scale_vector(matrices[i].pos);
		}
		unsigned int frameCount = clump.frameList.frameCount - 1;
		gtaRwGeometry &geometry = clump.geometryList.geometries[0];
		node *nodes = new node[frameCount];
		for(int i = 0; i < frameCount; i++)
		{
			nodes[i].meshes = new node::mesh[geometry.Extension.mesh.numMeshes];
			nodes[i].alloc_materials_map(geometry.matList.materialCount);
		}
		for(int i = 0; i < geometry.Extension.mesh.numMeshes; i++)
		{
			gtaRwMesh &mesh = geometry.Extension.mesh.meshes[i];
			for(int j = 0; j < mesh.numIndices / 3; j++)
			{
				unsigned int nodeId = 0;
				if(geometry.Extension.skin.vertexBoneWeights[mesh.indices[j * 3]].w0 > 0.0f)
					nodeId = geometry.Extension.skin.vertexBoneIndices[mesh.indices[j * 3]].i0;
				nodes[nodeId].meshes[i].add_index(nodes[nodeId], mesh.indices[j * 3]);
				nodes[nodeId].meshes[i].add_index(nodes[nodeId], mesh.indices[j * 3 + 1]);
				nodes[nodeId].meshes[i].add_index(nodes[nodeId], mesh.indices[j * 3 + 2]);
				nodes[nodeId].add_material(mesh.material);
			}
		}
		unsigned int numGeometries = 0;
		for(int i = 0; i < frameCount; i++)
		{
			for(int j = 0; j < geometry.Extension.mesh.numMeshes; j++)
			{
				nodes[i].meshes[j].mtlId = geometry.Extension.mesh.meshes[j].material;
				nodes[i].totalNumIndices += nodes[i].meshes[j].indices.size();
				if(nodes[i].meshes[j].indices.size() > 0)
					nodes[i].numUsedMeshes++;
			}
			if(nodes[i].numUsedMeshes > 0)
				numGeometries++;
		}
		newClump.Initialise(numGeometries, frameCount, numGeometries);
		memcpy(newClump.frameList.frames, &clump.frameList.frames[1], sizeof(gtaRwFrame) * frameCount);
		unsigned int geomCounter = 0;
		for(int i = 0; i < frameCount; i++)
		{
			memset(&newClump.frameList.frames[i].Extension.hAnim, 0, sizeof(gtaRwFrameHAnim));
			memset(&newClump.frameList.frames[i].Extension.nodeName, 0, sizeof(gtaFrameNodeName));
			if(clump.frameList.frames[i + 1].Extension.nodeName.enabled)
				newClump.frameList.frames[i].Extension.nodeName.Initialise(clump.frameList.frames[i + 1].Extension.nodeName.name);
			newClump.frameList.frames[i].parent -= 1;
			if(nodes[i].numUsedMeshes > 0)
			{
				unsigned int numMaterials = nodes[i].create_materials_map();
				newClump.geometryList.geometries[geomCounter].Initialise(nodes[i].totalNumIndices / 3, nodes[i].uniqueIds.size(), 1, geometry.format,
					geometry.numTexCoordSets, 0.0f, 0.0f, 0.0f, 0.0f);
				newClump.geometryList.geometries[geomCounter].Extension.mesh.Initialise(0, nodes[i].numUsedMeshes, nodes[i].totalNumIndices);
				newClump.atomics[geomCounter].Initialise(i, geomCounter, rpATOMICCOLLISIONTEST|rpATOMICRENDER, false);
				unsigned int meshCounter = 0;
				unsigned int triCounter = 0;
				for(int j = 0; j < geometry.Extension.mesh.numMeshes; j++)
				{
					if(nodes[i].meshes[j].indices.size() > 0)
					{
						unsigned int meshMtl = nodes[i].mtlMapping[nodes[i].meshes[j].mtlId];
						newClump.geometryList.geometries[geomCounter].Extension.mesh.meshes[meshCounter].Initialise(nodes[i].meshes[j].indices.size(), meshMtl);
						for(int k = 0; k < nodes[i].meshes[j].indices.size() / 3; k++)
						{
							newClump.geometryList.geometries[geomCounter].Extension.mesh.meshes[meshCounter].indices[k * 3] = nodes[i].meshes[j].indices[k * 3];
							newClump.geometryList.geometries[geomCounter].Extension.mesh.meshes[meshCounter].indices[k * 3 + 1] = nodes[i].meshes[j].indices[k * 3 + 1];
							newClump.geometryList.geometries[geomCounter].Extension.mesh.meshes[meshCounter].indices[k * 3 + 2] = nodes[i].meshes[j].indices[k * 3 + 2];
							newClump.geometryList.geometries[geomCounter].triangles[triCounter + k].vertA = nodes[i].meshes[j].indices[k * 3];
							newClump.geometryList.geometries[geomCounter].triangles[triCounter + k].vertB = nodes[i].meshes[j].indices[k * 3 + 1];
							newClump.geometryList.geometries[geomCounter].triangles[triCounter + k].vertC = nodes[i].meshes[j].indices[k * 3 + 2];
							newClump.geometryList.geometries[geomCounter].triangles[triCounter + k].mtlId = meshMtl;
						}
						triCounter += nodes[i].meshes[j].indices.size() / 3;
						meshCounter++;
					}
				}
				for(int j = 0; j < nodes[i].uniqueIds.size(); j++)
				{
					Vector3 posn;
					posn.TransformPosition((Vector3 *)(&clump.geometryList.geometries[0].morphTarget[0].verts[nodes[i].uniqueIds[j]]),
						(D3DMATRIX *)&matrices[i]);
					memcpy(&newClump.geometryList.geometries[geomCounter].morphTarget[0].verts[j], &posn, sizeof(gtaRwV3d));
					if(newClump.geometryList.geometries[geomCounter].normals)
					{
						Vector3 normal;
						normal.TransformNormal((Vector3 *)(&clump.geometryList.geometries[0].morphTarget[0].normals[nodes[i].uniqueIds[j]]),
							(D3DMATRIX *)&matrices[i]);
						memcpy(&newClump.geometryList.geometries[geomCounter].morphTarget[0].normals[j], &normal, sizeof(gtaRwV3d));
					}
					if(newClump.geometryList.geometries[geomCounter].prelit)
					{
						memcpy(&newClump.geometryList.geometries[geomCounter].preLitLum[j], 
							&clump.geometryList.geometries[0].preLitLum[nodes[i].uniqueIds[j]], sizeof(gtaRwRGBA));
					}
					for(int k = 0; k < newClump.geometryList.geometries[geomCounter].GetTexCoordsCount(); k++)
					{
						memcpy(&newClump.geometryList.geometries[geomCounter].texCoords[k][j], 
							&clump.geometryList.geometries[0].texCoords[k][nodes[i].uniqueIds[j]], sizeof(gtaRwTexCoords));
					}
				}
				if(!numMaterials)
				{
					newClump.geometryList.geometries[geomCounter].matList.Initialise(1);
					newClump.geometryList.geometries[geomCounter].matList.materials[0].Initialise(255, 255, 255, 255, false, 1.0f, 1.0f);
				}
				else
				{
					newClump.geometryList.geometries[geomCounter].matList.Initialise(numMaterials);
					for(int j = 0; j < nodes[i].numMaterials; j++)
					{
						if(nodes[i].mtlMapping[j] != -1)
						{
							unsigned int mtlId = nodes[i].mtlMapping[j];
							gtaRwMaterial &material = clump.geometryList.geometries[0].matList.materials[j];
							newClump.geometryList.geometries[geomCounter].matList.materials[mtlId].Initialise(material.color.r, material.color.g, 
								material.color.b, material.color.a, material.textured, material.surfaceProps.ambient, material.surfaceProps.diffuse);
							newClump.geometryList.geometries[geomCounter].matList.materials[mtlId].surfaceProps.specular = material.surfaceProps.specular;
							newClump.geometryList.geometries[geomCounter].matList.materials[mtlId].flags = material.flags;
							newClump.geometryList.geometries[geomCounter].matList.materials[mtlId].unused = material.unused;
							if(material.textured)
							{
								newClump.geometryList.geometries[geomCounter].matList.materials[mtlId].texture.Initialise(rwFILTERLINEARMIPLINEAR,
									rwTEXTUREADDRESSWRAP, rwTEXTUREADDRESSWRAP, false, material.texture.name.string, NULL);
							}
							if(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId].flags & MATERIAL_VEHICLELIGHT && newClump.frameList.frames[i].Extension.nodeName.name)
							{
								if(settings.m_bExportVehicleLightMaterials)
								{
									if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "headlight_l"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 255, 175, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "headlight_r"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 0, 255, 200, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "taillight_l"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 185, 255, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "taillight_r"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 255, 60, 0, 255);
								}
								if(settings.m_bExportImVehFtLightMaterials)
								{
									if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "brakelight_l"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 184, 255, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "brakelight_r"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 255, 59, 0, 255);
									if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "reversinglight_l"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 255, 173, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "reversinglight_r"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 0, 255, 198, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "indicator_lf"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 183, 255, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "indicator_rf"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 255, 58, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "indicator_lm"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 182, 255, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "indicator_rm"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 255, 57, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "indicator_lr"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 181, 255, 0, 255);
									else if(!strcmp(newClump.frameList.frames[i].Extension.nodeName.name, "indicator_rr"))
										set_mat_color(newClump.geometryList.geometries[geomCounter].matList.materials[mtlId], 255, 56, 0, 255);
								}
							}
							newClump.geometryList.geometries[geomCounter].matList.materials[mtlId].flags = 0;
						}
					}
				}
				geomCounter++;
			}
		}
		delete[] nodes;
		return true;
	}
};
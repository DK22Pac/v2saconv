#pragma once

#include "dffapi\CollisionPlugin.h"
#include <stdio.h>
#include <vector>
#include "Settings.h"

class ColFile : public gtaCollisionFile
{
public:
	unsigned int numVertices;

	ColFile()
	{
		memset(this, 0, sizeof(ColFile));
	}

	void SetModelInfo(const char *Name, short ModelId)
	{
		strncpy(this->modelName, Name, 20);
		this->modelId = ModelId;
	}

	void InitStatic(unsigned int NumFaces, unsigned int NumVertices)
	{
		if(NumFaces)
			this->col3.isNotEmpty = true;
		this->numVertices = NumVertices;
		this->col3.numFaces = NumFaces;
		this->col3.faces = new gtaCollisionTriangle_COL2[NumFaces];
		memset(this->col3.faces, 0, sizeof(gtaCollisionTriangle_COL2) * NumFaces);
		this->col3.vertices = new gtaCollisionVertex_COL2[NumVertices];
		memset(this->col3.vertices, 0, sizeof(gtaCollisionVertex_COL2) * NumVertices);
	}

	~ColFile()
	{
		if(this->col3.spheres)
			delete[] this->col3.spheres;
		if(this->col3.boxes)
			delete[] this->col3.boxes;
		if(this->col3.vertices)
			delete[] this->col3.vertices;
		if(this->col3.faces)
			delete[] this->col3.faces;
		if(this->col3.faceGroups)
			delete[] this->col3.faceGroups;
	}

	void Clear()
	{
		if(this->col3.spheres)
			delete[] this->col3.spheres;
		if(this->col3.boxes)
			delete[] this->col3.boxes;
		if(this->col3.vertices)
			delete[] this->col3.vertices;
		if(this->col3.faces)
			delete[] this->col3.faces;
		if(this->col3.faceGroups)
			delete[] this->col3.faceGroups;
		memset(this, 0, sizeof(ColFile));
	}

	bool Compare(gtaCollisionVertex_COL2& v1, gtaCollisionVertex_COL2& v2)
	{
		if(v1.x == v2.x && v1.y == v2.y && v1.z == v2.z)
			return true;
		return false;
	}

	bool CompareTris(gtaCollisionVertex_COL2& a1, gtaCollisionVertex_COL2& a2, gtaCollisionVertex_COL2& a3,
		gtaCollisionVertex_COL2& b1, gtaCollisionVertex_COL2& b2, gtaCollisionVertex_COL2& b3)
	{
		if(Compare(a1, b1))
		{
			if(Compare(a2, b2) && Compare(a3, b3))
					return true;
			else if(Compare(a2, b3) && Compare(a3, b2))
				return true;
		}
		else if(Compare(a1, b2))
		{
			if(Compare(a2, b1) && Compare(a3, b3))
					return true;
			else if(Compare(a2, b3) && Compare(a3, b1))
				return true;
		}
		else if(Compare(a1, b3))
		{
			if(Compare(a2, b1) && Compare(a3, b2))
					return true;
			else if(Compare(a2, b2) && Compare(a3, b1))
				return true;
		}
		return false;
	}

	bool CompareTrisB(gtaCollisionVertex_COL2& a1, gtaCollisionVertex_COL2& a2, gtaCollisionVertex_COL2& a3,
		gtaCollisionVertex_COL2& b1, gtaCollisionVertex_COL2& b2, gtaCollisionVertex_COL2& b3)
	{
		if(Compare(a1, b1) && Compare(a2, b2) && Compare(a3, b3))
			return true;
		return false;
	}

	void Set(gtaCollisionVertex_COL2& v1, gtaCollisionVertex_COL2& v2)
	{
		v1.x = v2.x;
		v1.y = v2.y;
		v1.z = v2.z;
	}

	unsigned char VertInside(gtaCollisionVertex_COL2& vert, BoundBox& bbox)
	{
		Vector3 p;
		p.x = (float)vert.x / 128.0f;
		p.y = (float)vert.y / 128.0f;
		p.z = (float)vert.z / 128.0f;
		if(p.x >= bbox.aabbMin.x && p.x <= bbox.aabbMax.x && p.y >= bbox.aabbMin.y && p.y <= bbox.aabbMax.y && p.z >= bbox.aabbMin.z && p.z <= bbox.aabbMax.z)
			return 1;
		return 0;
	}

	bool TriInside(gtaCollisionTriangle_COL2& tri, BoundBox& bbox)
	{
		if(VertInside(this->col3.vertices[tri.a], bbox) || VertInside(this->col3.vertices[tri.b], bbox) || VertInside(this->col3.vertices[tri.c], bbox))
			return true;
		return false;
	}

	void VertTestMin(gtaCollisionVertex_COL2& min, gtaCollisionVertex_COL2& v)
	{
		if(v.x < min.x)
			min.x = v.x;
		if(v.y < min.y)
			min.y = v.y;
		if(v.z < min.z)
			min.z = v.z;
	}

	void VertTestMax(gtaCollisionVertex_COL2& max, gtaCollisionVertex_COL2& v)
	{
		if(v.x > max.x)
			max.x = v.x;
		if(v.y > max.y)
			max.y = v.y;
		if(v.z > max.z)
			max.z = v.z;
	}

	struct treeleaf
	{
		BoundBox aabb;
		treeleaf *p1;
		treeleaf *p2;
		std::vector<gtaCollisionTriangle_COL2> tris;

		treeleaf()
		{
			p1 = p2 = NULL;
		}

		~treeleaf()
		{
			delete p1;
			delete p2;
		}

		bool is_valid(float &a, float &b)
		{
			return fabs(a - b) > 0.0001f;
		}

		bool is_valid()
		{
			if(is_valid(aabb.aabbMax.x, aabb.aabbMin.x) && is_valid(aabb.aabbMax.y, aabb.aabbMin.y) && is_valid(aabb.aabbMax.z, aabb.aabbMin.z))
				return true;
			return false;
		}

		void divide(ColFile *colFile)
		{
			if(!is_valid())
				return;
			p1 = new treeleaf;
			p2 = new treeleaf;
			p1->aabb.aabbMin = aabb.aabbMin;
			p1->aabb.aabbMax = aabb.aabbMax;
			p2->aabb.aabbMin = aabb.aabbMin;
			p2->aabb.aabbMax = aabb.aabbMax;
			Vector3 distances = aabb.aabbMax - aabb.aabbMin;
			if(distances.x > distances.y && distances.x > distances.z)
				p1->aabb.aabbMax.x = p2->aabb.aabbMin.x = aabb.aabbMin.x + distances.x / 2.0f;
			else if(distances.y > distances.z)
				p1->aabb.aabbMax.y = p2->aabb.aabbMin.y = aabb.aabbMin.y + distances.y / 2.0f;
			else
			{
				p1->aabb.aabbMax.z = p2->aabb.aabbMin.z = aabb.aabbMin.z + distances.z / 2.0f;
			}
			for(int i = 0; i < tris.size(); i++)
			{
				if(colFile->TriInside(tris[i], p1->aabb))
					p1->tris.push_back(tris[i]);
				else
					p2->tris.push_back(tris[i]);
			}
			tris.clear();
			if(p1->tris.size() > 50)
				p1->divide(colFile);
			if(p2->tris.size() > 50)
				p2->divide(colFile);
		}

		void count(unsigned int *counter)
		{
			if(p1)
			{
				p1->count(counter);
				p2->count(counter);
			}
			else if(tris.size() > 0)
				*counter += 1;
		}

		void flush(ColFile *colFile)
		{
			if(p1)
			{
				p1->flush(colFile);
				p2->flush(colFile);
			}
			else if(tris.size() > 0)
			{
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].startFace = colFile->col3.numFaces;
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].endFace = colFile->col3.numFaces + tris.size() - 1;
				gtaCollisionVertex_COL2 min, max;
				memcpy(&min, &colFile->col3.vertices[tris[0].a], sizeof(gtaCollisionVertex_COL2));
				memcpy(&max, &colFile->col3.vertices[tris[0].a], sizeof(gtaCollisionVertex_COL2));
				for(int i = 0; i < tris.size(); i++)
				{
					memcpy(&colFile->col3.faces[i + colFile->col3.numFaces], &tris[i], sizeof(gtaCollisionTriangle_COL2));
					colFile->VertTestMin(min, colFile->col3.vertices[tris[i].a]);
					colFile->VertTestMin(min, colFile->col3.vertices[tris[i].b]);
					colFile->VertTestMin(min, colFile->col3.vertices[tris[i].c]);
					colFile->VertTestMax(max, colFile->col3.vertices[tris[i].a]);
					colFile->VertTestMax(max, colFile->col3.vertices[tris[i].b]);
					colFile->VertTestMax(max, colFile->col3.vertices[tris[i].c]);
				}
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].min.x = (float)min.x / 128.0f;
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].min.y = (float)min.y / 128.0f;
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].min.z = (float)min.z / 128.0f;
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].max.x = (float)max.x / 128.0f;
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].max.y = (float)max.y / 128.0f;
				colFile->col3.faceGroups[colFile->col3.numFaceGroups].max.z = (float)max.z / 128.0f;
				colFile->col3.numFaceGroups++;
				colFile->col3.numFaces += tris.size();
			}
		}
	};

	void Optimize()
	{
		if(!this->col3.numFaces)
			return;

		gtaCollisionTriangle_COL2 *faces = this->col3.faces;
		gtaCollisionVertex_COL2 *vertices = this->col3.vertices;
		gtaCollisionTriangle_COL2 *newFaces = new gtaCollisionTriangle_COL2[this->col3.numFaces];
		gtaCollisionVertex_COL2 *newVertices = new gtaCollisionVertex_COL2[this->numVertices];
		unsigned int newFacesCount = 0;
		unsigned int newVerticesCount = 0;
		gtaCollisionVertex_COL2 min, max;
		memcpy(&min, &this->col3.vertices[faces[0].a], sizeof(gtaCollisionVertex_COL2));
		memcpy(&max, &this->col3.vertices[faces[0].a], sizeof(gtaCollisionVertex_COL2));
		// scan all faces
		for(int f = 0; f < this->col3.numFaces; f++)
		{
			// skip face if degenerated
			if(faces[f].a == faces[f].b || faces[f].b == faces[f].c || faces[f].a == faces[f].c)
				continue;
			if(Compare(vertices[faces[f].a], vertices[faces[f].b]) || Compare(vertices[faces[f].b], vertices[faces[f].c]) || Compare(vertices[faces[f].a], vertices[faces[f].c]))
				continue;
			// skip face if already in a list
			bool alreadyInAList = false;
			for(int i = 0; i < newFacesCount; i++)
			{
				if(CompareTris(vertices[faces[f].a], vertices[faces[f].b], vertices[faces[f].c], newVertices[newFaces[i].a], newVertices[newFaces[i].b], newVertices[newFaces[i].c]))
				{
					alreadyInAList = true;
					break;
				}
			}
			if(alreadyInAList)
				continue;
			// store new face
			newFaces[newFacesCount].light = faces[f].light;
			newFaces[newFacesCount].material = faces[f].material;
			// process vertices - check if each of 3 vertices already in a vertex list
			int vertListId[3] = {-1, -1, -1};
			for(int i = 0; i < newVerticesCount; i++)
			{
				if(vertListId[0] == -1)
				{
					if(Compare(vertices[faces[f].a], newVertices[i]))
					{
						vertListId[0] = i;
						continue;
					}
				}
				if(vertListId[1] == -1)
				{
					if(Compare(vertices[faces[f].b], newVertices[i]))
					{
						vertListId[1] = i;
						continue;
					}
				}
				if(vertListId[2] == -1)
				{
					if(Compare(vertices[faces[f].c], newVertices[i]))
					{
						vertListId[2] = i;
						continue;
					}
				}
				if(vertListId[0] != -1 && vertListId[1] != -1 && vertListId[2] != -1)
					break;
			}
			// store new vertices in an array
			if(vertListId[0] == -1)
			{
				Set(newVertices[newVerticesCount], vertices[faces[f].a]);
				VertTestMin(min, newVertices[newVerticesCount]);
				VertTestMax(max, newVertices[newVerticesCount]);
				newFaces[newFacesCount].a = newVerticesCount;
				newVerticesCount++;
			}
			else
				newFaces[newFacesCount].a = vertListId[0];
			if(vertListId[1] == -1)
			{
				Set(newVertices[newVerticesCount], vertices[faces[f].b]);
				VertTestMin(min, newVertices[newVerticesCount]);
				VertTestMax(max, newVertices[newVerticesCount]);
				newFaces[newFacesCount].b = newVerticesCount;
				newVerticesCount++;
			}
			else
				newFaces[newFacesCount].b = vertListId[1];
			if(vertListId[2] == -1)
			{
				Set(newVertices[newVerticesCount], vertices[faces[f].c]);
				VertTestMin(min, newVertices[newVerticesCount]);
				VertTestMax(max, newVertices[newVerticesCount]);
				newFaces[newFacesCount].c = newVerticesCount;
				newVerticesCount++;
			}
			else
				newFaces[newFacesCount].c = vertListId[2];
			newFacesCount++;
		}

		// Set bounding box/sphere
		this->col3.min.x = (float)min.x / 128.0f;
		this->col3.min.y = (float)min.y / 128.0f;
		this->col3.min.z = (float)min.z / 128.0f;
		this->col3.max.x = (float)max.x / 128.0f;
		this->col3.max.y = (float)max.y / 128.0f;
		this->col3.max.z = (float)max.z / 128.0f;
		BoundBox bbox;
		BoundSphere bsp;
		bbox.Set(Vector3(this->col3.min.x, this->col3.min.y, this->col3.min.z), Vector3(this->col3.max.x, this->col3.max.y, this->col3.max.z));
		bbox.ToSphere(&bsp);
		this->col3.center.x = bsp.center.x;
		this->col3.center.y = bsp.center.y;
		this->col3.center.z = bsp.center.z;
		this->col3.radius = bsp.radius;

		// Store new information
		delete[] this->col3.vertices;
		this->col3.vertices = newVertices;
		this->numVertices = newVerticesCount;
		delete[] this->col3.faces;
		this->col3.faces = newFaces;
		this->col3.numFaces = newFacesCount;

		// we need to set the new count of faces (here), otherwise we'll get incorrect aabb data
		if(this->col3.numFaces > 32767 && settings.m_bSkipLargeCol)
			this->col3.numFaces = 32767;
		// Generate aabb tree
		if(settings.m_bGenerateCollisionAABBTree && this->col3.numFaces > 80)
		{
			this->col3.hasFaceGroups = true;
			treeleaf treebase;
			for(int i = 0; i < this->col3.numFaces; i++)
				treebase.tris.push_back(this->col3.faces[i]);
			treebase.aabb = bbox;
			// build tree
			treebase.divide(this);
			// store new data
			unsigned int numGroups = 0;
			treebase.count(&numGroups);
			this->col3.faceGroups = new gtaCollisionFaceGroup[numGroups];
			this->col3.numFaceGroups = 0;
			this->col3.numFaces = 0;
			treebase.flush(this);
		}
	}

	void Write(char *filepath)
	{
		FILE *f = fopen(filepath, "wb");
		fwrite("COL3", 4, 1, f);
		unsigned int size = 112;
		if(this->col3.vertices && numVertices > 0)
		{
			size += numVertices * 6;
			if((numVertices * 6) % 4)
				size += 2;
		}
		if(this->col3.faces && this->col3.numFaces > 0)
			size += this->col3.numFaces * 8;
		if(this->col3.hasFaceGroups)
			size += 4 + this->col3.numFaceGroups * 28;
		if(this->col3.numSpheres > 0 && this->col3.spheres)
			size += this->col3.numSpheres * 20;
		if(this->col3.numBoxes > 0 && this->col3.boxes)
			size += this->col3.numBoxes * 28;
		fwrite(&size, 4, 1, f);
		fwrite(this->modelName, 76, 1, f);
		unsigned int offset = 116;
		unsigned int zero = 0;
		if(this->col3.numSpheres > 0 && this->col3.spheres)
		{
			fwrite(&offset, 4, 1, f);
			offset += this->col3.numSpheres * 20;
		}
		else
			fwrite(&zero, 4, 1, f);
		if(this->col3.numBoxes > 0 && this->col3.boxes)
		{
			fwrite(&offset, 4, 1, f);
			offset += this->col3.numBoxes * 28;
		}
		else
			fwrite(&zero, 4, 1, f);
		fwrite(&zero, 4, 1, f);
		if(this->col3.vertices && numVertices > 0)
		{
			fwrite(&offset, 4, 1, f);
			offset += numVertices * 6;
			if((numVertices * 6) % 4)
				offset += 2;
		}
		else
			fwrite(&zero, 4, 1, f);
		if(this->col3.hasFaceGroups)
			offset += 4 + this->col3.numFaceGroups * 28;
		if(this->col3.faces && this->col3.numFaces > 0)
		{
			fwrite(&offset, 4, 1, f);
			offset += this->col3.numFaces * 8;
		}
		else
			fwrite(&zero, 4, 1, f);
		fwrite(&zero, 4, 1, f);
		fwrite(&zero, 4, 1, f);
		fwrite(&zero, 4, 1, f);
		fwrite(&zero, 4, 1, f);
		if(this->col3.numSpheres > 0 && this->col3.spheres)
			fwrite(this->col3.spheres, this->col3.numSpheres * 20, 1, f);
		if(this->col3.numBoxes > 0 && this->col3.boxes)
			fwrite(this->col3.boxes, this->col3.numBoxes * 28, 1, f);
		if(this->col3.vertices && numVertices > 0)
		{
			fwrite(this->col3.vertices, numVertices * 6, 1, f);
			if((numVertices * 6) % 4)
			{
				unsigned short zero = 0;
				fwrite(&zero, 2, 1, f);
			}
		}
		if(this->col3.hasFaceGroups)
		{
			if(this->col3.numFaceGroups > 0 && this->col3.faceGroups)
				fwrite(this->col3.faceGroups, this->col3.numFaceGroups * 28, 1, f);
			fwrite(&this->col3.numFaceGroups, 4, 1, f);
		}
		if(this->col3.faces && this->col3.numFaces > 0)
			fwrite(this->col3.faces, this->col3.numFaces * 8, 1, f);
		fclose(f);
	}
};
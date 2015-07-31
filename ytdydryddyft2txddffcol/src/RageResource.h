#pragma once
#include <Windows.h>
#include <stdio.h>
#include "Math.h"
#include "zlib\zlib.h"
#include <new>
#include <set>

#define RSC_CPU_DATA 5
#define RSC_GPU_DATA 6
#define CHUNK 16384

struct Ptr
{
	unsigned int offset : 28;
	unsigned int dataType : 4;
	unsigned int _padding64;
};

#define PTR(type, name) union{ type *name; Ptr ptr_##name; }
#define PTR_ARR(type, name, size) union{ type *name[size]; Ptr ptr_##name[size]; }
#define TRANSLATEPTR(name) if(!resData->IsPointerTranslated(ptr_##name)) \
                               name = (decltype(name))resData->TranslatePtr(ptr_##name)
#define TRANSLATEPTR2(type, name) if(!resData->IsPointerTranslated(ptr_##name)) \
	                                  name = (type *)resData->TranslatePtr(ptr_##name)

struct RscHeader
{
	unsigned int fourcc;
	unsigned int version;
	unsigned int sysFlags;
	unsigned int gfxFlags;

	int GetSizeFromFlag(unsigned int flag, int baseSize)
	{
		baseSize <<= (int)(flag & 0xf);
		int size = (int)((((flag >> 17) & 0x7f) + (((flag >> 11) & 0x3f) << 1) + (((flag >> 7) & 0xf) << 2) + (((flag >> 5) & 0x3) << 3) + (((flag >> 4) & 0x1) << 4)) * baseSize);
		for (int i = 0; i < 4; ++i)
		{
			size += (((flag >> (24 + i)) & 1) == 1) ? (baseSize >> (1 + i)) : 0;
		}
		return size;
	}
	
	int GetSizeFromSystemFlag(unsigned int flag)
	{
		return GetSizeFromFlag(flag, 0x2000);
	}
	
	int GetSizeFromGraphicsFlag(unsigned int flag)
	{
		return GetSizeFromFlag(flag, 0x2000);
	}
	
	int GetResourceVersionFromFlags(unsigned int systemFlag, unsigned int graphicsFlag)
	{
		return (int)(((graphicsFlag >> 28) & 0xF) | (((systemFlag >> 28) & 0xF) << 4));
	}

	unsigned int cpuSize() { return GetSizeFromSystemFlag(sysFlags); }
	unsigned int gpuSize() { return GetSizeFromGraphicsFlag(gfxFlags); }
};

class ResourceData : public RscHeader
{
public:
	unsigned int m_dwCpuDataSize;
	unsigned int m_dwGpuDataSize;
	unsigned char *m_pCpuData;
	unsigned char *m_pGpuData;
	unsigned int m_dwSize;
	bool m_bLoaded;

	std::set<Ptr *> translatedPtrs;

	BYTE *GetData()
	{
		return m_pCpuData;
	}

	int DecompressFromFile(FILE *source, unsigned char *dest)
	{
		int ret;
		unsigned have;
		z_stream strm;
		unsigned char in[CHUNK];
		unsigned char out[CHUNK];
		unsigned int total = 0;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		ret = inflateInit2(&strm, -15);
		if(ret != Z_OK)
			return ret;
		do
		{
			strm.avail_in = fread(in, 1, CHUNK, source);
			if(ferror(source))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
			if(strm.avail_in == 0)
				break;
			strm.next_in = in;
			do
			{
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				switch (ret)
				{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
				}
				have = CHUNK - strm.avail_out;
				memcpy(&dest[total], out, have);
				total += have;
			} while (strm.avail_out == 0);
		} while (ret != Z_STREAM_END);
		(void)inflateEnd(&strm);
		return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
	}

	ResourceData(const char *filename)
	{
		FILE *file = fopen(filename, "rb");
		fread(this, sizeof(RscHeader), 1, file);
		m_dwCpuDataSize = cpuSize();
		m_dwGpuDataSize = gpuSize();
		m_dwSize = m_dwCpuDataSize + m_dwGpuDataSize;
		m_pCpuData = new unsigned char[m_dwCpuDataSize + m_dwGpuDataSize];
		m_pGpuData = &m_pCpuData[m_dwCpuDataSize];
		DecompressFromFile(file, m_pCpuData);
		fclose(file);
		m_bLoaded = true;
#ifdef GEN_SYS_GFX_FILES
		char fname[MAX_PATH];
		_splitpath(filename, NULL, NULL, fname, NULL);
		char temp[MAX_PATH];
		strcpy(temp, fname);
		strcat(temp, ".sys");
		FILE *sys = fopen(temp, "wb");
		fwrite(m_pCpuData, m_dwCpuDataSize, 1, sys);
		fclose(sys);
		strcpy(temp, fname);
		strcat(temp, ".gfx");
		FILE *gfx = fopen(temp, "wb");
		fwrite(m_pGpuData, m_dwGpuDataSize, 1, gfx);
		fclose(gfx);
#endif
	}

	~ResourceData()
	{
		delete m_pCpuData;
	}

	bool IsPointerTranslated(Ptr &ptr)
	{
		return translatedPtrs.find(&ptr) != translatedPtrs.end();
	}

	void *TranslatePtr(Ptr &ptr)
	{
		translatedPtrs.insert(&ptr);
		if(ptr.dataType == RSC_CPU_DATA)
			return &m_pCpuData[ptr.offset];
		else if(ptr.dataType == RSC_GPU_DATA)
			return &m_pGpuData[ptr.offset];
		return NULL;
	}
};

template <typename T> class SimpleCollection
{
public:
	PTR(T, m_pData);
	WORD m_wCount;
	WORD m_wSize;
	DWORD pad;

	SimpleCollection(ResourceData *resData)
	{
		TRANSLATEPTR(m_pData);
	}
};

template <typename T> class PtrCollection : public SimpleCollection<T*>
{
public:
	PtrCollection(ResourceData *resData) : SimpleCollection(resData)
	{
		if(m_pData)
		{
			for(WORD i = 0; i < m_wCount; i++)
			{
				if(!resData->IsPointerTranslated(*(Ptr *)((unsigned int)m_pData + i * sizeof(Ptr))))
					m_pData[i] = (T*)resData->TranslatePtr(*(Ptr *)((unsigned int)m_pData + i * sizeof(Ptr)));
				new (m_pData[i]) T(resData);
			}
		}
	}
};

template <class T> class Dictionary
{
public:
	Dictionary(ResourceData *resData) : m_hashes(resData), m_data(resData)
	{
	}
	
	unsigned int field_0[8];
	SimpleCollection<DWORD> m_hashes;
	PtrCollection<T> m_data;
};

class Texture
{
public:
    DWORD64 vtable;
    DWORD unk1[8];
	PTR(CHAR, m_pszName);
    WORD unk2;
    WORD unk3;
    DWORD unk4[7];
    WORD m_wWidth;
    WORD m_wHeight;
    BYTE unk5;
    BYTE unk6;
    WORD m_wStride;
    DWORD m_dwPixelFormat;
    BYTE unk7;
    BYTE m_nbLevels;
    BYTE unk8;
    BYTE unk9;
    DWORD unk10[4];
	PTR(BYTE, m_pPixelData);
	DWORD unk11[6];

	Texture(ResourceData *resData)
	{
		TRANSLATEPTR(m_pszName);
		TRANSLATEPTR(m_pPixelData);
	}
};

struct Bone
{
    FLOAT unk1[12]; // 0.0, 1.0
    WORD m_wBoneId; // -1 if main bone
	SHORT m_wParentBoneId; // -1 if main bone
    DWORD unk2; // 0
	PTR(CHAR, m_pszName);
    WORD m_wFlags; // 7710 ~ 7707
    WORD unk4; // same as array number
    WORD m_wBoneIndex; // anim unique id
    WORD m_wBoneNumber; // array number
    DWORD unk7[2]; // 0

	Bone(ResourceData *resData)
	{
		TRANSLATEPTR(m_pszName);
	}
};

struct Skeleton
{
    DWORD64 vtable;
    DWORD unk1[2];
	PTR(BYTE, unkPtr1);
    WORD unk2;
	WORD boneRelatedDataSize;
	DWORD unk3;
	PTR(Bone, m_apBones);
	PTR(D3DMATRIX, m_apTransformInverted);
	PTR(D3DMATRIX, m_apTransform);
	PTR(SHORT, m_apParentBoneIndices);
	PTR(D3DMATRIX, unkPtr2); // local transforms?
    DWORD unk4[2];
    DWORD unk5[3]; // CRCs?
    WORD unk6; // static 1
	WORD m_wBoneCount;
	WORD boneRotCount;
	WORD unk7;

	Skeleton(ResourceData *resData)
	{
		TRANSLATEPTR(m_apBones);
		if(m_apBones)
		{
			for(BYTE i = 0; i < m_wBoneCount; i++)
				new (&m_apBones[i]) Bone(resData);
		}
		TRANSLATEPTR(m_apTransform);
		TRANSLATEPTR(m_apTransformInverted);
		TRANSLATEPTR(m_apParentBoneIndices);
	}
};

struct ShaderParam
{
    BYTE m_nDataType; // 0 - texture
	BYTE m_nType;
    WORD unk1;
    DWORD unk2;
	PTR(BYTE, m_pData);

	ShaderParam(ResourceData *resData)
	{
		TRANSLATEPTR(m_pData);
		if(m_pData && !m_nDataType)
			new (m_pData) Texture(resData);
	};
};

struct Shader
{
	PTR(ShaderParam, m_pParams);
	DWORD m_dwNameHash;
    DWORD unk1;
	BYTE m_nParamsCount;
	BYTE unk2;
	WORD unk3;
	WORD m_wParamsDataSize;
	WORD unk4;
	DWORD m_dwSpsNameHash;
	DWORD unk5;
	DWORD unk6;
	DWORD unk7;
	DWORD unk8;

	Shader(ResourceData *resData)
	{
		TRANSLATEPTR(m_pParams);
		if(m_pParams)
		{
			for(BYTE i = 0; i < m_nParamsCount; i++)
				new (&m_pParams[i]) ShaderParam(resData);
		}
	}
};

struct ShaderGroup
{
    DWORD64 vtable;
	PTR(Dictionary<Texture>, m_pTxd);
	PtrCollection<Shader> m_shaders;

	ShaderGroup(ResourceData *resData) : m_shaders(resData)
	{
		TRANSLATEPTR(m_pTxd);
		new (m_pTxd) Dictionary<Texture>(resData);
	}
};

struct VertexFlags
{
	DWORD m_bPosition : 1;
	DWORD m_bBlendWeight : 1;
	DWORD m_bBlendIndices : 1;
	DWORD m_bNormal : 1;
	DWORD m_bColor : 1;
	DWORD m_bSpecularColor : 1;
	DWORD m_bTexCoord1 : 1;
	DWORD m_bTexCoord2 : 1;
	DWORD m_bTexCoord3 : 1;
	DWORD m_bTexCoord4 : 1;
	DWORD m_bTexCoord5 : 1;
	DWORD m_bTexCoord6 : 1;
	DWORD m_bTexCoord7 : 1;
	DWORD m_bTexCoord8 : 1;
	DWORD m_bTangent : 1;
	DWORD m_bBinormal : 1;
};

struct VertexElementTypes
{
	DWORD m_nPositionType : 4; // D3DVERTEXELEMENT_FLOAT3
	DWORD m_nBlendWeightType : 4; // D3DVERTEXELEMENT_D3DCOLOR
	DWORD m_nBlendIndicesType : 4; // D3DVERTEXELEMENT_D3DCOLOR
	DWORD m_nNormalType : 4; // D3DVERTEXELEMENT_FLOAT3
	DWORD m_nColorType : 4; // D3DVERTEXELEMENT_D3DCOLOR
	DWORD m_nSpecularColorType : 4; // D3DVERTEXELEMENT_D3DCOLOR
	DWORD m_nTexCoord1Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTexCoord2Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTexCoord3Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTexCoord4Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTexCoord5Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTexCoord6Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTexCoord7Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTexCoord8Type : 4; // D3DVERTEXELEMENT_FLOAT2
	DWORD m_nTangentType : 4; // D3DVERTEXELEMENT_FLOAT4
	DWORD m_nBinormalType : 4; // D3DVERTEXELEMENT_FLOAT4
};

struct VertexDeclaration
{
	VertexFlags m_usedElements;
	BYTE m_nTotalSize;
	BYTE _f6;
	BYTE m_bStoreNormalsDataFirst;
	BYTE m_nNumElements;
	VertexElementTypes m_elementTypes;
};

enum eD3DVertexElementType
{
	D3DVERTEXELEMENT_FLOAT16,
	D3DVERTEXELEMENT_FLOAT16_2,
	D3DVERTEXELEMENT_FLOAT16_3,
	D3DVERTEXELEMENT_FLOAT16_4,
	D3DVERTEXELEMENT_FLOAT1,
	D3DVERTEXELEMENT_FLOAT2,
	D3DVERTEXELEMENT_FLOAT3,
	D3DVERTEXELEMENT_FLOAT4,
	D3DVERTEXELEMENT_UBYTE4,
	D3DVERTEXELEMENT_D3DCOLOR,
	D3DVERTEXELEMENT_DEC3N,
	D3DVERTEXELEMENT_NOTAVAILABLE
};

struct VertexBuffer
{
    DWORD64 vtable;
    WORD m_wVertexSize;
	WORD unk1;
	DWORD unk2;
	PTR(BYTE, m_pLockedData);
	DWORD m_dwVertexCount;
    DWORD unk3;
	PTR(BYTE, m_pVertexData);
	DWORD unk4[2];
	PTR(VertexDeclaration, m_pDeclaration);

	VertexBuffer(ResourceData *resData)
	{
		TRANSLATEPTR(m_pLockedData);
		TRANSLATEPTR(m_pVertexData);
		TRANSLATEPTR(m_pDeclaration);
	}
};

struct IndexBuffer
{
    DWORD64 vtable;
    DWORD m_dwIndexCount;
	DWORD unk1;
	PTR(WORD, m_pIndexData);

	IndexBuffer(ResourceData *resData)
	{
		TRANSLATEPTR(m_pIndexData);
	}
};

struct Geometry
{
    DWORD64 vtable;
    DWORD unk1[4];
	PTR(VertexBuffer, m_pVertexBuffer[4]);
	PTR(IndexBuffer, m_pIndexBuffer[4]);
    DWORD m_dwIndexCount;
	DWORD m_dwFaceCount;
	WORD m_wVertexCount;
	WORD m_wIndicesPerFace;
    DWORD unk2;
	PTR(WORD, m_pBoneMapping);
    WORD m_wVertexStride;
	WORD m_wNumBones;
    DWORD unk4;
	PTR(BYTE, m_pVertexData);
    DWORD unk5[8];

	Geometry(ResourceData *resData)
	{
		for(BYTE i = 0; i < 4; i++)
		{
			TRANSLATEPTR2(VertexBuffer, m_pVertexBuffer[i]);
			new (m_pVertexBuffer[i]) VertexBuffer(resData);
			TRANSLATEPTR2(IndexBuffer, m_pIndexBuffer[i]);
			new (m_pIndexBuffer[i]) IndexBuffer(resData);
			TRANSLATEPTR(m_pBoneMapping);
			TRANSLATEPTR(m_pVertexData);
		}
		TRANSLATEPTR(m_pVertexData);
	}
};

struct Model
{
    DWORD64 vtable;
	PtrCollection<Geometry> m_geometries;
	PTR(FLOAT, m_pBound);
	PTR(WORD, m_pShaderMapping);
    BYTE unk1;
	BYTE m_bIsSkinned;
    BYTE unk2;
    BYTE m_nBone;
    BYTE unk4;
    BYTE unk5;
    WORD m_wShaderMappingCount;

	Model(ResourceData *resData) : m_geometries(resData)
	{
		TRANSLATEPTR(m_pBound);
		TRANSLATEPTR(m_pShaderMapping);
	}
};

struct LightAttr // sizeof = 0xA8
{
	// 2dEffect
    DWORD zeros[2];
    Vector4 m_vPosition;
    BYTE m_nRed, m_nGreen, m_nBlue, m_nAlpha;
    FLOAT unk2; // 15.0 ~ 2.0 ~ 8.0
    DWORD flags; // probably
    WORD m_wBone;
    BYTE m_nType; // 1 - point, 2 - spot
    BYTE unk4;
    WORD unk5[2];
    FLOAT unk6; // 9.0 ~ 2.0 ~ 37.0
    FLOAT unk7; // 32.0 ~ 8.0 ~ 64.0
    FLOAT unk80;
    FLOAT unk81;
    FLOAT unk82; // 1.0 ~ 0.65 ~ 0.141
    FLOAT unk9; // 9.0 ~ 2.0 ~ 3.0
    FLOAT unk100;
    FLOAT unk101;
    FLOAT unk11; // 1.0
    FLOAT unk12; // 1.0
    BYTE unkCA[4];
    FLOAT unk13; // 1.0
    FLOAT m_fCoronaSize;
    FLOAT unk15; // 1.0
    DWORD unk16; // 70 ~ 2560
    FLOAT unk17; // 0.01
    FLOAT unk18; // 1.0
    FLOAT unk19; // 0.1
    Vector3 m_vDirection;
    Vector3 m_vOrigin;
    FLOAT unk231; // 0.0 ~ 10.0
    FLOAT unk24; // 90.0 ~ 25.0
    FLOAT unk25; // 1.0
    FLOAT unk26; // 1.0
    FLOAT unk27; // 1.0
    FLOAT unk28;
    FLOAT unk29;

	LightAttr(ResourceData *resData)
	{
	}
};

struct Drawable
{
    DWORD64 vtable;
	PTR(BYTE, unkPtr1);
	PTR(ShaderGroup, m_pShaderGroup);
	PTR(Skeleton, m_pSkeleton);
    Vector4 m_vCenter;
    Vector4 m_vAabbMin;
    Vector4 m_vAabbMax;
	PTR(PtrCollection<Model>, m_pModelCollection[4]);
    Vector4 m_vMaxPoint;
    DWORD unknown1[6];
    WORD unknown2;
    WORD unknown3;
    DWORD unknown4;
	PTR(PtrCollection<Model>, m_pMainModelCollection);
	PTR(CHAR, m_pszName);
	SimpleCollection<LightAttr> m_lights;
    DWORD unknown5[2];
    PTR(BYTE, unkPtr2);

	Drawable(ResourceData *resData) : m_lights(resData)
	{
		TRANSLATEPTR(m_pShaderGroup);
		new (m_pShaderGroup) ShaderGroup(resData);
		TRANSLATEPTR(m_pSkeleton);
		new (m_pSkeleton) Skeleton(resData);
		for(BYTE i = 0; i < 4; i++)
		{
			TRANSLATEPTR2(PtrCollection<Model>, m_pModelCollection[i]);
			new (m_pModelCollection[i]) PtrCollection<Model>(resData);
		}
		TRANSLATEPTR(m_pMainModelCollection);
		new (m_pMainModelCollection) PtrCollection<Model>(resData);
		TRANSLATEPTR(m_pszName);
	}
};

struct FragDrawable
{
    DWORD64 vtable;
	PTR(BYTE, unkPtr1);
	PTR(ShaderGroup, m_pShaderGroup);
	PTR(Skeleton, m_pSkeleton);
    Vector4 m_vCenter;
    Vector4 m_vAabbMin;
    Vector4 m_vAabbMax;
	PTR(PtrCollection<Model>, m_pModelCollection[4]);
    Vector4 m_vMaxPoint;
    DWORD unknown1[6];
    WORD unknown2;
    WORD unknown3;
    DWORD unknown4;
	PTR(PtrCollection<Model>, m_pMainModelCollection);
	DWORD unknown5[2];
    Vector4 unkMatrix[4];
    DWORD unknown6[8];
    WORD unk7;
    WORD unk8; // 1?
    DWORD unk9[7];
    PTR(BYTE, unkPtr2);
    DWORD unk10[6];

	FragDrawable(ResourceData *resData)
	{
		TRANSLATEPTR(m_pShaderGroup);
		new (m_pShaderGroup) ShaderGroup(resData);
		TRANSLATEPTR(m_pSkeleton);
		new (m_pSkeleton) Skeleton(resData);
		for(BYTE i = 0; i < 4; i++)
		{
			TRANSLATEPTR2(PtrCollection<Model>, m_pModelCollection[i]);
			new (m_pModelCollection[i]) PtrCollection<Model>(resData);
		}
		TRANSLATEPTR(m_pMainModelCollection);
		new (m_pMainModelCollection) PtrCollection<Model>(resData);
	}
};

struct FragChild
{
	DWORD64 vtable;
	FLOAT m_fPristineMass;
	FLOAT m_fDamagedMass;
	BYTE m_nGroupId;
	BYTE unk1;
	WORD m_wBoneIndex;
	DWORD unk2[35];
	PTR(FragDrawable, m_pDrawable);

	FragChild(ResourceData *resData)
	{
		TRANSLATEPTR(m_pDrawable);
		new (m_pDrawable) FragDrawable(resData);
	}
};

struct FragChilds
{
    DWORD64 vtable;
    DWORD unk1[8];
    PTR(BYTE, unkPtr1);
    Vector4 unkVectors[9];
    PTR(BYTE, unkPtr2);
    PTR(BYTE, unkPtr3);
    PTR(FragChild*, m_ppChilds);
    PTR(BYTE, unkPtr4);
    PTR(BYTE, unkPtr5); // empty ptr?
    PTR(BYTE, unkPtr6);
    PTR(BYTE, unkPtr7);
    PTR(BYTE, unkPtr8);
    PTR(BYTE, unkPtr9);
    DWORD unk2[5];
    BYTE unk3;
    BYTE m_nGroupsCount;
    BYTE m_nChildsCount;
    BYTE unk4;

	FragChilds(ResourceData *resData)
	{
		TRANSLATEPTR(m_ppChilds);
		if(m_ppChilds)
		{
			for(BYTE i = 0; i < m_nChildsCount; i++)
			{
				if(!resData->IsPointerTranslated(*(Ptr *)((unsigned int)m_ppChilds + i * sizeof(Ptr))))
					m_ppChilds[i] = (FragChild *)resData->TranslatePtr(*(Ptr *)((unsigned int)m_ppChilds + i * sizeof(Ptr)));
				else
					m_ppChilds[i] = *(FragChild **)((unsigned int)m_ppChilds + i * sizeof(Ptr));
				new (m_ppChilds[i]) FragChild(resData);
			}
		}
	}
};

struct FragData
{
    DWORD64 vtable;
    DWORD unk1[2];
    PTR(FragChilds, m_pFragChilds);

	FragData(ResourceData *resData)
	{
		TRANSLATEPTR(m_pFragChilds);
		new (m_pFragChilds) FragChilds(resData);
	}
};

struct FragType
{
    DWORD64 vtable;
    PTR(BYTE, ptr1);
    DWORD unk1[4];
    FLOAT unknown[4];
	PTR(FragDrawable, m_pDrawable);
    DWORD unk2[5];
    INT unk3; // -1
    DWORD unk4[2];
    PTR(CHAR, m_pFragName);
    DWORD unk5[18];
    PTR(BYTE, ptr2);
    DWORD unk6[4];
    BYTE unk7;
    CHAR unk8; // -1
    BYTE unk9;
    BYTE unk10;
    DWORD unk11; // 1
    INT unk12; // -1
    DWORD unk13;
    FLOAT unk14[2]; // 1.0,1.0
    DWORD unk15[6];
    PTR(FragData, m_pFragData);
	DWORD unk16[6];
	SimpleCollection<LightAttr> m_lights;

	FragType(ResourceData *resData) : m_lights(resData)
	{
		TRANSLATEPTR(m_pDrawable);
		new (m_pDrawable) FragDrawable(resData);
		TRANSLATEPTR(m_pFragName);
		TRANSLATEPTR(m_pFragData);
		new (m_pFragData) FragData(resData);
	}
};
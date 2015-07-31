#pragma once
#include "RageResource.h"
#include "dffapi\Txd.h"
#include "dffapi\texture_helper.h"
#include "dffapi\Memory.h"
#include "Settings.h"
#include "Log.h"
#include <new>

class texture_converter
{
public:
	static bool convert_ytd_to_txd(char *srcpath, char *txdpath)
	{
		ResourceData resData(srcpath);
		Dictionary<Texture> *dictionary = new(resData.GetData()) Dictionary<Texture> (&resData);
		gtaRwTexDictionary txd;
		dictionary_to_txd(dictionary, &txd);
		gtaRwStream *stream = gtaRwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, txdpath);
		if(stream)
		{
			txd.StreamWrite(stream);
			gtaRwStreamClose(stream);
			return true;
		}
		txd.Destroy();
		return false;
	}

	static void dictionary_to_txd(Dictionary<Texture> *dictionary, gtaRwTexDictionary *txd)
	{
		if(!settings.m_nBaseTextureLevel)
			dictionary_to_txd_all_levels(dictionary, txd);
		else
			dictionary_to_txd_from_level(dictionary, txd, settings.m_nBaseTextureLevel);
	}

	static void dictionary_to_txd_all_levels(Dictionary<Texture> *dictionary, gtaRwTexDictionary *txd)
	{
		if(dictionary->m_data.m_wCount > 0 && dictionary->m_data.m_pData)
		{
			txd->Initialise(dictionary->m_data.m_wCount);
			for(int i = 0; i < dictionary->m_data.m_wCount; i++)
			{
				Texture &texture = *dictionary->m_data.m_pData[i];
				char textureName[64];
				if(strlen(texture.m_pszName) >= 64)
					strncpy(textureName, texture.m_pszName, 64);
				else
					strcpy(textureName, texture.m_pszName);
				if(settings.m_bExtendTextureName)
				{
					textureName[63] = '\0';
					if(strlen(texture.m_pszName) > 63)
						LOGL("  WARNING: Texture \"%s\" has too long name. Renamed to \"%s\"", texture.m_pszName, textureName);
				}
				else
				{
					textureName[31] = '\0';
					if(strlen(texture.m_pszName) > 31)
						LOGL("  WARNING: Texture \"%s\" has too long name. Renamed to \"%s\"", texture.m_pszName, textureName);
				}
				unsigned int format = GetFormatRwFormat((D3DFORMAT)texture.m_dwPixelFormat);
				if(texture.m_nbLevels > 1)
					format |= rwRASTERFORMATMIPMAP;
				txd->textures[i].Initialise(rwID_PCD3D9, texture.m_nbLevels > 1? rwFILTERLINEARMIPLINEAR: rwFILTERLINEAR,
					rwTEXTUREADDRESSWRAP, rwTEXTUREADDRESSWRAP, textureName, NULL, format, texture.m_dwPixelFormat, 
					texture.m_wWidth, texture.m_wHeight, GetFormatDepth((D3DFORMAT)texture.m_dwPixelFormat), texture.m_nbLevels, 4, 
					GetFormatHasAlpha((D3DFORMAT)texture.m_dwPixelFormat), false, false, GetFormatIsCompressed((D3DFORMAT)texture.m_dwPixelFormat));
				if(settings.m_nAnisotropyLevel > 0)
					txd->textures[i].Extension.anisot.Initialise(settings.m_nAnisotropyLevel);
				unsigned int total = 0;
				for(int j = 0; j < texture.m_nbLevels; j++)
				{
					txd->textures[i].levels[j].size = (texture.m_wStride * texture.m_wHeight) >> (j * 2);
					txd->textures[i].levels[j].pixels = (gtaRwUInt8 *)gtaMemAlloc(txd->textures[i].levels[j].size);
					memcpy(txd->textures[i].levels[j].pixels, &texture.m_pPixelData[total], txd->textures[i].levels[j].size);
					total += txd->textures[i].levels[j].size;
				}
			}		}
		else
			txd->Initialise(0);
	}

	static void dictionary_to_txd_from_level(Dictionary<Texture> *dictionary, gtaRwTexDictionary *txd, unsigned int startLevel)
	{
		if(dictionary->m_data.m_wCount > 0 && dictionary->m_data.m_pData)
		{
			txd->Initialise(dictionary->m_data.m_wCount);
			for(int i = 0; i < dictionary->m_data.m_wCount; i++)
			{
				Texture &texture = *dictionary->m_data.m_pData[i];
				char textureName[64];
				if(strlen(texture.m_pszName) >= 64)
					strncpy(textureName, texture.m_pszName, 64);
				else
					strcpy(textureName, texture.m_pszName);
				if(settings.m_bExtendTextureName)
					textureName[63] = '\0';
				else
					textureName[31] = '\0';
				unsigned int format = GetFormatRwFormat((D3DFORMAT)texture.m_dwPixelFormat);

				unsigned int total = 0;
				unsigned int width = texture.m_wWidth;
				unsigned int height = texture.m_wHeight;
				unsigned int levels = texture.m_nbLevels;
				unsigned int base_level = startLevel;
				if(base_level >= texture.m_nbLevels)
					base_level = texture.m_nbLevels - 1;
				levels -= base_level;
				for(int j = 0; j < base_level; j++)
				{
					width /= 2;
					if(!width)
						width = 1;
					height /= 2;
					if(!height)
						height = 1;
					total += (texture.m_wStride * texture.m_wHeight) >> (j * 2);
				}
				if(levels > 1)
					format |= rwRASTERFORMATMIPMAP;
				txd->textures[i].Initialise(rwID_PCD3D9, levels > 1? rwFILTERLINEARMIPLINEAR: rwFILTERLINEAR,
					rwTEXTUREADDRESSWRAP, rwTEXTUREADDRESSWRAP, textureName, NULL, format, texture.m_dwPixelFormat, 
					width, height, GetFormatDepth((D3DFORMAT)texture.m_dwPixelFormat), levels, 4, 
					GetFormatHasAlpha((D3DFORMAT)texture.m_dwPixelFormat), false, false, GetFormatIsCompressed((D3DFORMAT)texture.m_dwPixelFormat));
				if(settings.m_nAnisotropyLevel > 0)
					txd->textures[i].Extension.anisot.Initialise(settings.m_nAnisotropyLevel);
				unsigned int level = 0;
				for(int j = base_level; j < texture.m_nbLevels; j++)
				{
					txd->textures[i].levels[level].size = (texture.m_wStride * texture.m_wHeight) >> (j * 2);
					txd->textures[i].levels[level].pixels = (gtaRwUInt8 *)gtaMemAlloc(txd->textures[i].levels[level].size);
					memcpy(txd->textures[i].levels[level].pixels, &texture.m_pPixelData[total], txd->textures[i].levels[level].size);
					total += txd->textures[i].levels[level].size;
					level++;
				}
			}
		}
		else
			txd->Initialise(0);
	}
};
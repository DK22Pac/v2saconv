#pragma once

#include "dffapi\Clump.h"
#include "dffapi\Memory.h"

gtaRwAtomic *AtomicByGeomIndex(gtaRwClump &clump, unsigned int idx)
{
	if(clump.geometryList.geometryCount > 0)
	{
		for(int i = 0; i < clump.numAtomics; i++)
		{
			if(clump.atomics[i].geometryIndex == idx)
				return &clump.atomics[i];
		}
	}
	return NULL;
}

void AtomicSetupNM(gtaRwAtomic &atomic)
{
	if(atomic.Extension.rights[0].enabled && atomic.Extension.rights[0].pluginId == rwID_NORMALMAP)
		return;
	if(atomic.Extension.rights[1].enabled && atomic.Extension.rights[1].pluginId == rwID_NORMALMAP)
		return;
	if(atomic.Extension.rights[0].enabled && atomic.Extension.rights[0].pluginId == rwID_MATFX)
		atomic.Extension.rights[0].Destroy();
	if(atomic.Extension.rights[1].enabled && atomic.Extension.rights[1].pluginId == rwID_MATFX)
		atomic.Extension.rights[1].Destroy();
	atomic.Extension.matFx.Destroy();
	if(atomic.Extension.rights[0].enabled)
	{
		if(atomic.Extension.rights[0].pluginId == rwID_SKIN)
			atomic.Extension.rights[1].Initialise(rwID_NORMALMAP, 2);
		else
			atomic.Extension.rights[1].Initialise(rwID_NORMALMAP, 1);
	}
	else
		atomic.Extension.rights[0].Initialise(rwID_NORMALMAP, 1);
}

void GeometrySetupNM(gtaRwGeometry &geometry)
{
	if(geometry.textured2 || geometry.GetTexCoordsCount() > 1)
	{
		for(int i = 1; i < geometry.GetTexCoordsCount(); i++)
		{
			gtaMemFree(geometry.texCoords[i]);
			geometry.texCoords[i] = NULL;
		}
		geometry.textured2 = false;
		geometry.numTexCoordSets = 1;
	}
	geometry.light = true;
}

void GeometryDeleteColors(gtaRwGeometry &geometry)
{
	if(geometry.prelit)
	{
		geometry.prelit = false;
		gtaMemFree(geometry.preLitLum);
		geometry.preLitLum = NULL;
	}
	if(geometry.Extension.extraColour.enabled)
		geometry.Extension.extraColour.Destroy();
}

bool GeometryHasNM(gtaRwGeometry &geometry)
{
	for(int i = 0; i < geometry.matList.materialCount; i++)
	{
		if(geometry.matList.materials[i].Extension.normalMap.enabled)
			return true;
	}
	return false;
}
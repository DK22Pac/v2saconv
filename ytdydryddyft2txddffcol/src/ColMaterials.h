#pragma once

#include <list>
#include <vector>
#include "Hash.h"

using namespace std;

struct col_texture
{
	string name;
	vector<string> includes;
	vector<string> excludes;
	
	bool compare(char *textureName)
	{
		if(!textureName || textureName[0] == '\0')
			return false;
		if(name.empty() || strstr(textureName, name.c_str()))
		{
			for(int i = 0; i < includes.size(); i++)
			{
				if(strstr(textureName, includes[i].c_str()))
					return false;
			}
			for(int i = 0; i < excludes.size(); i++)
			{
				if(strstr(textureName, excludes[i].c_str()))
					return false;
			}
			if(name.empty())
				return false;
			return true;
		}
		return false;
	}
};

struct col_shader
{
	unsigned int nameHash;
	unsigned int gtaNameHash;
	bool has_texture;
	col_texture texture;

	bool compare(unsigned int shaderHash, char *textureName)
	{
		if(nameHash == shaderHash || gtaNameHash == shaderHash)
		{
			if(has_texture)
				return texture.compare(textureName);
			return true;
		}
		return false;
	}
};

struct col_material
{
	unsigned int id;
	vector<col_shader> shaders;
	vector<col_texture> textures;

	bool compare(unsigned int shaderHash, char *textureName)
	{
		for(int i = 0; i < shaders.size(); i++)
		{
			if(shaders[i].compare(shaderHash, textureName))
				return true;
		}
		for(int i = 0; i < textures.size(); i++)
		{
			if(textures[i].compare(textureName))
				return true;
		}
		return false;
	}
};

class col_materials
{
public:
	vector<col_material> materials;
	col_material skip;
public:
	col_materials()
	{
		FILE *f = fopen("col_gen_materials.dat", "rt");
		if(!f)
			return;
		char line[1024];
		while(fgets(line, 1024, f))
		{
			if(*line != '\n' && *line != '\r')
			{
				if(!strncmp(line, "skip", 4))
				{
					while(fgets(line, 1024, f) && strncmp(line, "end", 3))
					{
						if(!strncmp(line, "shader:", 7))
						{
							char *pLine = &line[7];
							char name[128];
							if(sscanf(pLine, "%s", name) == 1)
							{
								col_shader shader;
								unsigned int i = skip.shaders.size();
								skip.shaders.push_back(shader);
								if(name[0] == '0' && name[1] == 'x')
								{
									unsigned int hashValue;
									sscanf(&name[2], "%X", &hashValue);
									skip.shaders[i].nameHash = hashValue;
									skip.shaders[i].gtaNameHash = hashValue;
								}
								else
								{
									skip.shaders[i].nameHash = HASH(name);
									char gtaName[128];
									sprintf(gtaName, "gta_%s", name);
									skip.shaders[i].gtaNameHash = HASH(gtaName);
								}
								skip.shaders[i].has_texture = false;
								pLine = strstr(pLine, "texture:");
								if(pLine)
								{
									pLine = &pLine[8];
									if(sscanf(pLine, "%s", name) == 1)
									{
										skip.shaders[i].has_texture = true;
										if(*name != '+' && *name != '-')
											skip.shaders[i].texture.name = name;
										for(int c = 0; c < strlen(pLine); c++)
										{
											if(pLine[c] == '+')
											{
												if(sscanf(&pLine[c + 1], "%s", name) == 1)
													skip.shaders[i].texture.includes.push_back(name);
												else
													break;
											}
											else if(pLine[c] == '-')
											{
												if(sscanf(&pLine[c + 1], "%s", name) == 1)
													skip.shaders[i].texture.excludes.push_back(name);
												else
													break;
											}
										}
									}
								}	
							}
						}
						else if(!strncmp(line, "texture:", 8))
						{
							char *pLine = &line[8];
							char name[128];
							if(sscanf(pLine, "%s", name) == 1)
							{
								col_texture texture;
								unsigned int i = skip.textures.size();
								skip.textures.push_back(texture);
								if(*name != '+' && *name != '-')
									skip.textures[i].name = name;
								for(int c = 0; c < strlen(pLine); c++)
								{
									if(pLine[c] == '+')
									{
										if(sscanf(&pLine[c + 1], "%s", name) == 1)
											skip.textures[i].includes.push_back(name);
										else
											break;
									}
									else if(pLine[c] == '-')
									{
										if(sscanf(&pLine[c + 1], "%s", name) == 1)
											skip.textures[i].excludes.push_back(name);
										else
											break;
									}
								}
							}
						}
					}
				}
				else if(!strncmp(line, "material:", 9))
				{
					char *pLine = &line[9];
					unsigned int mtlId;
					if(sscanf(pLine, "%u", &mtlId) == 1)
					{
						int m = materials.size();
						col_material material;
						materials.push_back(material);
						materials[m].id = mtlId;
						while(fgets(line, 1024, f) && strncmp(line, "end", 3))
						{
							if(!strncmp(line, "shader:", 7))
							{
								pLine = &line[7];
								char name[128];
								if(sscanf(pLine, "%s", name) == 1)
								{
									col_shader shader;
									unsigned int i = materials[m].shaders.size();
									materials[m].shaders.push_back(shader);
									if(name[0] == '0' && name[1] == 'x')
									{
										unsigned int hashValue;
										sscanf(&name[2], "%X", &hashValue);
										materials[m].shaders[i].nameHash = hashValue;
										materials[m].shaders[i].gtaNameHash = hashValue;
									}
									else
									{
										materials[m].shaders[i].nameHash = HASH(name);
										char gtaName[128];
										sprintf(gtaName, "gta_%s", name);
										materials[m].shaders[i].gtaNameHash = HASH(gtaName);
									}
									materials[m].shaders[i].has_texture = false;
									pLine = strstr(pLine, "texture:");
									if(pLine)
									{
										pLine = &pLine[8];
										if(sscanf(pLine, "%s", name) == 1)
										{
											materials[m].shaders[i].has_texture = true;
											if(*name != '+' && *name != '-')
												materials[m].shaders[i].texture.name = name;
											for(int c = 0; c < strlen(pLine); c++)
											{
												if(pLine[c] == '+')
												{
													if(sscanf(&pLine[c + 1], "%s", name) == 1)
														materials[m].shaders[i].texture.includes.push_back(name);
													else
														break;
												}
												else if(pLine[c] == '-')
												{
													if(sscanf(&pLine[c + 1], "%s", name) == 1)
														materials[m].shaders[i].texture.excludes.push_back(name);
													else
														break;
												}
											}
										}
									}	
								}
							}
							else if(!strncmp(line, "texture:", 8))
							{
								char *pLine = &line[8];
								char name[128];
								if(sscanf(pLine, "%s", name) == 1)
								{
									col_texture texture;
									unsigned int i = materials[m].textures.size();
									materials[m].textures.push_back(texture);
									if(*name != '+' && *name != '-')
										materials[m].textures[i].name = name;
									for(int c = 0; c < strlen(pLine); c++)
									{
										if(pLine[c] == '+')
										{
											if(sscanf(&pLine[c + 1], "%s", name) == 1)
												materials[m].textures[i].includes.push_back(name);
											else
												break;
										}
										else if(pLine[c] == '-')
										{
											if(sscanf(&pLine[c + 1], "%s", name) == 1)
												materials[m].textures[i].excludes.push_back(name);
											else
												break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		fclose(f);
	}

	bool skip_material(unsigned int shaderHash, char *textureName)
	{
		return skip.compare(shaderHash, textureName);
	}

	unsigned int match_material(unsigned int shaderHash, char *textureName)
	{
		for(int i = 0; i < materials.size(); i++)
		{
			if(materials[i].compare(shaderHash, textureName))
				return materials[i].id;
		}
		return 0;
	}

	void print()
	{
		printf("skip\n");
		for(int i = 0; i < skip.shaders.size(); i++)
		{
			printf("shader: 0x%X", skip.shaders[i].nameHash);
			if(skip.shaders[i].has_texture)
			{
				if(skip.shaders[i].texture.name.empty())
					printf("texture:");
				else
					printf("texture: %s", skip.shaders[i].texture.name.c_str());
				for(int j = 0; j < skip.shaders[i].texture.includes.size(); j++)
					printf(" +%s", skip.shaders[i].texture.includes[j].c_str());
				for(int j = 0; j < skip.shaders[i].texture.excludes.size(); j++)
					printf(" -%s", skip.shaders[i].texture.excludes[j].c_str());
			}
			printf("\n");
		}
		for(int i = 0; i < skip.textures.size(); i++)
		{
			if(skip.textures[i].name.empty())
				printf("texture:");
			else
			printf("texture: %s", skip.textures[i].name.c_str());
			for(int j = 0; j < skip.textures[i].includes.size(); j++)
				printf(" +%s", skip.textures[i].includes[j].c_str());
			for(int j = 0; j < skip.textures[i].excludes.size(); j++)
				printf(" -%s", skip.textures[i].excludes[j].c_str());
			printf("\n");
		}
		printf("end\n");
		for(int m = 0; m < materials.size(); m++)
		{
			printf("material:%u\n", materials[m].id);
			for(int i = 0; i < materials[m].shaders.size(); i++)
			{
				printf("shader: 0x%X", materials[m].shaders[i].nameHash);
				if(materials[m].shaders[i].has_texture)
				{
					if(materials[m].shaders[i].texture.name.empty())
						printf("texture:");
					else
						printf("texture: %s", materials[m].shaders[i].texture.name.c_str());
					for(int j = 0; j < materials[m].shaders[i].texture.includes.size(); j++)
						printf(" +%s", materials[m].shaders[i].texture.includes[j].c_str());
					for(int j = 0; j < materials[m].shaders[i].texture.excludes.size(); j++)
						printf(" -%s", materials[m].shaders[i].texture.excludes[j].c_str());
				}
				printf("\n");
			}
			for(int i = 0; i < materials[m].textures.size(); i++)
			{
				if(materials[m].textures[i].name.empty())
					printf("texture:");
				else
					printf("texture: %s", materials[m].textures[i].name.c_str());
				for(int j = 0; j < materials[m].textures[i].includes.size(); j++)
					printf(" +%s", materials[m].textures[i].includes[j].c_str());
				for(int j = 0; j < materials[m].textures[i].excludes.size(); j++)
					printf(" -%s", materials[m].textures[i].excludes[j].c_str());
				printf("\n");
			}
			printf("end\n");
		}
	}
};

col_materials colMats;
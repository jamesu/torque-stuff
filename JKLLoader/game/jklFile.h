#ifndef _JKLFILE_H_
#define _JKLFILE_H_

/*
	jklFile.cc
	
	Description :
		This file contains various structures used in the JklFile class, aswell as the class itself to load .jkl files. 
	
	Author :
		James Urquhart (jamesu at gmail.com)

*/

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TOKENIZER_H_
#include "core/tokenizer.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _MATERIALLIST_H_
#include "dgl/materialList.h"
#endif

/* Geometry structures */
typedef struct JKLSector
{
	U32 flags;
	F32 ambient;
	F32 extra;
	U32 colormap;
	ColorF tint;
	Box3F bounds;
	Box3F collision_bounds;
	Point3F center;
	F32 radius;
	U32 startVerts;
	U32 endVerts;
	U32 startSurf;
	U32 endSurf;
};

typedef struct JKLSurface
{
	S32 mat;
	U32 surfflags;
	U32 faceflags;
	U32 geo;
	U32 light;
	U32 tex;
	S32 adjoin;
	F32 extralight;
	U32 startVerts;
	U32 endVerts;
	Point3F normal;
};

typedef struct JKLAdjoin
{
	U32 flags;
	U32 mirror;
	F32 dist;
};

class JKLLevel;
class JKLConvex;

class JKLFile : public ResourceInstance
{
	friend class JKLLevel;
	friend class JKLConvex;
	public:
		virtual ~JKLFile();
		bool load(Stream &s);
		bool save(Stream &s);

		void setBasePath(const char *path);
		
		static S32 version;
	protected:
		bool parse();
		void getBounds();
		Tokenizer *pTok;
		Vector<Point3F> verts;
		Vector<Point2F> tverts;
		/* NOTE: 
			Texture vertices are specified in texels and not dependant on texture size (unlike usual texture coordinates 
			accepted by 3d libraries). So to translate them to U,V accepted by OpenGl or Direct3D, you need to divide them by 
			the width/height of the texrure. IE: d3dU=u/texture_width; d3dV=v/texture_height; 
		*/
		Vector<U16> indices;
		Vector<U16> texindices; // texture vertex indices
		Box3F bounds; // Bounds of the whole jkl
		StringTableEntry basePath; // base path for textures
		
		Vector<JKLSector*> sectors;
		Vector<JKLAdjoin> adjoins;
		Vector<JKLSurface> surfaces;
		Vector<F32> intensities;
		//MaterialList *mats; // materials <-- IDEA: JKLMaterialList
		Vector<TextureHandle> mats; // materials
		enum {
			SECTION_NONE=0,
			SECTION_COPYRIGHT,
			SECTION_HEADER,
			SECTION_SOUNDS,
			SECTION_MATERIALS,
			SECTION_GEORESOURCE,
			SECTION_SECTORS,
			SECTION_AICLASS,
			SECTION_MODELS,
			SECTION_SPRITES,
			SECTION_KEYFRAMES,
			SECTION_ANIMCLASS,
			SECTION_SOUNDCLASS,
			SECTION_COGSCRIPTS,
			SECTION_COGS,
			SECTION_TEMPLATES,
			SECTION_THINGS,
			SECTION_ARCHLIGHTING,
			SUBSECTION_WORLD_VERTICES,
			SUBSECTION_WORLD_TEXTURE_VERTICES,
			SUBSECTION_WORLD_ADJOINS,
			SUBSECTION_WORLD_SURFACES,
			SUBSECTION_WORLD_SECTORS,
			SUBSECTION_WORLD_SURFACE_NORMALS,
			SUBSECTION_WORLD_MATERIALS,
		};
	

};

#endif

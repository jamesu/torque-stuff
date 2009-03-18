/*
	jklFile.cc
	
	Description :
		This file implements the JKLFile class to real .jkl files, as well as a derivation of the Tokenizer class.
	
	Author :
		James Urquhart (jamesu at gmail.com)

*/

#include "platform/platform.h"
#include "console/console.h"
#include "console/simBase.h"
#include "core/color.h"
#include "core/stream.h"
#include "core/fileStream.h"
#include "math/mMath.h"
#include "core/tokenizer.h"
#include "game/jklFile.h"

ResourceInstance *constructJKL(Stream &s)
{
	JKLFile *jkl = new JKLFile;
	if (jkl->load(s)) 
		return jkl;
	else {
		delete jkl;
		return NULL;
	}
}

class JKLTokenzier : public Tokenizer
{
bool advanceToken(const bool crossLine)
{
   U32 currPosition = 0;
   mCurrTokenBuffer[0] = '\0';

   while (mCurrPos < mBufferSize) {
      char c = mpBuffer[mCurrPos];

      bool cont = true;
      switch (c) { /* NOTE: Add token seperators here */
        case ' ':
        case '\t':
         if (currPosition == 0) {
            // Token hasn't started yet...
            mCurrPos++;
         } else {
            // End of token
            mCurrPos++;
            cont = false;
         }
         break;

        case '\r':
        case '\n':
		if (crossLine) {
			if (currPosition == 0) {
			// Haven't started getting token, but we're crossing lines...
			while (mpBuffer[mCurrPos] == '\r' || mpBuffer[mCurrPos] == '\n') {
				mCurrPos++;
				mCurrLine++;
			}
			} else {
			// Getting token, stop here, leave pointer at newline...
			cont = false;
			}
		} else {
			cont = false;
			break;
		}
		break;
	

        default:
         if (c == '\"') {
            // Quoted token#
            if (currPosition != 0)
              Con::errorf("Error, quotes MUST be at start of token.  Error: (line : %d)", getCurrentLine());

            U32 startLine = getCurrentLine();
            mCurrPos++;

            while (mpBuffer[mCurrPos] != '\"') {
               if (mCurrPos >= mBufferSize) 
                   Con::errorf("End of file before quote closed.  Quote started: (line : %d)", startLine);
               if (mpBuffer[mCurrPos] == '\n' or mpBuffer[mCurrPos] == '\r')
                    Con::errorf("End of line reached before end of quote.  Quote started: (line : %d)", startLine);     
               mCurrTokenBuffer[currPosition++] = mpBuffer[mCurrPos++];
            }

            mCurrPos++;
            cont = false;
         } else if ((c == '/' && mpBuffer[mCurrPos+1] == '/') or (c == '#')) {
            // Line quote...
            if (currPosition != 0)
	    	cont = false; // let crossLine determine on the next pass
               // continue to end of line
               while (mCurrPos < mBufferSize && (mpBuffer[mCurrPos] != '\n' && mpBuffer[mCurrPos] != '\r'))
                  mCurrPos++;
         } else {
            mCurrTokenBuffer[currPosition++] = c;
            mCurrPos++;
         }
         break;
      }

      if (cont == false)
         break;
   }

   mCurrTokenBuffer[currPosition] = '\0';
   
   return currPosition != 0;
}

};

S32 JKLFile::version = 2;

void JKLFile::getBounds() 
{
	bounds.min.x =  bounds.min.y = bounds.min.z = 0;
	bounds.max.x =  bounds.max.y = bounds.max.z = 0;
	for (int i=0;i<sectors.size();i++) {
		if (sectors[i]->bounds.min.x < bounds.min.x)
			bounds.min.x = sectors[i]->bounds.min.x;
		if (sectors[i]->bounds.min.y < bounds.min.y)
			bounds.min.y = sectors[i]->bounds.min.y;
		if (sectors[i]->bounds.min.z < bounds.min.z)
			bounds.min.z = sectors[i]->bounds.min.z;
		if (sectors[i]->bounds.max.x > bounds.max.x)
			bounds.max.x = sectors[i]->bounds.max.x;
		if (sectors[i]->bounds.max.y > bounds.max.y)
			bounds.max.y = sectors[i]->bounds.max.y;
		if (sectors[i]->bounds.max.z > bounds.max.z)
			bounds.max.z = sectors[i]->bounds.max.z;
	}
}

#define MAT_BUFF_SIZE 512

bool JKLFile::parse()
{
	U32 cur_section = SECTION_NONE;
	U32 sub_section = SECTION_NONE;
	U32 i, y; // Loopers
	U32 vert1;
	U32 vert2;
	U32 numVerts;
	U32 oldSize;
	bool ok;
	char *errorstr = "Warning : expected %s, but found %s";
	char matBuffer[MAT_BUFF_SIZE];

	while(pTok->advanceToken(true))
	{	
		if (pTok->tokenICmp("end")) {
			// End of Section
			cur_section = SECTION_NONE;
			sub_section = SECTION_NONE;
			continue;
		}
		else if (pTok->tokenICmp("SECTION:")) {
			// New Section
			pTok->advanceToken(false); // Get Section Name
			Con::printf("Switching Section: %s", pTok->getToken());
			/* Switch Section depending on name */
			if (pTok->tokenICmp("HEADER"))
				cur_section = SECTION_HEADER;
			else if (pTok->tokenICmp("SOUNDS")) 
				cur_section = SECTION_SOUNDS;
			else if (pTok->tokenICmp("MATERIALS")) 
				cur_section = SECTION_MATERIALS;
			else if (pTok->tokenICmp("GEORESOURCE")) 
				cur_section = SECTION_GEORESOURCE;
			else if (pTok->tokenICmp("AICLASS")) 
				cur_section = SECTION_AICLASS;
			else if (pTok->tokenICmp("MODELS")) 
				cur_section = SECTION_MODELS;
			else if (pTok->tokenICmp("SPRITES")) 
				cur_section = SECTION_SPRITES;
			else if (pTok->tokenICmp("KEYFRAMES"))
				cur_section = SECTION_KEYFRAMES; 
			else if (pTok->tokenICmp("ANIMCLASS")) 
				cur_section = SECTION_ANIMCLASS;
			else if (pTok->tokenICmp("SOUNDCLASS")) 
				cur_section = SECTION_SOUNDCLASS;
			else if (pTok->tokenICmp("COGSCRIPTS")) 
				cur_section = SECTION_COGSCRIPTS;
			else if (pTok->tokenICmp("COGS")) 
				cur_section = SECTION_COGS;
			else if (pTok->tokenICmp("THINGS")) 
				cur_section = SECTION_THINGS;
			else if (pTok->tokenICmp("TEMPLATES")) 
				cur_section = SECTION_TEMPLATES;
			else if (pTok->tokenICmp("SECTORS"))
				cur_section = SECTION_SECTORS;
			else if (pTok->tokenICmp("COPYRIGHT")) 
				cur_section = SECTION_COPYRIGHT;
			else
				cur_section = SECTION_NONE;
			
			// Reset section
			sub_section = SECTION_NONE;
			continue; // Skip reading rest for this line
		}
		
		/* Otherwise, just look what we have in the section */
		switch (cur_section)
		{
			case SECTION_HEADER:
			break;
			case SECTION_SOUNDS:
			break;
			case SECTION_MATERIALS:
				if (pTok->tokenICmp("World")) {
					pTok->advanceToken(false); // Get next word
					if (pTok->tokenICmp("Materials")) {
						pTok->advanceToken(false);
						mats.setSize(dAtoi(pTok->getToken()));
						sub_section = SUBSECTION_WORLD_MATERIALS;
					}
				}
			break;
			case SECTION_GEORESOURCE:
				/* Determine current subsection */
				if (pTok->tokenICmp("World")) {
					pTok->advanceToken(false); // Get next word
					if (pTok->tokenICmp("Vertices")) {
						pTok->advanceToken(false);
						sub_section = SUBSECTION_WORLD_VERTICES;
						verts.setSize(dAtoi(pTok->getToken()));
					}
					else if (pTok->tokenICmp("Texture")) {
						pTok->advanceToken(false);
						if (pTok->tokenICmp("Vertices")) {
							pTok->advanceToken(false);
							sub_section = SUBSECTION_WORLD_TEXTURE_VERTICES;
							tverts.setSize(dAtoi(pTok->getToken()));
						}
					}
					else if (pTok->tokenICmp("Adjoins")) {
						pTok->advanceToken(false);
						sub_section = SUBSECTION_WORLD_ADJOINS;
						adjoins.setSize(dAtoi(pTok->getToken()));
					}
					else if (pTok->tokenICmp("Surfaces")) {
						pTok->advanceToken(false);
						sub_section = SUBSECTION_WORLD_SURFACES;
						surfaces.setSize(dAtoi(pTok->getToken()));
					}
					else
						Con::warnf("Warning : no corresponding World section at %d", pTok->getCurrentLine());
				}
			break;
			case SECTION_SECTORS:
				if (pTok->tokenICmp("World")) {
					pTok->advanceToken(false);
					if (pTok->tokenICmp("Sectors")) {
						pTok->advanceToken(false);
						sub_section = SUBSECTION_WORLD_SECTORS;
						sectors.setSize(dAtoi(pTok->getToken()));
						Con::printf("Found Sector list : %d", sectors.size());
					}
					else
						Con::warnf("Warning : no corresponding World section at %d", pTok->getCurrentLine());
				}
			break;
			case SECTION_AICLASS:
			break;
			case SECTION_MODELS:
			break;
			case SECTION_SPRITES:
			break;
			case SECTION_KEYFRAMES:
			break;
			case SECTION_ANIMCLASS:
			break;
			case SECTION_SOUNDCLASS:
			break;
			case SECTION_COGSCRIPTS:
			break;
			case SECTION_COGS:
			break;
			case SECTION_TEMPLATES:
			break;
			case SECTION_THINGS:
			break;
			case SECTION_ARCHLIGHTING:
			break;
			case SECTION_COPYRIGHT:
			case SECTION_NONE:
			break;
		}
		/* Check subsections */
		switch (sub_section)
		{
			case SUBSECTION_WORLD_VERTICES:
				// Raw read in of vertexes
				for (i=0;i<verts.size();i++) {
					pTok->advanceToken(true);pTok->advanceToken(false);
					verts[i].x = dAtof(pTok->getToken());
					pTok->advanceToken(false);
					verts[i].y = dAtof(pTok->getToken());
					pTok->advanceToken(false);
					verts[i].z = dAtof(pTok->getToken());
				}
				sub_section = SECTION_NONE;
			break;
			case SUBSECTION_WORLD_TEXTURE_VERTICES:
				// Raw read in of texture vertexes
				for (i=0;i<tverts.size();i++) {
					pTok->advanceToken(true);pTok->advanceToken(false);
					tverts[i].x = dAtof(pTok->getToken());
					pTok->advanceToken(false);
					tverts[i].y = dAtof(pTok->getToken());
				}
				sub_section = SECTION_NONE;
			break;
			case SUBSECTION_WORLD_MATERIALS:
				//#num:   filename,  scalex, scaley
				for (i=0;i<mats.size();i++) {
					// Now parse
					pTok->advanceToken(true);
					// first, check for abrupt ending to the list...
					if (pTok->tokenICmp("end"))
					{
						Con::printf("NOTE: Abrupt end to material list");
						sub_section = SECTION_NONE;
						break;
					}
					pTok->advanceToken(false); // else continue
					// construct texture since vector doesn't do this for us
					constructInPlace(&mats[i]);
					// now load it
					dSprintf(matBuffer, MAT_BUFF_SIZE, "%s/%s", /*basePath*/ "starter.fps/level/", pTok->getToken());
					mats[i] = TextureHandle(matBuffer, InteriorTexture);
					if (!mats[i]) {
						//Con::warnf("Warning: could not load %s", pTok->getToken());
						mats[i] = TextureHandle("common/level/dflt", InteriorTexture);
					}
					// Skip the rest
					pTok->advanceToken(false);
					pTok->advanceToken(false);
				}
				sub_section = SECTION_NONE;
			break;
			case SUBSECTION_WORLD_ADJOINS:
				//#num:   flags:  mirror:  dist:
				for (i=0;i<adjoins.size();i++) {
					pTok->advanceToken(true);pTok->advanceToken(false);
					adjoins[i].flags = dAtoi(pTok->getToken()); 
					pTok->advanceToken(false);
					adjoins[i].mirror = dAtoi(pTok->getToken()); 
					pTok->advanceToken(false);
					adjoins[i].dist = dAtof(pTok->getToken());
				}
				sub_section = SECTION_NONE;
				
			break;
			case SUBSECTION_WORLD_SURFACES:
				for (i=0;i<surfaces.size();i++) {
					pTok->advanceToken(true);pTok->advanceToken(false);
					// We have a surface
					surfaces[i].mat = dAtoi(pTok->getToken());		pTok->advanceToken(false);
					surfaces[i].surfflags = dAtoi(pTok->getToken());	pTok->advanceToken(false);
					surfaces[i].faceflags = dAtoi(pTok->getToken());	pTok->advanceToken(false);
					surfaces[i].geo = dAtoi(pTok->getToken());		pTok->advanceToken(false);
					surfaces[i].light = dAtoi(pTok->getToken());		pTok->advanceToken(false);
					surfaces[i].tex = dAtoi(pTok->getToken());		pTok->advanceToken(false);
					surfaces[i].adjoin = dAtoi(pTok->getToken());		pTok->advanceToken(false);
					surfaces[i].extralight = dAtof(pTok->getToken());	pTok->advanceToken(false);
					surfaces[i].startVerts = oldSize = indices.size();
					numVerts = dAtoi(pTok->getToken()); // number of indices
					indices.increment(numVerts);
					texindices.increment(numVerts);
					intensities.increment(numVerts);
					for (y=0;y<numVerts;y++) {
						// Read in vertex indices
						pTok->advanceToken(false);
						dSscanf(pTok->getToken(), "%d,%d", &vert1, &vert2);
						indices[oldSize + y] = vert1;
						texindices[oldSize + y] = vert2;
					}
					surfaces[i].endVerts = indices.size();
					
					for (y=0;y<numVerts;y++) {
						// Read intensities
						pTok->advanceToken(false);
						intensities[oldSize + y] = dAtof(pTok->getToken());
					}
					
				}
				// Now read in normals
				for (i=0;i<surfaces.size();i++) {
					pTok->advanceToken(true);pTok->advanceToken(false);
					surfaces[i].normal.x = dAtof(pTok->getToken());	pTok->advanceToken(false);
					surfaces[i].normal.y = dAtof(pTok->getToken());	pTok->advanceToken(false);
					surfaces[i].normal.z = dAtof(pTok->getToken());
				}
				sub_section = SECTION_NONE;
				
			break;
			case SUBSECTION_WORLD_SECTORS:
				if (pTok->tokenICmp("SECTOR")) {
					/* Read in details of sector */
					pTok->advanceToken(false);y = dAtoi(pTok->getToken());
					sectors[y] = new JKLSector;
					
					ok = true;
					while (ok) {
						pTok->advanceToken(true);
					// FLAGS
					if (pTok->tokenICmp("FLAGS")) {
						pTok->advanceToken(false);
						sectors[y]->flags = dAtoi(pTok->getToken());
					}
					// AMBIENT LIGHT
					else if (pTok->tokenICmp("AMBIENT")) {
						pTok->advanceToken(false);
						if (pTok->tokenICmp("LIGHT")) {
							pTok->advanceToken(false);
							sectors[y]->ambient = dAtof(pTok->getToken());
						}
						else
							Con::errorf(errorstr, "LIGHT", pTok->getToken());
					}
					// EXTRA LIGHT
					else if (pTok->tokenICmp("EXTRA")) {
						pTok->advanceToken(false);
						if (pTok->tokenICmp("LIGHT")) {
							pTok->advanceToken(false);
							sectors[y]->extra = dAtof(pTok->getToken());
						}
						else
							Con::errorf(errorstr, "LIGHT", pTok->getToken());
					}
					// COLORMAP
					else if (pTok->tokenICmp("COLORMAP")) {
						pTok->advanceToken(false);
						sectors[y]->colormap = dAtoi(pTok->getToken());
					}
					// TINT
					else if (pTok->tokenICmp("TINT")) {
						pTok->advanceToken(false);
						sectors[y]->tint.red = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->tint.green = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->tint.blue = dAtof(pTok->getToken());
					}
					// BOUNDBOX
					else if (pTok->tokenICmp("BOUNDBOX")) {
						pTok->advanceToken(false);
						sectors[y]->bounds.min.x = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->bounds.min.y = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->bounds.min.z = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->bounds.max.x = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->bounds.max.y = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->bounds.max.z = dAtof(pTok->getToken());
					}
					// COLLIDEBOX
					else if (pTok->tokenICmp("COLLIDEBOX")) {
						pTok->advanceToken(false);
						sectors[y]->collision_bounds.min.x = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->collision_bounds.min.y = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->collision_bounds.min.z = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->collision_bounds.max.x = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->collision_bounds.max.y = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->collision_bounds.max.z = dAtof(pTok->getToken());
					}
					// SOUND
					else if (pTok->tokenICmp("SOUND")) {
						pTok->advanceToken(false);pTok->advanceToken(false); // skip
					}
					// CENTER
					else if (pTok->tokenICmp("CENTER")) {
						pTok->advanceToken(false);
						sectors[y]->center.x = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->center.y = dAtof(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->center.z = dAtof(pTok->getToken());
					}
					// RADIUS
					else if (pTok->tokenICmp("RADIUS")) {
						pTok->advanceToken(false);
						sectors[y]->radius = dAtof(pTok->getToken());
					}
					// VERTICES
					else if (pTok->tokenICmp("VERTICES")) {
						pTok->advanceToken(false);
						sectors[y]->startVerts = oldSize = indices.size();
						numVerts = dAtoi(pTok->getToken());
						indices.increment(numVerts);
						for (i=0;i<numVerts;i++) {
							pTok->advanceToken(true);pTok->advanceToken(false);
							indices[oldSize + i] = dAtoi(pTok->getToken());
						}
						sectors[y]->endVerts = indices.size();
					}
					// SURFACES
					else if (pTok->tokenICmp("SURFACES")) {
						pTok->advanceToken(false);
						sectors[y]->startSurf = dAtoi(pTok->getToken());pTok->advanceToken(false);
						sectors[y]->endSurf = sectors[y]->startSurf + dAtoi(pTok->getToken());
						ok = false; // Finished sector
					}
					else {
						Con::errorf(errorstr, "Sector Variable", pTok->getToken());
						ok = false;
					}
					}
				}
			break;
			case SECTION_NONE:
			default:
				/* Do nothing */
			break;
		}
		
	}
	return true; // Presumed success
}

void JKLFile::setBasePath(const char *path)
{
	basePath = path;
}

bool JKLFile::load(Stream &s)
{
	pTok = new JKLTokenzier;
	pTok->openStream((Stream*)&s);
	// TODO: !! We need to get the base path!!
	bool res = parse();
	delete pTok;
	getBounds(); // calculate bounds
	return res;
}

bool JKLFile::save(Stream &s)
{
	return false;
}

JKLFile::~JKLFile()
{
	verts.clear();
	tverts.clear();
	indices.clear();
	surfaces.clear();
	intensities.clear();
	texindices.clear();
	for (U32 i=0;i<sectors.size();i++)
		delete sectors[i];
	sectors.clear();
	basePath = NULL;
}


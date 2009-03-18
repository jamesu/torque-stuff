/*
	jklLevel.cc
	
	Description :
		This class draws Jedi Knight Levels (.jkl) from JklFile resources.
	
	Author :
		James Urquhart (jamesu at gmail.com)

*/
// Derived from SimplestObject by Matthew Fairfax (matt@rustycode.com)

#include "game/jklFile.h"
#include "game/jklLevel.h"

#include "dgl/dgl.h"
#include "core/bitStream.h"
#include "game/gameConnection.h"
#include "math/mathIO.h"
#include "core/fileStream.h"
#include "console/consoleTypes.h"
#include "platform/platformAssert.h"

#define FRONTEPSILON 0.00001

//#define COLOR_TEST // Overrides textures

//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(JKLLevel);

// Return the bounding box transformed into world space
Box3F JKLConvex::getBoundingBox() const
{
	return getBoundingBox(mObject->getTransform(), mObject->getScale());
}

// Transform and scale the bounding box by the inputs
Box3F JKLConvex::getBoundingBox(const MatrixF& mat, const Point3F& scale) const
{
	Box3F newBox = box;
	newBox.min.convolve(scale);
	newBox.max.convolve(scale);
	mat.mul(newBox);
	return newBox;
}

// Return the point furthest from the input vector
Point3F JKLConvex::support(const VectorF& v) const
{
	Point3F ret(0.0f, 0.0f, 0.0f);
	F32 dp = 0.0f;
	F32 tp = 0.0f;
	JKLFile *oLevel = pOwner->level;

	// Loop through the points and save the furthest one
	for (U32 vi = oLevel->sectors[sector]->startVerts;vi < oLevel->sectors[sector]->endVerts;vi++) {
		tp = mDot(v,  oLevel->verts[oLevel->indices[vi]]);
		if (tp > dp)
		{
			dp = tp;
			ret = oLevel->verts[oLevel->indices[vi]];
		}
	}
	return ret;
}

// Return the vertices, faces, and edges of the convex
void JKLConvex::getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf)
{
	// Fill in a few details
	cf->material = 0;
	cf->object   = mObject;
	
	JKLFile *oLevel = pOwner->level;
	PlaneF plane;
	U32 pvert[4];
	pvert[3] = 0;

	// Get the starting offset
	S32 base = cf->mVertexList.size();
	// Add collision indices
	for (U32 i = oLevel->sectors[sector]->startVerts; i < oLevel->sectors[sector]->endVerts; i++)
	{
		cf->mVertexList.increment();
		mat.mulP(oLevel->verts[oLevel->indices[i]], &cf->mVertexList.last());
	}
	
	// Insert Verts
	for (U32 s=oLevel->sectors[sector]->startSurf;
		 s<oLevel->sectors[sector]->endSurf;s++) {
		if (oLevel->surfaces[s].adjoin != -1) 
			continue; // if we have an adjoin surface, don't check
		
		plane = PlaneF(oLevel->verts[oLevel->indices[oLevel->surfaces[s].startVerts]], oLevel->surfaces[s].normal);
		// Insert faces
		if(mDot(plane, n) > FRONTEPSILON)
		{
			// Get Face list...
			pvert[3] = 0;
			for (U32 vert=oLevel->sectors[sector]->startVerts;vert<oLevel->sectors[sector]->endVerts;vert++)
			{
			// We need to get indices from the collision indices
				for (U32 svert=oLevel->surfaces[s].startVerts;svert<oLevel->surfaces[s].endVerts;svert++)
				{
					if (oLevel->indices[vert] == oLevel->indices[svert]) // indices match
					{
						if (pvert[3] < 3) {
							pvert[pvert[3]] = base+(vert-oLevel->sectors[sector]->startVerts);
							pvert[3]++;
						}
						else
							break; // need no more verts
					}
				}
			}
			if (pvert[3] < 3)
				break; // didn't get all the verts?
			// Now add the face...
			cf->mFaceList.last().vertex[0] = pvert[0];
			cf->mFaceList.last().vertex[1] = pvert[1];
			cf->mFaceList.last().vertex[2] = pvert[2];
			//cf->mEdgeList.increment();
			//cf->mEdgeList.last().vertex[0] = 0; // ..
			//cf->mEdgeList.last().vertex[1] = 0; // ..
		}
	}
		
}


// Return list(s) of convex faces
void JKLConvex::getPolyList(AbstractPolyList* list)
{
	// Be sure to transform the list into model space
	list->setTransform(&mObject->getTransform(), mObject->getScale());
	list->setObject(mObject);
	PlaneF surf_plane;
	
	JKLFile *oLevel = pOwner->level;
	
	// First add the first point and get the starting offset for these points in the list
	U32 base = list->addPoint(oLevel->verts[oLevel->indices[oLevel->sectors[sector]->startVerts]]);

	// Now add the other points
	for (U32 i = oLevel->sectors[sector]->startVerts+1; i < oLevel->sectors[sector]->endVerts; i++)
		list->addPoint(oLevel->verts[oLevel->indices[i]]);
		
	bool initial = true;
	U32 planeBase;
	// And add planes
	for (U32 surf=oLevel->sectors[sector]->startSurf;surf<oLevel->sectors[sector]->endSurf;surf++)
	{
		surf_plane = PlaneF(oLevel->verts[oLevel->indices[oLevel->surfaces[surf].startVerts]], oLevel->surfaces[surf].normal);
		if (initial) {
			planeBase = list->addPlane(surf_plane);
			initial = false;
		}
		else
			list->addPlane(surf_plane);
	}
	
	U32 sf = 0;
	U32 pvert[4]; // vertexes that make up plane
	// Now, add the surfaces...
	for (U32 surf=oLevel->sectors[sector]->startSurf;surf<oLevel->sectors[sector]->endSurf;surf++)
	{
		if (oLevel->surfaces[surf].adjoin != -1)
			continue; // skip surfaces that have adjoins
		pvert[3] = 0;
		// Get correct vertexes to make a plane for the surface
		for (U32 svert=oLevel->surfaces[surf].startVerts;svert<oLevel->surfaces[surf].endVerts;svert++)
		{	
			for (U32 vert=oLevel->sectors[sector]->startVerts;vert<oLevel->sectors[sector]->endVerts;vert++)
			{
				if (oLevel->indices[vert] == oLevel->indices[svert])
				{
					// If we found an indicie in the sector vertexes that matches...
					if (pvert[3] == 0) { // start list if not started
						list->begin(0, sf++);
					}
					if (pvert[3] < 3) {
						pvert[pvert[3]] = base+(vert-oLevel->sectors[sector]->startVerts);
						pvert[3]++;
					}
					// Add vertex for this surface
					list->vertex(base + (vert-oLevel->sectors[sector]->startVerts));
					break; // now find next surface vert
				}
				
			}
		}
		// If we have no plane, try to determine if we missed making one
		AssertFatal(pvert[3] == 3, avar("Interior: Could not make plane for surface %d in sector %d!", surf, sector));
		AssertFatal(pvert[3] != 0, "Interior: Error. List never started!");
		
		//Con::printf("PLANE: %d %d %d", pvert[0], pvert[1], pvert[2]);
		list->plane(planeBase + (surf-oLevel->sectors[sector]->startSurf));
		list->end(); // done surface
	}
}

JKLLevel::JKLLevel(void)
{
	// Setup NetObject.
	mTypeMask = StaticObjectType | StaticRenderedObjectType | StaticTSObjectType;
	mNetFlags.set(Ghostable);

	// Haven't loaded yet
	mLoaded = false;
	level = NULL;

	// Give it a nonexistant bounding box
	mObjBox.min.set(0, 0, 0);
	mObjBox.max.set(0, 0, 0);
}

JKLLevel::~JKLLevel(void)
{
}

void JKLLevel::initPersistFields()
{
	// Initialise parents' persistent fields.
	Parent::initPersistFields();
	addGroup("Data");
	addField("levelFile",       TypeFilename,  Offset(mFileName, JKLLevel));
	endGroup("Data");
}

bool JKLLevel::onAdd()
{
	if(!Parent::onAdd()) return(false);

	// Load our object
	mLoaded = load();

	// Reset the World Box.
	resetWorldBox();
	// Set the Render Transform.
	setRenderTransform(mObjToWorld);

	// Add to Scene.
	addToScene();

	// Return OK.
	return(true);
}

bool JKLLevel::load()
{
   F32 size = 3.0f;
	// Load the JKL
	level = ResourceManager->load(mFileName);
	if (bool(level) == false) {
		return false;
	}
	
	// Set the bounds
	mObjBox.min.set(1e10, 1e10, 1e10);
	mObjBox.max.set(-1e10, -1e10, -1e10);
	
	
	mObjBox.min.setMin(level->bounds.min);
	mObjBox.max.setMax(level->bounds.max);

   return true;
}

void JKLLevel::onRemove()
{
	// Remove from Scene.
	removeFromScene();
	//if (level != NULL)
	//	level->close();
	//level = NULL;
	//ResourceManager->close(level);

	// Do Parent.
	Parent::onRemove();
}

void JKLLevel::inspectPostApply()
{
	// Set Parent.
	Parent::inspectPostApply();

	// Set fxPortal Mask.
	setMaskBits(JKLLevelMask);
}

//------------------------------------------------------------------------------

void JKLLevel::onEditorEnable()
{
}

//------------------------------------------------------------------------------

void JKLLevel::onEditorDisable()
{
}

bool JKLLevel::prepRenderImage(	SceneState* state, const U32 stateKey, const U32 startZone,
										const bool modifyBaseZoneState)
{
	// Return if last state.
	if (isLastState(state, stateKey)) return false;
	// Set Last State.
	setLastState(state, stateKey);

   // Is Object Rendered?
   if (state->isObjectRendered(this))
   {	   
		// Yes, so get a SceneRenderImage.
		SceneRenderImage* image = new SceneRenderImage;
		// Populate it.
		image->obj = this;
		image->isTranslucent = false;
		image->sortType = SceneRenderImage::Normal;
		
		// Insert it into the scene images.
		state->insertRenderImage(image);
   }

   return false;
}

void JKLLevel::renderObject(SceneState* state, SceneRenderImage*)
{
	// Check we are in Canonical State.
	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

	// Save state.
	RectI viewport;

	// Save Projection Matrix so we can restore Canonical state at exit.
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	// Save Viewport so we can restore Canonical state at exit.
	dglGetViewport(&viewport);

	// Setup the projection to the current frustum.
	//
	// NOTE:-	You should let the SceneGraph drive the frustum as it
	//			determines portal clipping etc.
	//			It also leaves us with the MODELVIEW current.
	//
	state->setupBaseProjection();

	// Save ModelView Matrix so we can restore Canonical state at exit.
	glPushMatrix();

	// Transform by the objects' transform e.g move it.
	dglMultMatrix(&getTransform());

	glScalef(mObjScale.x, mObjScale.y, mObjScale.z);

	bool debugCollision = false;

	// I separated out the actual render function to make this a little cleaner
	render();

	// Restore our canonical matrix state.
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	// Restore our canonical viewport state.
	dglSetViewport(viewport);

	// Check we have restored Canonical State.
	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

bool JKLLevel::render(void)
{
	#ifdef COLOR_TEST
	glEnable(GL_COLOR_MATERIAL);
	#else
	glEnable(GL_TEXTURE_2D);
	#endif

	/* Render ALL surfaces */
	for (int s=0;s<level->sectors.size();s++) {
		/*if (!level->sectors[s]->outside)
		{
			// Check if we are in
			// If we are, start rendering from this point
		}*/
		
		
		for (int i=level->sectors[s]->startSurf;i<level->sectors[s]->endSurf;i++) {
			if (level->surfaces[i].adjoin != -1) 
				continue; // if we have an adjoin surface, don't draw
			#ifndef COLOR_TEST
			glBindTexture(GL_TEXTURE_2D,  level->mats[level->surfaces[i].mat].getGLName());
			#endif

			glBegin(GL_TRIANGLE_FAN);
			U32 texW = level->mats[level->surfaces[i].tex].getWidth();
			U32 texH = level->mats[level->surfaces[i].tex].getHeight();
			
			// TODO: use vertex arrays
			glNormal3f(level->surfaces[i].normal.x, level->surfaces[i].normal.y, level->surfaces[i].normal.z);
			for (int v=level->surfaces[i].startVerts;v<level->surfaces[i].endVerts;v++) {
				glTexCoord2f(level->tverts[level->texindices[v]].x / texW, level->tverts[level->texindices[v]].y / texH);
				glVertex3f(level->verts[level->indices[v]].x,level->verts[level->indices[v]].y,level->verts[level->indices[v]].z);
				#ifdef COLOR_TEST
				glColor3f(level->intensities[v],level->intensities[v],level->intensities[v]);
				#endif
			}
			glEnd();
		}
	}

	#ifdef COLOR_TEST
	glDisable(GL_COLOR_MATERIAL);
	#else
	glDisable(GL_TEXTURE_2D);
	#endif

	return true;
}

U32 JKLLevel::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
	// Pack Parent.
	U32 retMask = Parent::packUpdate(con, mask, stream);

	// Write fxPortal Mask Flag.
	if (stream->writeFlag(mask & JKLLevelMask))
	{
		// Write Object Transform.
		stream->writeAffineTransform(mObjToWorld);
		stream->writeString(mFileName);

      // Write Object Scale.
      mathWrite(*stream, mObjScale);
	}

	// Were done ...
	return(retMask);
}

//------------------------------------------------------------------------------

void JKLLevel::unpackUpdate(NetConnection * con, BitStream * stream)
{
	// Unpack Parent.
	Parent::unpackUpdate(con, stream);

	// Read fxPortal Mask Flag.
	if(stream->readFlag())
	{
		MatrixF		ObjectMatrix;
      Point3F     scale;

		// Read Object Transform.
		stream->readAffineTransform(&ObjectMatrix);
		// Set Transform.
		setTransform(ObjectMatrix);
		mFileName = stream->readSTString();

      // Read Object Scale
      mathRead(*stream, &scale);
      // Set Scale
      setScale(scale);

		// Reset the World Box.
		resetWorldBox();
		// Set the Render Transform.
		setRenderTransform(mObjToWorld);
	}
}

void JKLLevel::buildConvex(const Box3F& box, Convex* convex)
{
	// Transform the incoming box into model space
	Box3F realBox = box;
	mWorldToObj.mul(realBox);
	realBox.min.convolveInverse(mObjScale);
	realBox.max.convolveInverse(mObjScale);

	U16 sectorList[MAX_CONVEX_SECTORS];
	JKLConvex *sectorConvexes[MAX_CONVEX_SECTORS];
	U8 numSectorList = 0;
	// Box that encompassed sectors in convex
	Box3F calcBox = mObjBox;
	U32 i;

	// Check to make sure that the boxes overlap
	if (realBox.isOverlapped(calcBox))
	{
		// Get List of sectors in box
		for (U32 s=0;s<level->sectors.size();s++)
		{
			if (realBox.isOverlapped(level->sectors[s]->bounds))
			{
				if (numSectorList >= MAX_CONVEX_SECTORS) {
					Con::warnf("Interior: Sector overflow!");
					break;
				}
				sectorList[numSectorList] = s;
				sectorConvexes[numSectorList] = NULL; // default NULL
				numSectorList++;
			}
		}
		if (numSectorList == 0) return; // no sectors, nothing to do
		// See if these convexes already exist
		Convex* cc = 0;
		CollisionWorkingList& wl = convex->getWorkingList();
		for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
		{
			if (itr->mConvex->getType() == JKLConvexType &&
				static_cast<JKLConvex*>(itr->mConvex)->getObject() == this)
			{
				for (i=0;i<numSectorList;i++)
				{	
					if (sectorList[i] == static_cast<JKLConvex*>(itr->mConvex)->sector)
					{
						sectorConvexes[i] = (JKLConvex*)itr->mConvex;
						break;
					}
				}
			}
		}
		
		// Now handle the convexes we don't have
		for (i=0;i<numSectorList;i++)
		{
			if (sectorConvexes[i] == NULL)
			{
				sectorConvexes[i] = new JKLConvex;
				sectorConvexes[i]->sector = sectorList[i];
				sectorConvexes[i]->mObject = this;
				sectorConvexes[i]->pOwner = this;
				sectorConvexes[i]->box = level->sectors[sectorList[i]]->bounds;
				convex->addToWorkingList(sectorConvexes[i]);
			}
		}
	}
}

bool JKLLevel::castRay(const Point3F& s, const Point3F& e, RayInfo* info)
{
	F32 test;
	F32 out = 1.0f;
	VectorF norm;
	PlaneF top;
	U32 faceNo=0;

	// TODO: fix! not working correctly!
	
	for (U32 sc=0;sc<level->sectors.size();sc++)
	{
		if (!level->sectors[sc]->bounds.collideLine(s,e))
			continue; //skip
		for (U32 surf=level->sectors[sc]->startSurf;surf<level->sectors[sc]->endSurf;surf++)
		{
		if (level->surfaces[surf].adjoin != -1)
			continue; // skip
			
		// Use surface normal to calculate collision
		// We need bounding boxes to determine correct sector
		top = PlaneF(level->verts[level->indices[level->surfaces[surf].startVerts]], level->surfaces[surf].normal);
		
		test = top.intersect(s, e);
		// If we collided and it is nearer than previous collisions save this info
		if (test >= 0.0f && test < 1.0f && test < out)
		{
			out = test;
			norm = top;
			faceNo = surf;
		}
		}
	}

	// If we had a collision fill in the info structure and return
	if (out >= 0.0f && out < 1.0f)
	{
		info->t = out;
		info->normal = norm;
		info->point.interpolate(s, e, out);
		info->face = faceNo; // TODO: do we need to set?
		info->object = this;

		return true;
	}

	// Otherwise we didn't collide
	return false;
}

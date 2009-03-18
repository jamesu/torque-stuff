/*
	jklLevel.h
	
	Description :
		This file declares the JKLLevel class, which renders .jkl levels loaded via the JklFile class.
	
	Author :
		James Urquhart (jamesu at gmail.com)

*/

#ifndef _JKLLevel_H_
#define _JKLLevel_H_

#include "platform/platform.h"
#include "sim/sceneObject.h"
#include "collision/convex.h"
#ifndef _JKLFILE_H_
#include "game/jklFile.h"
#endif

class JKLLevel;

// Max number of sectors to produce convexes of in one shot
#define MAX_CONVEX_SECTORS 6

// This is the convex collision implementation for JKLLevel
// Once something has collided against a JKLLevel then it creates one of
// these for the actual collisions to occur against
class JKLConvex : Convex
{
	typedef Convex Parent;
	friend class JKLLevel;  // So the "owner" object can set some properties when it creates this
public:
	// This is where you set the convex type
	// Needs to be defined in convex.h in the ConvexType enum
	JKLConvex() { mType = JKLConvexType;};
	~JKLConvex() {};

protected:
	Box3F box;                 // The bounding box of the convex (in object space)
	JKLLevel* pOwner;    // A pointer back to the "owner" object so we can reference the vertices
	U16 sector;

public:
	// Return the bounding box transformed into world space
	Box3F getBoundingBox() const;
	// Return the bounding box transformed and scaled by the input values
	Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;

	// This returns a list of convex faces to collide against
	void    getPolyList(AbstractPolyList* list);

	// This returns the furthest point from the input vector
	Point3F support(const VectorF& v) const;

	// Returns the vertices, faces, and edges of our convex
	void    getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf);
};

class JKLLevel : public SceneObject
{
	friend class JKLConvex;  // So the "child" convex(es) can reference the vertices

public:
	typedef SceneObject		Parent;

	// Declare Console Object.
	DECLARE_CONOBJECT(JKLLevel);
protected:

	// Create and use these to specify custom events.
	//
	// NOTE:-	Using these allows you to group the changes into related
	//			events.  No need to change everything if something minor
	//			changes.  Only really important if you have *lots* of these
	//			objects at start-up or you send alot of changes whilst the
	//			game is in progress.
	//
	//			Use "setMaskBits(fxRenderObjectMask)" to signal.

	enum {	JKLLevelMask	= (1 << 0),
		JKLLevelAnother	= (1 << 1) };

	bool			mLoaded;       // Make sure we don't load this twice
	Point3F			mPoints[8];    // Our only data - a list of vertices
	Resource<JKLFile>	level;

	// I split the render and load functions out of renderObject() and onAdd() for clarity
	bool render(void);
	bool load(void);
public:
	// Creation and destruction
	JKLLevel(void);
	~JKLLevel(void);

	// SceneObject
	void renderObject(SceneState*, SceneRenderImage*);
	virtual bool prepRenderImage(SceneState*, const U32 stateKey, const U32 startZone,
								const bool modifyBaseZoneState = false);

	// SimObject      
	bool onAdd();
	void onRemove();
	void onEditorEnable();
	void onEditorDisable();
	void inspectPostApply();

	// NetObject
	U32 packUpdate(NetConnection *, U32, BitStream *);
	void unpackUpdate(NetConnection *, BitStream *);

	// ConObject.
	static void initPersistFields();
	StringTableEntry mFileName;

	// Collision
	// castRay returns the percentage along the line from a starting and an ending point
	// to where something collides with the object
	bool castRay(const Point3F&, const Point3F&, RayInfo*);

	// Called whenever something overlaps our bounding box
	// This is where the sceneobject can submit convexes to be collided against
	void buildConvex(const Box3F& box, Convex* convex);
};

#endif

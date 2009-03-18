#ifndef _GRAVITYSOLVER_H_
#define _GRAVITYSOLVER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

class PhysicalZone;

class GravitySolver
{
	friend class PhysicalZone;

public:

	enum Type
	{
		GRAVITY_STANDARD    = 0,	// Fixed axis with constant acceleration
		GRAVITY_DIRECTION,		// Fixed axis with center of gravity
		GRAVITY_CENTRAL,		// Variable axis with center of gravity
		GRAVITY_CYLINDRICAL,		// Semi-variable axis with center of gravity
		GRAVITY_NUMTYPES ,
	};

	typedef struct
	{
		Point3F rDirection;	// Defines the up vector
		F32     rAcceleration;	// Acceleration per second/tick
		F32     rVelocity;		// Note: max velocity (m/s) - really needed?
	} Result;

	GravitySolver();
	GravitySolver(F32 gravity);	// Classical straight down

	void		defaults();
	void		update(F32 hotspot, F32 strength);
	void		set(GravitySolver::Type type, F32 falloff, F32 hotspot, F32 strength, Point3F position, Point3F direction);

	inline	F32 strengthFromDistance(F32 dist) {return mClampF(mStep * (mFalloff - dist), 0.0f, mMaxAcceleration);}
	void		getForces(Point3F point, GravitySolver::Result &forces);
	void		addForces(Point3F point, GravitySolver::Result &forces); // Adds ontop existing results

protected:

	GravitySolver::Type mType;

	F32 mFalloff;
	Point3F mPosition;
	Point3F mDirection;

	// Calculated values (on set())
	F32 mMaxAcceleration;
	F32 mStep;
	F32 mSign; // + or -?

public:
	static GravitySolver smDefaultGravity;
	static F32 smDefaultGravityStrength;
	static F32 smGravityOrientRate; // Time in seconds in which objects orient to gravity

	static void calculateOrient(GravitySolver::Result gravity, QuatF& orient);
	static void calculateOrient(GravitySolver::Result gravity, AngAxisF& orient);
};

#endif
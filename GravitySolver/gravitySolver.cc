#include "platform/platform.h"
#include "math/mMath.h"
#include "game/gravitySolver.h"
#include "console/console.h"

#define PLUSORMINUS(x) x >= 0.0f ? 1.0f : -1.0f;
GravitySolver GravitySolver::smDefaultGravity(-50);
F32           GravitySolver::smDefaultGravityStrength = 50;
F32           GravitySolver::smGravityOrientRate = 1.0 / 5.0;

// ====================================================================================

ConsoleFunction(setStandardGravity, void, 3, 3, "(strength, direction)")
{
	F32 in_x, in_y = 0;
	F32 in_z = -1;

	dSscanf(argv[2], "%f %f %f", &in_x, &in_y, &in_z);
	GravitySolver::smDefaultGravity.set(GravitySolver::GRAVITY_STANDARD, 0, 0, dAtof(argv[1]), Point3F(0,0,0), Point3F(in_x, in_y, in_z));
}

ConsoleFunction(setCentralGravity, void, 1, 1, "")
{
	GravitySolver::smDefaultGravity.set(GravitySolver::GRAVITY_CENTRAL, 10, 10, 10, Point3F(0,0,0), Point3F(0,0,0));
}

// ====================================================================================

GravitySolver::GravitySolver()
{
	mType = GravitySolver::GRAVITY_STANDARD;
	mPosition.set(0,0,0);
	mDirection.set(0,0,-1);
	
	mFalloff = 0;
	mSign = -1.0;
	mStep = 1.0;
	mMaxAcceleration = 80;
}

GravitySolver::GravitySolver(F32 gravity)
{
	set(GRAVITY_STANDARD, 0, 0, gravity, Point3F(0,0,0), Point3F(0,0,1));
}

// ====================================================================================

void GravitySolver::defaults()
{
	mType = smDefaultGravity.mType;
	mPosition = smDefaultGravity.mPosition;
	mDirection = smDefaultGravity.mDirection;

	mSign = smDefaultGravity.mSign;
	mStep = smDefaultGravity.mStep;
	mMaxAcceleration = smDefaultGravity.mMaxAcceleration;
	mFalloff = smDefaultGravity.mFalloff;
}

void GravitySolver::set(GravitySolver::Type type, F32 falloff, F32 hotspot, F32 strength, Point3F position, Point3F direction)
{
	mType = type;
	mDirection = direction;
	mDirection.normalize();
	mPosition = position;
	mFalloff = falloff;

	update(hotspot, strength);
}

void GravitySolver::update(F32 hotspot, F32 strength)
{
	mSign = PLUSORMINUS(strength);

	// Calculate values
	if (mType == GRAVITY_STANDARD) {
		mMaxAcceleration = strength;
		mStep = 1.0;
	} else {
		mMaxAcceleration = mFabs(30 * strength);
		mStep = mMaxAcceleration / (mFalloff - hotspot);
	}
}

void GravitySolver::getForces(Point3F point, GravitySolver::Result &forces)
{
	F32 dist;
	Point3F delta;

	switch (mType)
	{
	case GRAVITY_DIRECTION:
		forces.rDirection = mDirection * mSign;

		dist = mDot((point - mPosition), forces.rDirection);
		forces.rAcceleration = strengthFromDistance(dist);

		forces.rVelocity = smDefaultGravityStrength;
	break;

	case GRAVITY_CENTRAL:
		// out - in, +strength = repell, -strength = attract
		//forces.rDirection = (point - mPosition) * mSign; // (mPosition - point) * mSign;
		forces.rDirection = (mPosition - point) * mSign; // (mPosition - point) * mSign;
		
		dist = forces.rDirection.len();
		if (dist > 0.001f) forces.rDirection /= dist;
		forces.rAcceleration = strengthFromDistance(dist) * mSign;

		if (forces.rAcceleration != 0)
		{
			int fuck = 0;
		}

		forces.rVelocity = smDefaultGravityStrength;
	break;

	case GRAVITY_CYLINDRICAL:
		delta = mPosition - point;
		delta.normalize();

		forces.rDirection = (delta - mDirection) * mDot(delta, mDirection);//getNormalComponent(delta, mDirection);
		forces.rDirection *= mSign;

		dist = forces.rDirection.len();
		if (dist > 0.001f) forces.rDirection /= dist;
		forces.rAcceleration = strengthFromDistance(dist);
		
		forces.rVelocity = smDefaultGravityStrength;
	break;

	default:
	case GRAVITY_STANDARD:
		// Easy
		forces.rDirection = mDirection;
		forces.rAcceleration = mMaxAcceleration;
		forces.rVelocity = smDefaultGravityStrength;
	break;
	}
}

void GravitySolver::addForces(Point3F point, GravitySolver::Result &forces)
{
	GravitySolver::Result new_forces;
	getForces(point, new_forces);

	// Should suffice
	forces.rAcceleration = (forces.rAcceleration + new_forces.rAcceleration) / 2.0;
	forces.rDirection = forces.rDirection + new_forces.rDirection;
	forces.rDirection.normalize();
}

// Ideally, we should be able to calculate the angle between up and the gravity normal on each axis
/*Point3F in_gravity = gravity.rDirection;

F32 ang_x = mCos(1.0 / mSqrt((in_gravity.x * in_gravity.x) + (in_gravity.z * in_gravity.z))); //mAcos(1.0 / (gravity.x));
F32 ang_y = mCos(1.0 / mSqrt((in_gravity.y * in_gravity.y) + (in_gravity.z * in_gravity.z))); //mAcos(1.0 / (gravity.y));

orient->set(EulerF(ang_x, ang_y, 0));*/

void GravitySolver::calculateOrient(GravitySolver::Result gravity, QuatF &orient)
{
	// Where we need to point should be opposite to where we are being dragged in
	// the following *works*
	VectorF xAxis;

	EulerF grav = gravity.rDirection;
	//grav.neg();
	if( mFabs(grav.z) > mFabs(grav.x) && mFabs(grav.z) > mFabs(grav.y))
		mCross( VectorF( 0, 1, 0 ), grav, &xAxis );
	else
		mCross( VectorF( 0, 0, 1 ), grav, &xAxis );

	VectorF yAxis;
	mCross( grav, xAxis, &yAxis );

	MatrixF testMat(true);
	testMat.setColumn( 0, xAxis );
	testMat.setColumn( 1, yAxis );
	testMat.setColumn( 2, grav );
  
	orient = QuatF(testMat);
}

void GravitySolver::calculateOrient(GravitySolver::Result gravity, AngAxisF &orient)
{
	F32 gravMag = gravity.rDirection.len();
	Point3F calcDirection = mCross(Point3F(0,0,1), gravity.rDirection);
	F32 gravAngle = mAcos( mDot(Point3F(0,0,1), calcDirection) );

	// Bit of a hack-around, doesn't work
	orient.set(calcDirection, gravAngle);
}

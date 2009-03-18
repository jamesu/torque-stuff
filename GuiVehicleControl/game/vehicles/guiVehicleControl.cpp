#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "gui/core/guiControl.h"
#include "game/gameBase.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"
#include "game/player.h"
#include "sceneGraph/sceneGraph.h"
#include "sim/actionMap.h"

//----------------------------------------------------------------------------

/// GuiVehicleControl
///
/// The following control allows one to move about the control object using a mouse cursor. Certain areas of the screen
/// produce different values for movement, specifically :
///    * Up / Down controls pitch
///    * Left / Right within GuiVehicleControl::mMouseDeadZone controls yaw
///    * Left / Right outside GuiVehicleControl::mMouseDeadZone controls roll
///    * Factor of movement is controlled by mCurveCoeff
///    * Function applied to movement values is controlled by mCurveMode (i.e. square, log), to smooth out movement
///    * Console functions called when left mouse button is pressed are referenced in mEventCtrl
///
/// A good usage of this control would be to supply Move values to pilot the FlyingVehicle class, in a "point to where you want to go" fashion.
/// @note See MoveManager for more information on Move values.
///
class GuiVehicleControl : public GuiControl
{
private:
	typedef GuiControl Parent;
   
	F32 mMouseDeadZone;
	F32 mCurveCoeff;
	U32 mCurveMode;

	GuiCursor *mCurrentCursor;

	GuiCursor *DefaultCursor;
	GuiCursor *ShootCursor;
	GuiCursor *TargetCursor;

	StringTableEntry mEventCtrl[3];
	bool             mUseRightAxis; ///< Use the right stick instead of left?

public:
	GuiVehicleControl();
   
	enum CurveMode
	{
		CURVE_LINEAR=0,
		CURVE_SQUARE=1,
		CURVE_EXPLOG=2
	};

	/// @name Overrides so we can maintain normal control
	/// {
	void onMouseDown(const GuiEvent &evt);
	void onMouseUp(const GuiEvent &evt);
	void onRightMouseDown(const GuiEvent &evt);	void onRightMouseUp(const GuiEvent &evt);
	bool onMouseWheelUp(const GuiEvent &evt);
	bool onMouseWheelDown(const GuiEvent &evt);
	/// }
   
	void getCursor(GuiCursor *&cursor, bool &visible, const GuiEvent &event);

	void onMouseEnter(const GuiEvent &evt);
	void onMouseLeave(const GuiEvent &evt);
	void onMouseMove(const GuiEvent &evt); ///< Main handler
	//void onInputEvent(const InputEvent &event); ///< Override handler
   
   
	/// @name Important stuff
	/// {
	static void initPersistFields();

	DECLARE_CONOBJECT(GuiVehicleControl);
	/// }
};


IMPLEMENT_CONOBJECT(GuiVehicleControl);

//----------------------------------------------------------------------------

GuiVehicleControl::GuiVehicleControl()
{
	mMouseDeadZone = 1.0 / 3.0;
	mCurveMode = CURVE_LINEAR;
	mCurveCoeff = 1.0;

	mCurrentCursor = NULL;

	for (U32 i=0;i<3;i++) {
		mEventCtrl[i] = NULL;
	}
	mUseRightAxis = false;

	DefaultCursor = dynamic_cast<GuiCursor*>(Sim::findObject("DefaultCursor"));
	ShootCursor   = dynamic_cast<GuiCursor*>(Sim::findObject("ShootCursor"));
	TargetCursor  = dynamic_cast<GuiCursor*>(Sim::findObject("TargetCursor"));
}

void GuiVehicleControl::initPersistFields()
{
	Parent::initPersistFields();
	addField( "mouseDeadZone", TypeF32, Offset( mMouseDeadZone, GuiVehicleControl ) );
	addField( "curveMode", TypeS32, Offset( mCurveMode, GuiVehicleControl) );
	addField( "curveCoeff", TypeF32, Offset( mCurveCoeff, GuiVehicleControl) );

	addField( "cmdMouseDown",       TypeString, Offset(mEventCtrl[0], GuiVehicleControl ) );
	addField( "cmdRightMouseDown",  TypeString, Offset(mEventCtrl[1], GuiVehicleControl ) );
	addField( "cmdMiddleMouseDown", TypeString, Offset(mEventCtrl[2], GuiVehicleControl ) );

	addField( "useRightAxis", TypeBool, Offset( mUseRightAxis, GuiVehicleControl) );

	endGroup("Control");
}

void GuiVehicleControl::getCursor(GuiCursor *&cursor, bool &visible, const GuiEvent &event)
{
	event;
	cursor = mCurrentCursor;
	visible = true;
}

void GuiVehicleControl::onMouseEnter(const GuiEvent &evt)
{
	if (ShootCursor) {
		mCurrentCursor = ShootCursor;
	}
}

void GuiVehicleControl::onMouseLeave(const GuiEvent &evt)
{
	if (DefaultCursor) {
		mCurrentCursor = DefaultCursor;
	}
}

void GuiVehicleControl::onMouseMove(const GuiEvent &evt)
{
	// Contribute mouse position to MoveManager::*Speed variables
	// Dead zone (mMouseDeadZone), with the rest contributing
	// mContributePitch, mContributeYaw, and mContriuteRoll.
	// (Mouse position normalized to 0.0->1.0)

	  // Calculate useful vectors (squared)
	Point2I realPos = globalToLocalCoord(evt.mousePoint);
	Point2F normalizedPos((F32)realPos.x / (F32)mBounds.extent.x,
                         (F32)realPos.y / (F32)mBounds.extent.y);
	Point2F centerPos(normalizedPos.x - 0.5, normalizedPos.y - 0.5);
   
	if (mCurveMode == 1) {
		normalizedPos.x = mCurveCoeff * (F32)(normalizedPos.x*normalizedPos.x);
		normalizedPos.y = mCurveCoeff * (F32)(normalizedPos.y*normalizedPos.y);
	} else if (mCurveMode == 2) {
		normalizedPos.x = mCurveCoeff * mExp((F32)mLog((F32)normalizedPos.x));
		normalizedPos.y = mCurveCoeff * mExp((F32)mLog((F32)normalizedPos.y));
	} else {
		normalizedPos *= mCurveCoeff;
	}
   
	//Con::printf("real =[%d,%d], normal=[%f,%f], center=[%f,%f]", realPos.x, realPos.y, normalizedPos.x, normalizedPos.y, centerPos.x, centerPos.y);

	// Send directly to *Speed variables
	// TODO: factor in sensitivity?
	// centerPos is used, expressed in the range -0.5,0.5

	// Pitch goes from up -> down,but needs to be converted to 0-1 range
	MoveManager::mPitchSpeed = centerPos.y * 2;
   	
	// Calculate YAW - must be in dead zone
	F32 absX = mFabs(centerPos.x);
	if (absX < mMouseDeadZone) {
		// Calculate in range (0.0-1.0) in dead zone
		MoveManager::mYawSpeed = absX * (1.0 / mMouseDeadZone);
		if (centerPos.x < 0.0f) MoveManager::mYawSpeed = -MoveManager::mYawSpeed;
	} else {
		MoveManager::mYawSpeed = 0;
	}
   	
	// Calculate ROLL
	if (absX > mMouseDeadZone)
	{
		// Negate dead zone and determine percentage outside dead zone
		// (0.5-mMouseDeadZone)
		MoveManager::mRollSpeed = (absX - mMouseDeadZone) * ( 1.0 / (0.5-mMouseDeadZone) );
		if (centerPos.x < 0.0f) MoveManager::mRollSpeed = -MoveManager::mRollSpeed;
	} else {
		MoveManager::mRollSpeed = 0;
	}

	// Calculate X and Y axis
	if (!mUseRightAxis) {
		MoveManager::mXAxis_L = centerPos.x;
		MoveManager::mYAxis_L = centerPos.y;
	} else {
		MoveManager::mXAxis_R = centerPos.x;
		MoveManager::mYAxis_R = centerPos.y;
	}	
}

// The following overrides are so that mouse events go to the game's ActionMap!
void GuiVehicleControl::onMouseDown(const GuiEvent &evt)
{
	StringTableEntry cmd = mEventCtrl[evt.mouseButton];
	if (cmd != NULL) {
		Con::executef(2, cmd, Con::getFloatArg(1.0));
	}
}

void GuiVehicleControl::onMouseUp(const GuiEvent &evt)
{
	StringTableEntry cmd = mEventCtrl[evt.mouseButton];
	if (cmd != NULL) {
		Con::executef(2, cmd, Con::getFloatArg(0.0));
	}
}

bool GuiVehicleControl::onMouseWheelUp(const GuiEvent &evt)
{
	if ( !mAwake || !mVisible ) { return( false ); }
	//ActionMap::smForceLocalHandle = true;
	return true;
}

bool GuiVehicleControl::onMouseWheelDown(const GuiEvent &evt)
{
	if ( !mAwake || !mVisible ) { return( false ); }
	//ActionMap::smForceLocalHandle = true;
	return true;
}

   
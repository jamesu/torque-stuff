# GuiVehicleControl

A GUI Control that implements a control system for Torque's FlyingVehicle class, 
whereby a mouse cursor is used to point to where the player wants to move the vehicle.

The MoveManager class had to be changed to consolidate the movement speed values, but apart from that, the code can cleanly be applied to a fresh copy of torque.

## Installation

Extract the included files as-is to your torque directory.
Then apply the changes to `game/gameConnectionMoves.cpp` as follows :

1) Change the block that reads :

    F32 MoveManager::mPitchUpSpeed = 0;
    F32 MoveManager::mPitchDownSpeed = 0;
    F32 MoveManager::mYawLeftSpeed = 0;
    F32 MoveManager::mYawRightSpeed = 0;
    F32 MoveManager::mRollLeftSpeed = 0;
    F32 MoveManager::mRollRightSpeed = 0;

To :

    F32 MoveManager::mPitchSpeed = 0;
    //F32 MoveManager::mPitchDownSpeed = 0;
    F32 MoveManager::mYawSpeed = 0;
    //F32 MoveManager::mYawRightSpeed = 0;
    F32 MoveManager::mRollSpeed = 0;
    //F32 MoveManager::mRollRightSpeed = 0;

--

2) Change the block that reads :

    Con::addVariable("mvPitchUpSpeed", TypeF32, &mPitchUpSpeed);
    Con::addVariable("mvPitchDownSpeed", TypeF32, &mPitchDownSpeed);
    Con::addVariable("mvYawLeftSpeed", TypeF32, &mYawLeftSpeed);
    Con::addVariable("mvYawRightSpeed", TypeF32, &mYawRightSpeed);
    Con::addVariable("mvRollLeftSpeed", TypeF32, &mRollLeftSpeed);
    Con::addVariable("mvRollRightSpeed", TypeF32, &mRollRightSpeed);

To : 

    Con::addVariable("mvPitchSpeed", TypeF32, &mPitchSpeed);
    //Con::addVariable("mvPitchDownSpeed", TypeF32, &mPitchDownSpeed);
    Con::addVariable("mvYawSpeed", TypeF32, &mYawSpeed);
    //Con::addVariable("mvYawRightSpeed", TypeF32, &mYawRightSpeed);
    Con::addVariable("mvRollSpeed", TypeF32, &mRollSpeed);
    //Con::addVariable("mvRollRightSpeed", TypeF32, &mRollRightSpeed);

--

3) Comment out the following lines :

    F32 pitchAdd = MoveManager::mPitchUpSpeed - MoveManager::mPitchDownSpeed;
    F32 yawAdd = MoveManager::mYawLeftSpeed - MoveManager::mYawRightSpeed;
    F32 rollAdd = MoveManager::mRollRightSpeed - MoveManager::mRollLeftSpeed;

Then, replace the pitch, yaw, roll block that comes after it with :

    curMove.pitch = MoveManager::mPitch + MoveManager::mPitchSpeed;
    curMove.yaw = MoveManager::mYaw + MoveManager::mYawSpeed;
    curMove.roll = MoveManager::mRoll + MoveManager::mRollSpeed;

Finally, if you are not using TSE (Torque Shader Engine), you may notice that the MoveManager doesn't contain the "MoveManager::mv*Axis_*" values referenced in the control. You can safely comment out any line that refers to these variables.

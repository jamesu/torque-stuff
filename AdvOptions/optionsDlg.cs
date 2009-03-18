// Variables needed by detail options!
$max_screenerror = 25;
$min_TSScreenError = 2;
$max_TSScreenError = 20;
$min_TSDetailAdjust = 0.6;
$max_TSDetailAdjust = 1.0;
//------------------------------------------

function optionsDlg::setPane(%this, %pane)
{
   OptAudioPane.setVisible(false);
   OptGraphicsPane.setVisible(false);
   OptNetworkPane.setVisible(false);
   OptControlsPane.setVisible(false);
   OptDetailPane.setVisible(false);
   ("Opt" @ %pane @ "Pane").setVisible(true);
   OptRemapList.fillList();
}

function saveSettings(%this)
{
   %flushTextures = false;
   
   // Vsync
   if ( $SwapIntervalSupported && OP_VSyncTgl.getValue() != $pref::Video::disableVerticalSync )
   {
      $pref::Video::disableVerticalSync = OP_VSyncTgl.getValue();
      setVerticalSync( !$pref::Video::disableVerticalSync );
   }

   // Detail Stuff
   $pref::Terrain::screenError = $max_screenerror - mFloor( OP_TerrainSlider.getValue() );
   $pref::TS::screenError = $max_TSScreenError - mFloor( OP_ShapeSlider.getValue() * ( $max_TSScreenError - $min_TSScreenError ) );
   $pref::TS::detailAdjust = $min_TSDetailAdjust + OP_ShapeSlider.getValue() * ( $max_TSDetailAdjust - $min_TSDetailAdjust );
   $pref::Shadows = OP_ShadowSlider.getValue();
      setShadowDetailLevel( $pref::Shadows );
   $pref::VisibleDistanceMod = OP_VisibleDistanceSlider.getValue();
   // Sky
     %tmp = OP_SkyDetailMenu.getSelected();
     if ( %tmp == 5 )
   	$pref::SkyOn = false; // No sky for us as #5 is "No Sky"
	else
	{
 	  $pref::SkyOn = true;
	  $pref::numCloudLayers = ( 4 - %tmp ); // 4 max clouds, menu options are 0..4
	}
	
   %temp = OP_PlayerRenderMenu.getSelected();
   $pref::Player::renderMyPlayer = %temp & 1;
   $pref::Player::renderMyItems = %temp & 2;
   
   // Vertex Lighting
   if ( $pref::Interior::VertexLighting != OP_VertexLightTgl.getValue() )
   {
      $pref::Interior::VertexLighting = OP_VertexLightTgl.getValue();
      // Crashes if textured flushed after setting this 
      // %flushTextures = true;
   }
   
   // Texture Detail Sliders need to be saved
   $pref::Terrain::texDetail = 6 - mFloor( OP_TerrainTexSlider.getValue() );

   %temp = 5 - mFloor( OP_ShapeTexSlider.getValue() );
   if ( $pref::OpenGL::mipReduction != %temp )
   {
      $pref::OpenGL::mipReduction = %temp;
      setOpenGLMipReduction( $pref::OpenGL::mipReduction );
      %flushTextures = true;
   }

   %temp = 5 - mFloor( OP_BuildingTexSlider.getValue() );
   if ( $pref::OpenGL::interiorMipReduction != %temp )
   {
      $pref::OpenGL::interiorMipReduction = %temp;
      setOpenGLInteriorMipReduction( $pref::OpenGL::interiorMipReduction );
      %flushTextures = true;
   }

   %temp = 5 - mFloor( OP_SkyTexSlider.getValue() );
   if ( $pref::OpenGL::skyMipReduction != %temp )
   {
      $pref::OpenGL::skyMipReduction = %temp;
      setOpenGLSkyMipReduction( $pref::OpenGL::skyMipReduction );
      %flushTextures = true;
   }
   
   // Anisotropy Rendering
   if ( $AnisotropySupported )
   {
      %temp = OP_AnisotropySlider.getValue();
      if ( $pref::OpenGL::anisotropy != %temp )
      {
         $pref::OpenGL::anisotropy = %temp;
         setOpenGLAnisotropy( $pref::OpenGL::anisotropy );
         %flushTextures = true;
      }
   }

   if (%flushTextures)
    OptionsDlg.schedule( 0, doTextureFlush );
}

function OptionsDlg::doTextureFlush(%this)
{
	Canvas.repaint();
	flushTextureCache();
}

function OptionsDlg::onWake(%this)
{
   %this.setPane(Graphics);
   %buffer = getDisplayDeviceList();
   %count = getFieldCount( %buffer );
   OptGraphicsDriverMenu.clear();
   for(%i = 0; %i < %count; %i++)
      OptGraphicsDriverMenu.add(getField(%buffer, %i), %i);
   %selId = OptGraphicsDriverMenu.findText( $pref::Video::displayDevice );
	if ( %selId == -1 )
		%selId = 0; // How did THAT happen?
	OptGraphicsDriverMenu.setSelected( %selId );
	OptGraphicsDriverMenu.onSelect( %selId, "" );
   OP_GammaSlider.setValue( $pref::OpenGL::gammaCorrection ); // Gamma Correction

   // Audio
   OptAudioUpdate();
   OptAudioVolumeMaster.setValue( $pref::Audio::masterVolume);
   OptAudioVolumeShell.setValue(  $pref::Audio::channelVolume[$GuiAudioType]);
   OptAudioVolumeSim.setValue(    $pref::Audio::channelVolume[$SimAudioType]);
   OptAudioDriverList.clear();
   OptAudioDriverList.add("OpenAL", 1);
   OptAudioDriverList.add("none", 2);
   %selId = OptAudioDriverList.findText($pref::Audio::driver);
	if ( %selId == -1 )
		%selId = 0; // How did THAT happen?
	OptAudioDriverList.setSelected( %selId );
	OptAudioDriverList.onSelect( %selId, "" );
	// Detail Controls
	OP_TerrainSlider.setValue( $max_screenerror - $pref::Terrain::screenError );
	OP_ShapeSlider.setValue( ( $max_TSScreenError - $pref::TS::screenError ) / ( $max_TSScreenError - $min_TSScreenError ) );
   	OP_ShadowSlider.setValue( $pref::Shadows );
	// Sky
	   OP_SkyDetailMenu.init();
 	  if ( !$pref::SkyOn )
 	     %selId = 5;
 	  else if ( $pref::numCloudLayers >= 0 && $pref::numCloudLayers < 4 )
 	     %selId = 4 - $pref::numCloudLayers;
  	 else
  	    %selId = 1;
  	 OP_SkyDetailMenu.setSelected( %selId );
  	 OP_SkyDetailMenu.setText(OP_SkyDetailMenu.getTextById( %selId ));
   OP_VertexLightTgl.setValue( $pref::Interior::VertexLighting );
   OP_PlayerRenderMenu.init();
   %selId = $pref::Player::renderMyPlayer | ( $pref::Player::renderMyItems << 1 );
   OP_PlayerRenderMenu.setSelected( %selId );
   OP_VisibleDistanceSlider.setValue( $pref::VisibleDistanceMod );

   // Init Texture Menu
   OP_DetailSelect.init();
   %selId = 1;
   OP_DetailSelect.setSelected( %selId );
   OP_DetailSelect.setText(OP_DetailSelect.getTextById( %selId ));
   // And also init the texture sliders
   OP_ShapeTexSlider.setValue( 5 - $pref::OpenGL::mipReduction );
   OP_TerrainTexSlider.setValue( 6 - $pref::Terrain::texDetail );
   OP_BuildingTexSlider.setValue( 5 - $pref::OpenGL::interiorMipReduction );
   OP_SkyTexSlider.setValue( 5 - $pref::OpenGL::skyMipReduction );
   
   // Anisotropy Rendering
   OP_AnisotropySlider.setValue( $pref::OpenGL::anisotropy );
   OP_AnisotropySlider.setActive( $AnisotropySupported );
   
   // Vsync
   if ( $SwapIntervalSupported )
   {
      OP_VSyncTgl.setValue( $pref::Video::disableVerticalSync );
      OP_VSyncTgl.setActive( true );
   }
   else
   {
      OP_VSyncTgl.setValue( false );
      OP_VSyncTgl.setActive( false );
   }
}

function OptionsDlg::onSleep(%this)
{
   // write out the control config into the fps/config.cs file
   moveMap.save( "~/client/config.cs" );

}

function OP_DetailSelect::init(%this)
{
	%this.clear();
	%this.add("Terrain", 1);
	%this.add("Shape", 2);
	%this.add("Building", 3);
	%this.add("Sky", 4);
}

function OP_DetailSelect::onSelect( %this, %id, %text )
{
	// Hide All Sliders
	OP_TerrainTexSlider.visible = false;
	OP_ShapeTexSlider.visible = false;
	OP_BuildingTexSlider.visible = false;
	OP_SkyTexSlider.visible = false;
	// Now Show the correct slider
	switch ( %id )
	{
	case 1:OP_TerrainTexSlider.visible = true;
	case 2:OP_ShapeTexSlider.visible = true;
	case 3:OP_BuildingTexSlider.visible = true;
	case 4:OP_SkyTexSlider.visible = true;
	}
}

function OP_SkyDetailMenu::init(%this)
{
	%this.clear();
	%this.add("Full Sky", 1);
	%this.add("Two Cloud Layers", 2);
	%this.add("One Cloud Layer", 3);
	%this.add("Sky Box Only", 4);
	%this.add("No Sky", 5);
}

function OptGraphicsDriverMenu::onSelect( %this, %id, %text )
{
	// Attempt to keep the same res and bpp settings:
	if ( OptGraphicsResolutionMenu.size() > 0 )
		%prevRes = OptGraphicsResolutionMenu.getText();
	else
		%prevRes = getWords( $pref::Video::resolution, 0, 1 );

	// Check if this device is full-screen only:
	if ( isDeviceFullScreenOnly( %this.getText() ) )
	{
		OptGraphicsFullscreenToggle.setValue( true );
		OptGraphicsFullscreenToggle.setActive( false );
		OptGraphicsFullscreenToggle.onAction();
	}
	else
		OptGraphicsFullscreenToggle.setActive( true );

	if ( OptGraphicsFullscreenToggle.getValue() )
	{
		if ( OptGraphicsBPPMenu.size() > 0 )
			%prevBPP = OptGraphicsBPPMenu.getText();
		else
			%prevBPP = getWord( $pref::Video::resolution, 2 );
	}

	// Fill the resolution and bit depth lists:
	OptGraphicsResolutionMenu.init( %this.getText(), OptGraphicsFullscreenToggle.getValue() );
	OptGraphicsBPPMenu.init( %this.getText() );

	// Try to select the previous settings:
	%selId = OptGraphicsResolutionMenu.findText( %prevRes );
	if ( %selId == -1 )
		%selId = 0;
	OptGraphicsResolutionMenu.setSelected( %selId );

	if ( OptGraphicsFullscreenToggle.getValue() )
	{
		%selId = OptGraphicsBPPMenu.findText( %prevBPP );
		if ( %selId == -1 )
			%selId = 0;
		OptGraphicsBPPMenu.setSelected( %selId );
		OptGraphicsBPPMenu.setText( OptGraphicsBPPMenu.getTextById( %selId ) );
	}
	else
		OptGraphicsBPPMenu.setText( "Default" );

}

function OptGraphicsResolutionMenu::init( %this, %device, %fullScreen )
{
	%this.clear();
	%resList = getResolutionList( %device );
	%resCount = getFieldCount( %resList );
	%deskRes = getDesktopResolution();

   %count = 0;
	for ( %i = 0; %i < %resCount; %i++ )
	{
		%res = getWords( getField( %resList, %i ), 0, 1 );

		if ( !%fullScreen )
		{
			if ( firstWord( %res ) >= firstWord( %deskRes ) )
				continue;
			if ( getWord( %res, 1 ) >= getWord( %deskRes, 1 ) )
				continue;
		}

		// Only add to list if it isn't there already:
		if ( %this.findText( %res ) == -1 )
      {
			%this.add( %res, %count );
         %count++;
      }
	}
}

function OptGraphicsFullscreenToggle::onAction(%this)
{
   Parent::onAction();
   %prevRes = OptGraphicsResolutionMenu.getText();

   // Update the resolution menu with the new options
   OptGraphicsResolutionMenu.init( OptGraphicsDriverMenu.getText(), %this.getValue() );

   // Set it back to the previous resolution if the new mode supports it.
   %selId = OptGraphicsResolutionMenu.findText( %prevRes );
   if ( %selId == -1 )
   	%selId = 0;
 	OptGraphicsResolutionMenu.setSelected( %selId );
}


function OptGraphicsBPPMenu::init( %this, %device )
{
	%this.clear();

	if ( %device $= "Voodoo2" )
		%this.add( "16", 0 );
	else
	{
		%resList = getResolutionList( %device );
		%resCount = getFieldCount( %resList );
      %count = 0;
		for ( %i = 0; %i < %resCount; %i++ )
		{
			%bpp = getWord( getField( %resList, %i ), 2 );

			// Only add to list if it isn't there already:
			if ( %this.findText( %bpp ) == -1 )
         {
				%this.add( %bpp, %count );
            %count++;
         }
		}
	}
}

function optionsDlg::applyGraphics( %this )
{
	%newDriver = OptGraphicsDriverMenu.getText();
	%newRes = OptGraphicsResolutionMenu.getText();
	%newBpp = OptGraphicsBPPMenu.getText();
	%newFullScreen = OptGraphicsFullscreenToggle.getValue();

	if ( %newDriver !$= $pref::Video::displayDevice )
	{
		setDisplayDevice( %newDriver, firstWord( %newRes ), getWord( %newRes, 1 ), %newBpp, %newFullScreen );
		//OptionsDlg::deviceDependent( %this );
	}
	else
		setScreenMode( firstWord( %newRes ), getWord( %newRes, 1 ), %newBpp, %newFullScreen );
}



$RemapCount = 0;
$RemapName[$RemapCount] = "Forward";
$RemapCmd[$RemapCount] = "moveforward";
$RemapCount++;
$RemapName[$RemapCount] = "Backward";
$RemapCmd[$RemapCount] = "movebackward";
$RemapCount++;
$RemapName[$RemapCount] = "Strafe Left";
$RemapCmd[$RemapCount] = "moveleft";
$RemapCount++;
$RemapName[$RemapCount] = "Strafe Right";
$RemapCmd[$RemapCount] = "moveright";
$RemapCount++;
$RemapName[$RemapCount] = "Turn Left";
$RemapCmd[$RemapCount] = "turnLeft";
$RemapCount++;
$RemapName[$RemapCount] = "Turn Right";
$RemapCmd[$RemapCount] = "turnRight";
$RemapCount++;
$RemapName[$RemapCount] = "Look Up";
$RemapCmd[$RemapCount] = "panUp";
$RemapCount++;
$RemapName[$RemapCount] = "Look Down";
$RemapCmd[$RemapCount] = "panDown";
$RemapCount++;
$RemapName[$RemapCount] = "Jump";
$RemapCmd[$RemapCount] = "jump";
$RemapCount++;
$RemapName[$RemapCount] = "Fire Weapon";
$RemapCmd[$RemapCount] = "mouseFire";
$RemapCount++;
$RemapName[$RemapCount] = "Adjust Zoom";
$RemapCmd[$RemapCount] = "setZoomFov";
$RemapCount++;
$RemapName[$RemapCount] = "Toggle Zoom";
$RemapCmd[$RemapCount] = "toggleZoom";
$RemapCount++;
$RemapName[$RemapCount] = "Free Look";
$RemapCmd[$RemapCount] = "toggleFreeLook";
$RemapCount++;
$RemapName[$RemapCount] = "Switch 1st/3rd";
$RemapCmd[$RemapCount] = "toggleFirstPerson";
$RemapCount++;
$RemapName[$RemapCount] = "Toggle Message Hud";
$RemapCmd[$RemapCount] = "toggleMessageHud";
$RemapCount++;
$RemapName[$RemapCount] = "Message Hud PageUp";
$RemapCmd[$RemapCount] = "pageMessageHudUp";
$RemapCount++;
$RemapName[$RemapCount] = "Message Hud PageDown";
$RemapCmd[$RemapCount] = "pageMessageHudDown";
$RemapCount++;
$RemapName[$RemapCount] = "Resize Message Hud";
$RemapCmd[$RemapCount] = "resizeMessageHud";
$RemapCount++;
$RemapName[$RemapCount] = "Toggle Camera";
$RemapCmd[$RemapCount] = "toggleCamera";
$RemapCount++;
$RemapName[$RemapCount] = "Drop Camera at Player";
$RemapCmd[$RemapCount] = "dropCameraAtPlayer";
$RemapCount++;
$RemapName[$RemapCount] = "Drop Player at Camera";
$RemapCmd[$RemapCount] = "dropPlayerAtCamera";
$RemapCount++;


function restoreDefaultMappings()
{
   moveMap.delete();
   exec( "~/client/scripts/default.bind.cs" );
   OptRemapList.fillList();
}

function getMapDisplayName( %device, %action )
{
	if ( %device $= "keyboard" )
		return( %action );		
	else if ( strstr( %device, "mouse" ) != -1 )
	{
		// Substitute "mouse" for "button" in the action string:
		%pos = strstr( %action, "button" );
		if ( %pos != -1 )
		{
			%mods = getSubStr( %action, 0, %pos );
			%object = getSubStr( %action, %pos, 1000 );
			%instance = getSubStr( %object, strlen( "button" ), 1000 );
			return( %mods @ "mouse" @ ( %instance + 1 ) );
		}
		else
			error( "Mouse input object other than button passed to getDisplayMapName!" );
	}
	else if ( strstr( %device, "joystick" ) != -1 )
	{
		// Substitute "joystick" for "button" in the action string:
		%pos = strstr( %action, "button" );
		if ( %pos != -1 )
		{
			%mods = getSubStr( %action, 0, %pos );
			%object = getSubStr( %action, %pos, 1000 );
			%instance = getSubStr( %object, strlen( "button" ), 1000 );
			return( %mods @ "joystick" @ ( %instance + 1 ) );
		}
		else
	   { 
	      %pos = strstr( %action, "pov" );
         if ( %pos != -1 )
         {
            %wordCount = getWordCount( %action );
            %mods = %wordCount > 1 ? getWords( %action, 0, %wordCount - 2 ) @ " " : "";
            %object = getWord( %action, %wordCount - 1 );
            switch$ ( %object )
            {
               case "upov":   %object = "POV1 up";
               case "dpov":   %object = "POV1 down";
               case "lpov":   %object = "POV1 left";
               case "rpov":   %object = "POV1 right";
               case "upov2":  %object = "POV2 up";
               case "dpov2":  %object = "POV2 down";
               case "lpov2":  %object = "POV2 left";
               case "rpov2":  %object = "POV2 right";
               default:       %object = "??";
            }
            return( %mods @ %object );
         }
         else
            error( "Unsupported Joystick input object passed to getDisplayMapName!" );
      }
	}
		
	return( "??" );		
}

function buildFullMapString( %index )
{
   %name       = $RemapName[%index];
   %cmd        = $RemapCmd[%index];

	%temp = moveMap.getBinding( %cmd );
   %device = getField( %temp, 0 );
   %object = getField( %temp, 1 );
   if ( %device !$= "" && %object !$= "" )
	   %mapString = getMapDisplayName( %device, %object );
   else
      %mapString = "";

	return( %name TAB %mapString );
}

function OptRemapList::fillList( %this )
{
	%this.clear();
   for ( %i = 0; %i < $RemapCount; %i++ )
      %this.addRow( %i, buildFullMapString( %i ) );
}

//------------------------------------------------------------------------------
function OptRemapList::doRemap( %this )
{
	%selId = %this.getSelectedId();
   %name = $RemapName[%selId];

	OptRemapText.setValue( "REMAP \"" @ %name @ "\"" );
	OptRemapInputCtrl.index = %selId;
	Canvas.pushDialog( RemapDlg );
}

//------------------------------------------------------------------------------
function redoMapping( %device, %action, %cmd, %oldIndex, %newIndex )
{
	//%actionMap.bind( %device, %action, $RemapCmd[%newIndex] );
	moveMap.bind( %device, %action, %cmd );
	OptRemapList.setRowById( %oldIndex, buildFullMapString( %oldIndex ) );
	OptRemapList.setRowById( %newIndex, buildFullMapString( %newIndex ) );
}

//------------------------------------------------------------------------------
function findRemapCmdIndex( %command )
{
	for ( %i = 0; %i < $RemapCount; %i++ )
	{
		if ( %command $= $RemapCmd[%i] )
			return( %i );			
	}
	return( -1 );	
}

function OptRemapInputCtrl::onInputEvent( %this, %device, %action )
{
	//error( "** onInputEvent called - device = " @ %device @ ", action = " @ %action @ " **" );
	Canvas.popDialog( RemapDlg );

	// Test for the reserved keystrokes:
	if ( %device $= "keyboard" )
	{
      // Cancel...
      if ( %action $= "escape" )
      {
         // Do nothing...
		   return;
      }
	}
	
   %cmd  = $RemapCmd[%this.index];
   %name = $RemapName[%this.index];

	// First check to see if the given action is already mapped:
	%prevMap = moveMap.getCommand( %device, %action );
   if ( %prevMap !$= %cmd )
   {
	   if ( %prevMap $= "" )
	   {
         moveMap.bind( %device, %action, %cmd );
		   OptRemapList.setRowById( %this.index, buildFullMapString( %this.index ) );
	   }
	   else
	   {
         %mapName = getMapDisplayName( %device, %action );
		   %prevMapIndex = findRemapCmdIndex( %prevMap );
		   if ( %prevMapIndex == -1 )
			   MessageBoxOK( "REMAP FAILED", "\"" @ %mapName @ "\" is already bound to a non-remappable command!" );
		   else
         {
            %prevCmdName = $RemapName[%prevMapIndex];
			   MessageBoxYesNo( "WARNING", 
				   "\"" @ %mapName @ "\" is already bound to \"" 
					   @ %prevCmdName @ "\"!\nDo you want to undo this mapping?", 
				   "redoMapping(" @ %device @ ", \"" @ %action @ "\", \"" @ %cmd @ "\", " @ %prevMapIndex @ ", " @ %this.index @ ");", "" );
         }
		   return;
	   }
   }
}

// Audio 
function OptAudioUpdate()
{
   // set the driver text
   %text =   "Vendor: " @ alGetString("AL_VENDOR") @
           "\nVersion: " @ alGetString("AL_VERSION") @
           "\nRenderer: " @ alGetString("AL_RENDERER") @
           "\nExtensions: " @ alGetString("AL_EXTENSIONS");
   OptAudioInfo.setText(%text);

}


// Channel 0 is unused in-game, but is used here to test master volume.

new AudioDescription(AudioChannel0)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 0;
};

new AudioDescription(AudioChannel1)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 1;
};

new AudioDescription(AudioChannel2)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 2;
};

new AudioDescription(AudioChannel3)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 3;
};

new AudioDescription(AudioChannel4)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 4;
};

new AudioDescription(AudioChannel5)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 5;
};

new AudioDescription(AudioChannel6)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 6;
};

new AudioDescription(AudioChannel7)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 7;
};

new AudioDescription(AudioChannel8)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 8;
};

$AudioTestHandle = 0;

function OptAudioUpdateMasterVolume(%volume)
{
   if (%volume == $pref::Audio::masterVolume)
      return;
   alxListenerf(AL_GAIN_LINEAR, %volume);
   $pref::Audio::masterVolume = %volume;
   if (!alxIsPlaying($AudioTestHandle))
   {
      $AudioTestHandle = alxCreateSource("AudioChannel0", expandFilename("~/data/sound/testing.wav"));
      alxPlay($AudioTestHandle);
   }
}

function OptAudioUpdateChannelVolume(%channel, %volume)
{
   if (%channel < 1 || %channel > 8)
      return;

   if (%volume == $pref::Audio::channelVolume[%channel])
      return;

   alxSetChannelVolume(%channel, %volume);
   $pref::Audio::channelVolume[%channel] = %volume;
   if (!alxIsPlaying($AudioTestHandle))
   {
      $AudioTestHandle = alxCreateSource("AudioChannel"@%channel, expandFilename("~/data/sound/testing.wav"));
      alxPlay($AudioTestHandle);
   }
}


function OptAudioDriverList::onSelect( %this, %id, %text )
{
   if (%text $= "")
      return;

   if ($pref::Audio::driver $= %text)
      return;

   $pref::Audio::driver = %text;
   OpenALInit();
}

//------------------------------------------

// Graphic Stuff

function updateGammaCorrection()
{
   $pref::OpenGL::gammaCorrection = OP_GammaSlider.getValue();
   videoSetGammaCorrection( $pref::OpenGL::gammaCorrection ); 
}

// Terrain Stuff
function updateTerrainDetail()
{
   $pref::Terrain::screenError = $max_screenerror - mFloor( OP_TerrainSlider.getValue());
   if ( OP_TerrainSlider.getValue() != $max_screenerror - $pref::Terrain::screenError )
      OP_TerrainSlider.setValue( $max_screenerror - $pref::Terrain::screenError );
}

// Player Render Options
function OP_PlayerRenderMenu::init( %this )
{
   %this.clear();
   %this.add( "Player and Items", 3 );
   %this.add( "Player only", 1 );
   %this.add( "Items only", 2 );
   %this.add( "Neither Player nor Items", 0 );
}

//------------------------------------------


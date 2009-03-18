# .JKL Loader Resource

This resource implements a class that displays levels from Dark Forces II : Jedi Knight. 
In addition, player collision is supported. However, no culling of the level is performed, 
so large levels will likely lag on low-end systems.

The JKLLevel SceneObject class is based off of Matthew Fairfax's SimplestObject class.

NOTE: textures are hard coded to load from starter.fps/level. So you will need to copy any used textures
      to this directory. Also, .mat's are *not* supported by torque, so you will need to convert the
      original textures to .jpg's or .png's. If a texture cannot be found, common/level/dflt is loaded instead.


## Installation

To install, extract the files in the "game" folder into your "game" directory, and the files in the "core" folder into your core folder.
Add jklFile.cc, jklLevel.cc, and tokenizer.cc to your regular torque project.

Next, make sure .jkl files are registered with the resource manager. In game/main.cc :

Add the extern :

    extern ResourceInstance *constructJKL(Stream &);

And add this call to the list of similar function calls in initLibraries() :
ResourceManager->registerExtension(".jkl", constructJKL);

Finally, you need to add the JKL Convex type to collision/convex.h :

    enum ConvexType {
       TSConvexType,
       BoxConvexType,
       TerrainConvexType,
       InteriorConvexType,
       ShapeBaseConvexType,
       TSStaticConvexType,
    +  JKLConvexType   
    }; 

In order to place JKL levels into the world editor, add the following function in /example/common/editor/ObjectBuilderGui.gui :

    function ObjectBuilderGui::buildJKLLevel(%this)
    {
       %this.className = "JKLLevel";
       %this.process();
    }

Add this to /example/common/editor/EditorGui.cs in the Creator::init( %this ) function :

    // walk all the statics and add them to the correct group
    %staticId = "";
    %file = findFirstFile( "*.jkl" );
    while( %file !$= "" ) 
    {
       // Determine which group to put the file in
       // and build the group heirarchy as we go
       %split    = strreplace(%file, "/", " ");
       %dirCount = getWordCount(%split)-1;
       %parentId = %base;
       
       for(%i=0; %i<%dirCount; %i++)
       {
          %parent = getWords(%split, 0, %i);
          // if the group doesn't exist create it
          if ( !%staticId[%parent] )
             %staticId[%parent] = %this.addGroup( %parentId, getWord(%split, %i));
          %parentId = %staticId[%parent];
       }
       // Add the file to the group
       %create = "createJKLLevel(\"" @ %file @ "\");";
       %this.addItem( %parentId, fileBase( %file ), %create );
       
       %file = findNextFile( "*.jkl" );
    }
   
And add this function to the same file :

    function createJKLLevel(%name)
    {
       %obj = new JKLLevel()
       {
          position = "0 0 0";
          rotation = "0 0 0";
          levelFile = %name;
       };
       
       return(%obj);
    }

There, all done!

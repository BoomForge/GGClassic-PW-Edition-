//----------------------------------------------------
//--- GAMEGURU - M-Grass
//----------------------------------------------------
#include "stdafx.h"
#include "gameguru.h"
#include "PW-EnvironmentStyleSettings.h"
#include "PW-GrassMix.h"
#include "PW-StyleImport.h"
#include <array>
#include <cmath>
#include <limits>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


namespace PWGrassMix { bool PrepareNative(); }

// Global
int g_iSuperTriggerFullGrassReveal = 0;

// Externs

#ifdef ENABLEIMGUI
extern bool bUpdateVeg;
#endif


// 
//  GRASS SYSTEM
// 

void grass_init ( void )
{
    PWGrassMix::ResetInstances(); // release previous map's owned instances
    // A saved mixed-grass map must retain its actual first selected species
    // as the native Classic base even after the level header restores the
    // previously selected single-style vegbank setting.
    if (PWGrassMix::RenderEnabled())
    {
        for (int slotIndex=0; slotIndex<PWGrassMix::MaxStyles; ++slotIndex)
        {
            const PWGrassMix::Slot& slot=PWGrassMix::slots[slotIndex];
            if (!slot.enabled || slot.weight<=0.f || slot.style.empty()) continue;
            for (int bank=1; bank<=g.vegstylemax; ++bank)
            {
                if (!PWStyleImport::Same(slot.style,t.vegstylebank_s[bank].Get()))
                    continue;
                g.vegstyleindex=bank;
                g.vegstyle_s=t.vegstylebank_s[bank];
                t.visuals.vegetationindex=bank;
                t.visuals.vegetation_s=t.vegstylebank_s[bank];
                break;
            }
            break;
        }
    }
	//  init our resource values
	t.tGrassObj=t.terrain.objectstartindex+6;
	t.tGrassImg=t.terrain.imagestartindex+8;
	t.tObjectGridStart = t.terrain.objectstartindex + 6201;
	t.terrain.vegetationshaderindex=t.terrain.effectstartindex+2;
	t.tVegShadowTex = t.terrain.imagestartindex+2;
	t.tTempMesh = t.terrain.objectstartindex+7;

	// load shaders for non-PBR and PBR vegetation
	if ( GetEffectExist ( t.terrain.effectstartindex+2 ) == 0 )
	{
		//LoadEffect (  "effectbank\\reloaded\\apbr_veg.fx", t.terrain.effectstartindex+6, 0 );
		if ( g.gpbroverride == 1 )
			LoadEffect (  "effectbank\\reloaded\\apbr_veg.fx", t.terrain.effectstartindex+2, 0 );
		else
			LoadEffect (  "effectbank\\reloaded\\vegetation_basic.fx", t.terrain.effectstartindex+2, 0 );
	}

	// PBR Support textures
	int iPBRAGEDImg = t.terrain.imagestartindex+14;
	int iPBRSpecImg = t.terrain.imagestartindex+15;
	int iPBRCubeImg = t.terrain.imagestartindex+31;
	int iPBRCurveImg = t.terrain.imagestartindex+32;
	if ( ImageExist ( t.terrain.imagestartindex+14 ) == 0 ) LoadImage ( "effectbank\\reloaded\\media\\AGED.png",t.terrain.imagestartindex+14 );
	if ( ImageExist ( t.terrain.imagestartindex+15 ) == 0 ) LoadImage ( "effectbank\\reloaded\\media\\blank_black.dds",t.terrain.imagestartindex+15 );
	SetPBRResourceValues ( iPBRAGEDImg, iPBRSpecImg, iPBRCubeImg, iPBRCurveImg );

	//  Pass the veg system the following initialisation values first. These are just resource numbers and numerical settings;
	//  1. Grass object/mesh ID for base grass model
	//  2. Start object for grid of grass models (ensure grid size doesnt exist available object numbers when x2)
	//  3. Grass texture
	//  4. Shadow texture
	//  5. A temporary mesh used for building grass objects
	//  6. Shader ID
	//  7. Grass existance memblock
	//  8. Memblock width/height
	//  9. Camera mask for veg objects
	//C++ ISSUE %001 replaced with 1 - should be fine since %001 in binary is 1 in dec
	SetResourceValues (  t.tGrassObj,t.tObjectGridStart,t.tGrassImg,t.tVegShadowTex,t.tTempMesh,t.terrain.vegetationshaderindex,t.terrain.grassmemblock,MAXTEXTURESIZE, 1 );

	//  now load our grass mesh
	if (  ObjectExist(t.tGrassObj)  ==  1  )  DeleteObject (  t.tGrassObj );
	if (  GetMeshExist(t.tGrassObj)  ==  1  )  DeleteMesh (  t.tGrassObj );

	cstr pFileToLoad = cstr(cstr("vegbank\\")+g.vegstyle_s+"\\veg.DBO");
	if ( FileExist ( pFileToLoad.Get() ) == 0 ) pFileToLoad = cstr(cstr("vegbank\\")+g.vegstyle_s+"\\veg.X");
	LoadObject ( pFileToLoad.Get(),t.tGrassObj );
    if (ObjectExist(t.tGrassObj) == 1)
    {
        // Custom mesh import saves size/orientation in its style folder.
        // The legacy default is precisely 200% with no extra yaw.
        const std::string style = std::string("vegbank\\") + g.vegstyle_s.Get();
        const PWEnvironmentStyle::Settings pwSettings = PWEnvironmentStyle::Read(style);
        const float scale = 200.f * pwSettings.percent / 100.f;
        ScaleObject(t.tGrassObj, scale, scale, scale);
        if (pwSettings.yaw != 0.f) YRotateObject(t.tGrassObj, pwSettings.yaw);
    }
	MakeMeshFromObject ( t.tGrassObj,t.tGrassObj );


	// load our grass piece image
	grass_setgrassimage();

	// apply correct shader
	grass_applyshader ();

	//  if the user has updated the grass bitmap in the editor and the init function has been called, we are testing a level
	//  and a new grass memblock file needs to be made from the bitmap. Otherwise there should be one to load
	t.tfileveggrass_s=g.mysystem.levelBankTestMap_s+"vegmaskgrass.dat"; //"levelbank\\testmap\\vegmaskgrass.dat";
	if (  t.terrain.grassregionx1 != t.terrain.grassregionx2 || t.terrain.grassregionupdate == 2 || FileExist(t.tfileveggrass_s.Get()) == 0 ) 
	{
		grass_updategrassfrombitmap ( );
	}
	else
	{
		grass_loadgrass ( );
	}

	// Load all selected style meshes/atlases before creating native patches.
    PWGrassMix::PrepareNative();

	//  calculate how big our grid of vegetation is
	grass_setgrassgridandfade ( );

	//  now create and setup all of our vegetation objects. This will delete any veg that already exists. Passing;
	//  1. Number of grass items per veg object
	//  2. Width of grass
	//  3. Height of grass
	//  4. Size of each veg area
	//  5. Dimension of the veg grid
	if (  t.terrain.superflat == 1 ) 
	{
		t.tTerrainID = 0;
	}
	else
	{
		t.tTerrainID = t.terrain.TerrainID;
	}
	int iTrimUsingGrassMemblock = 0;
	if ( t.game.gameisexe == 1 ) iTrimUsingGrassMemblock = t.terrain.grassmemblock;
	if( g.usegrassbelowwater > 0)
		MakeVegetationGrid(4.0f*t.visuals.VegQuantity_f, t.visuals.VegWidth_f, t.visuals.VegHeight_f, terrain_veg_areawidth, t.terrain.vegetationgridsize, t.tTerrainID, iTrimUsingGrassMemblock , true );
	else
		MakeVegetationGrid( 4.0f*t.visuals.VegQuantity_f,t.visuals.VegWidth_f,t.visuals.VegHeight_f,terrain_veg_areawidth,t.terrain.vegetationgridsize,t.tTerrainID, iTrimUsingGrassMemblock , false );

    UpdateGrassTexture(g.gpbroverride);

	//  small lookup for memblock painting circles
	Dim ( t.curve_f,100 );
	for ( t.r = 0 ; t.r <= 180; t.r++ )
	{
		t.trx_f = Cos(t.r-90)*100.0;
		t.trz_f = Sin(t.r-90)*100.0;
		t.curve_f [ int((100+t.trz_f)/2) ] = t.trx_f/100.0;
	}

	RefreshGridExistArray();
	UpdateBlitzTerrain(CameraPositionX(0), CameraPositionZ(0), t.terrain.TerrainID, g.postprocessimageoffset + 5);
    // Do not briefly draw the original one-style vegetation grid on top of a
    // selected PW mix while the replacement instances are being prepared.
    // grass_loop() will explicitly restore Classic on invalid/missing assets.
    // Keep Classic visible until PW has *actually* placed mixed blades.
    // Resource readiness and an empty painted mask must not blank the editor.
    SetVegetationGridVisible(true);

}

void grass_initstyles ( void )
{
	// Collect vegetation styles
	g.vegstylemax=0;
	SetDir ( "vegbank" );
	ChecklistForFiles();
	for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
	{
		t.tfile_s=ChecklistString(t.c);
		if (  ChecklistValueA(t.c) == 1 ) 
		{
			if (  t.tfile_s.Get()[0] != '.' ) 
			{
				++g.vegstylemax;
				Dim (  t.vegstylebank_s,g.vegstylemax );
				t.vegstylebank_s[g.vegstylemax]=Lower(t.tfile_s.Get());
			}
		}
	}
	SetDir (  ".." );

	// and init grass choice
	grass_initstyles_reset();
}

void grass_initstyles_reset ( void )
{
	#ifdef VRTECH
	g.vegstyle_s = "weedy 01"; //PE: hanged from: "lushy";
	#else
	g.vegstyle_s="lushy";
	#endif
	if ( PathExist( cstr(cstr("vegbank\\")+g.vegstyle_s).Get() ) == 0 ) 
	{
		g.vegstyle_s=t.vegstylebank_s[1];
	}
	//  find vegstyle index
	for ( g.vegstyleindex = 1 ; g.vegstyleindex<=  g.vegstylemax; g.vegstyleindex++ )
	{
		if ( cstr(Lower(g.vegstyle_s.Get())) == t.vegstylebank_s[g.vegstyleindex] ) 
		{
			break;
		}
	}
	if ( g.vegstyleindex > g.vegstylemax )
	{
		g.vegstyleindex = g.vegstylemax;
		g.vegstyle_s = t.vegstylebank_s[g.vegstyleindex];
	}
}

void grass_assignnewshader ( void )
{
	// Choose the vegetation shader to use
	//if ( g.gpbroverride == 1 ) // t.terrain.iTerrainPBRMode == 1 )
	//	t.terrain.vegetationshaderindex = t.terrain.effectstartindex+6;
	//else
	//	t.terrain.vegetationshaderindex = t.terrain.effectstartindex+2;
	t.terrain.vegetationshaderindex = t.terrain.effectstartindex+2;

	// Veg shader constants
	memset ( &t.effectparam.vegetation, 0, sizeof(t.effectparam.vegetation) );
	if ( GetEffectExist ( t.terrain.vegetationshaderindex ) == 1 )
	{
		//t.effectparam.vegetation.g_lights_data=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_data");
		//t.effectparam.vegetation.g_lights_pos0=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_pos0");
		//t.effectparam.vegetation.g_lights_atten0=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_atten0");
		//t.effectparam.vegetation.g_lights_diffuse0=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_diffuse0");
		//t.effectparam.vegetation.g_lights_pos1=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_pos1");
		//t.effectparam.vegetation.g_lights_atten1=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_atten1");
		//t.effectparam.vegetation.g_lights_diffuse1=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_diffuse1");
		//t.effectparam.vegetation.g_lights_pos2=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_pos2");
		//t.effectparam.vegetation.g_lights_atten2=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_atten2");
		//t.effectparam.vegetation.g_lights_diffuse2=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_diffuse2");
		//t.effectparam.vegetation.SpotFlashPos=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"SpotFlashPos");
		//t.effectparam.vegetation.SpotFlashColor=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"SpotFlashColor");

		// wipe any previous param storage
		ResetEffect ( t.terrain.vegetationshaderindex );
	}
}

void grass_applyshader ( void )
{
	// Choose the vegetation shader to use
	grass_assignnewshader();

	// Apply veg shader to all veg objects
	UpdateGrassTexture ( g.gpbroverride );//t.terrain.iTerrainPBRMode );
	UpdateGrassShader ( t.terrain.vegetationshaderindex );
}

void grass_setgrassimage ( void )
{
	SetImageAutoMipMap (  1 );
	t.tGrassImg=t.terrain.imagestartindex+8;
	if ( ImageExist(t.tGrassImg)  ==  1  )  DeleteImage (  t.tGrassImg );
	if ( g.gdividetexturesize == 0 ) 
	{
		image_setlegacyimageloading(true);
		LoadImage ( "effectbank\\reloaded\\media\\white_D.dds",t.tGrassImg );
		image_setlegacyimageloading(false);
	}
	else
	{
        // PW Edition: user grass styles may provide an ordinary PNG atlas
        // instead of DDS. Preserve DDS-first Classic compatibility.
        cstr grassBase = cstr(cstr("vegbank\\") + g.vegstyle_s + "\\grass");
        cstr grassFile = grassBase + ".dds";
        if (FileExist(grassFile.Get()) == 0) grassFile = grassBase + ".png";
        if (FileExist(grassFile.Get()) == 0) grassFile = grassBase + ".jpg";
        if (FileExist(grassFile.Get()) == 0) grassFile = grassBase + ".jpeg";
        if (FileExist(grassFile.Get()) == 0) grassFile = grassBase + ".bmp";
        if (FileExist(grassFile.Get()) == 0) grassFile = grassBase + ".tga";
        LoadImage(grassFile.Get(), t.tGrassImg);
	}
	extern bool bResourcesSet, bGridMade;
	#ifdef ENABLEIMGUI
	if (!bGridMade)
	{
		bUpdateVeg = true;
	}
	#endif
	UpdateGrassTexture ( g.gpbroverride );
	SetImageAutoMipMap ( 0 );
}

void grass_setgrassgridandfade ( void )
{

	#ifdef VRTECH
	t.terrain.vegetationgridsize = t.visuals.vegetationmode * 0.3;
	#else
	t.terrain.vegetationgridsize = (20+(t.visuals.vegetationmode*0.8f)) * 0.3;
	#endif
	t.tGrassFadeDistance = terrain_veg_areawidth * (t.terrain.vegetationgridsize/2 - 1);
}

void grass_loop ( void )
{
	// early exit if no veg used
	if ( t.visuals.vegetationmode == 0  )  return;

	// if in superflat mode we call a version of update which is much faster
	if ( t.terrain.superflat == 1 ) 
	{
		UpdateSuperFlat ( CameraPositionX(0), CameraPositionZ(0), TERRAIN_SUPERFLAT_HEIGHT, g.postprocessimageoffset+5 );
	}
	else
	{
		// do we need to update after an terrain raise
		if ( t.terrain.grassupdateafterterrain == 1 ) 
		{
			#ifdef ENABLEIMGUI
			// seems we need to ensure grass texture copies to grass memblock so the latest data can be turned into
			// grass with the function below (seems this functionality disappeared during MAX development for VRQ)
			grass_updategrassfrombitmap();
			#endif

			// region affected
			t.tRegionX1 = t.terrain.grassregionx1 * 50;
			t.tRegionX2 = t.terrain.grassregionx2 * 50;
			t.tRegionZ1 = t.terrain.grassregionz1 * 50;
			t.tRegionZ2 = t.terrain.grassregionz2 * 50;
			if (t.tRegionX1 != t.terrain.lastgrassupdatex1 || t.tRegionX2 != t.terrain.lastgrassupdatex2 || t.tRegionZ1 != t.terrain.lastgrassupdatez1 || t.tRegionZ2 != t.terrain.lastgrassupdatez2)
			{
				UpdateVegZoneBlitzTerrain (  t.tRegionX1,t.tRegionZ1,t.tRegionX2,t.tRegionZ2,t.terrain.TerrainID );
				t.terrain.lastgrassupdatex1 = t.tRegionX1;
				t.terrain.lastgrassupdatex2 = t.tRegionX2;
				t.terrain.lastgrassupdatez1 = t.tRegionZ1;
				t.terrain.lastgrassupdatez2 = t.tRegionZ2;
				t.terrain.grassupdateafterterrain = 0;
			}
		}
		if ( t.hardwareinfoglobals.nograss == 0 ) 
		{
			UpdateBlitzTerrain ( CameraPositionX(0),CameraPositionZ(0),t.terrain.TerrainID,g.postprocessimageoffset+5 );
		}
	}

}

void grass_clearregion ( void )
{
	//  clear the grass memblock in the described terrain region
	t.tvegareax1=((t.terrain.terrainregionx1+0.0)/5)-1;
	t.tvegareax2=((t.terrain.terrainregionx2+0.0)/5)+1;
	t.tvegareaz1=((t.terrain.terrainregionz1+0.0)/5)-1;
	t.tvegareaz2=((t.terrain.terrainregionz2+0.0)/5)+1;
	if (  t.tvegareax1<0  )  t.tvegareax1 = 0;
	if (  t.tvegareax2>MAXTEXTURESIZE-1  )  t.tvegareax2 = MAXTEXTURESIZE - 1;
	if (  t.tvegareaz1<0  )  t.tvegareaz1 = 0;
	if (  t.tvegareaz2>MAXTEXTURESIZE-1  )  t.tvegareaz2 = MAXTEXTURESIZE - 1;
	for ( t.tvz = t.tvegareaz1 ; t.tvz<=  t.tvegareaz2; t.tvz++ )
	{
		t.tZStep = t.tvz*MAXTEXTURESIZE;
		for ( t.tvx = t.tvegareax1 ; t.tvx<=  t.tvegareax2; t.tvx++ )
		{
			WriteMemblockByte (  t.terrain.grassmemblock,(4+4+4+((t.tvx+t.tZStep)*4))+2,0 );
		}
	}
}

void grass_updatedirtyregionfast ( void )
{
	// used when in F9 mode and using grass paint mode (0 key)
	// scan vegmap image and extract grass values into veggrass memblock
	SetCurrentBitmap (  g.terrainworkbitmapindex );

	PasteImage (  t.terrain.imagestartindex+2,0,0 );
	LockPixels (  );
	//  *2 as the veggrass aray is 2048 vs 1024 terraingrid coords
	t.tValue = t.terrain.grassregionz1*2 ; grass_clamptomemblockres() ; t.tBack = t.tValue;
	t.tValue = t.terrain.grassregionz2*2 ; grass_clamptomemblockres() ; t.tFront = t.tValue;
	t.tValue = t.terrain.grassregionx1*2 ; grass_clamptomemblockres() ; t.tLeft = t.tValue;
	t.tValue = t.terrain.grassregionx2*2 ; grass_clamptomemblockres() ; t.tRight = t.tValue;
	for ( t.tvz = t.tBack ; t.tvz<=  t.tFront; t.tvz++ )
	{
		t.tZStep = t.tvz*MAXTEXTURESIZE;
		for ( t.tvx = t.tLeft ; t.tvx<=  t.tRight; t.tvx++ )
		{
			WriteMemblockByte (  t.terrain.grassmemblock,(4+4+4+((t.tvx+t.tZStep)*4))+2,RgbR(GetPoint(t.tvx,t.tvz)) );
		}
	}
	UnlockPixels (  );
	SetCurrentBitmap (  0 );
	t.tRegionX1 = t.terrain.grassregionx1 * 50;
	t.tRegionX2 = t.terrain.grassregionx2 * 50;
	t.tRegionZ1 = t.terrain.grassregionz1 * 50;
	t.tRegionZ2 = t.terrain.grassregionz2 * 50;
	if (  t.tRegionX1  !=  t.terrain.lastgrassupdatex1 || t.tRegionX2  !=  t.terrain.lastgrassupdatex2 || t.tRegionZ1  !=  t.terrain.lastgrassupdatez1 || t.tRegionZ2  !=  t.terrain.lastgrassupdatez2 ) 
	{
		if (  t.terrain.superflat  ==  1 ) 
		{
			UpdateVegZoneSuperFlat (  t.tRegionX1,t.tRegionZ1,t.tRegionX2,t.tRegionZ2,TERRAIN_SUPERFLAT_HEIGHT );
		}
		else
		{
			UpdateVegZoneBlitzTerrain (  t.tRegionX1,t.tRegionZ1,t.tRegionX2,t.tRegionZ2,t.terrain.TerrainID );
		}
		t.terrain.lastgrassupdatex1 = t.tRegionX1;
		t.terrain.lastgrassupdatex2 = t.tRegionX2;
		t.terrain.lastgrassupdatez1 = t.tRegionZ1;
		t.terrain.lastgrassupdatez2 = t.tRegionZ2;
	}
}

void grass_clamptomemblockres ( void )
{
	//  clamps input value tValue to the memblock resolution
	if (  t.tValue < 0  )  t.tValue  =  0;
	if (  t.tValue  >=  MAXTEXTURESIZE  )  t.tValue  =  MAXTEXTURESIZE - 1;
}

void grass_updategrassfrombitmap ( void )
{
	// if no image, must have had a VIDMEM reset, leave quietly
	if ( ImageExist(t.terrain.imagestartindex+2) == 0  )  return;
	// uses raw image data inside VEG module
	if ( MemblockExist(t.terrain.grassmemblock) == 1  )  DeleteMemblock (  t.terrain.grassmemblock );
	CreateMemblockFromImage ( t.terrain.grassmemblock, t.terrain.imagestartindex+2 );
	
	//this was only required because images are loading in RGBA, when they should be BGRA (as it was with DX9!!)
	//ConvertVegMemblock ( t.terrain.grassmemblock );
    PWGrassMix::InvalidateMask();

	// slopes and water can't exist in superflat mode, so only delete invalid grass in terrain mode
	if ( t.terrain.superflat == 0  )  DeleteInvalidGrass (  t.terrain.TerrainID,t.terrain.waterliney_f,1.0 );
}

void grass_loadgrass ( void )
{
	// load grass data memblock
	if ( FileExist(t.tfileveggrass_s.Get()) == 1 ) 
	{
		OpenToRead ( 3, t.tfileveggrass_s.Get() );
		if ( MemblockExist(t.terrain.grassmemblock) ) DeleteMemblock ( t.terrain.grassmemblock );
		ReadMemblock ( 3, t.terrain.grassmemblock );
		CloseFile ( 3 );

		//  151214 - if old grass memblock, reconstruct as new raw image grass memblock
		if (  ReadMemblockDWord(t.terrain.grassmemblock,8) != 32 ) 
		{
			DeleteMemblock (  t.terrain.grassmemblock );
			MakeMemblock (  t.terrain.grassmemblock,4+4+4+((MAXTEXTURESIZE*MAXTEXTURESIZE)*4) );
			WriteMemblockDWord (  t.terrain.grassmemblock,0,MAXTEXTURESIZE );
			WriteMemblockDWord (  t.terrain.grassmemblock,4,MAXTEXTURESIZE );
			WriteMemblockDWord (  t.terrain.grassmemblock,8,32 );
		}

		//  This helper call removes all grass entries in the grass memblock where the grass exists on a steep slope or under water.
		//  It can be called at any time when slope grass needs to be removed, but SetResourceValues (  must have been called earlier. )
		//  It is quite slow because it cycles through several million memblock entries and makes ground height calcs for each!
		//  Params; terrain ID, waterheight, max height difference over 1 unit
		if (  t.terrain.superflat == 0  )  DeleteInvalidGrass (  t.terrain.TerrainID,t.terrain.waterliney_f,1.0 );
	}
	else
	{
		grass_buildblankgrass ( );
	}
}

void grass_savegrass ( void )
{
	// regenerate the memblock from the vegmask bitmap for consistency
	t.terrain.grassregionupdate=0;
	grass_updategrassfrombitmap ( );

	// save grass memblock to disk
	if ( FileExist(t.tfileveggrass_s.Get()) == 1 ) DeleteAFile ( t.tfileveggrass_s.Get() );
	if ( OpenToWriteEx ( 3, t.tfileveggrass_s.Get() ) == true )
	{
		WriteMemblock (  3,t.terrain.grassmemblock );
		CloseFile (  3 );
	}
}

void grass_buildblankgrass ( void )
{
	//  make a blank grass data memblock, or clear the one that already exists
	//  151214 - wrong format but VEH module deals with nonraw-image memblocks as ZERO
	if (  MemblockExist(t.terrain.grassmemblock)  ==  0 ) 
	{
		MakeMemblock (  t.terrain.grassmemblock,4+4+4+((MAXTEXTURESIZE*MAXTEXTURESIZE)*4) );
	}
	WriteMemblockDWord (  t.terrain.grassmemblock,0,MAXTEXTURESIZE );
	WriteMemblockDWord (  t.terrain.grassmemblock,4,MAXTEXTURESIZE );
	WriteMemblockDWord (  t.terrain.grassmemblock,8,32 );
	t.tPindex=4+4+4;
	for ( t.tP = 0 ; t.tP<=  MAXTEXTURESIZE*MAXTEXTURESIZE - 1; t.tP++ )
	{
		WriteMemblockByte (  t.terrain.grassmemblock,t.tPindex+2,0 );
		t.tPindex += 4;
	}
	grass_savegrass ( );
}

void grass_buildblankgrass_fornew ( void )
{
	void SetVegetationGridVisibleForce(bool bShow);

	SetVegetationGridVisibleForce(false);
	//  Create new memblock for grass
	if (  MemblockExist(t.terrain.grassmemblock) == 1  )  DeleteMemblock (  t.terrain.grassmemblock );
	if (  MemblockExist(t.terrain.grassmemblock) == 0 ) 
	{
		MakeMemblock (  t.terrain.grassmemblock,4+4+4+((MAXTEXTURESIZE*MAXTEXTURESIZE)*4) );
		WriteMemblockDWord (  t.terrain.grassmemblock,0,MAXTEXTURESIZE );
		WriteMemblockDWord (  t.terrain.grassmemblock,4,MAXTEXTURESIZE );
		WriteMemblockDWord (  t.terrain.grassmemblock,8,32 );
	}

	// save grass memblock to disk
	// 151214 - wrong format but VEH module deals with nonraw-image memblocks as ZERO
	if ( FileExist(t.tfileveggrass_s.Get()) == 1 ) DeleteAFile ( t.tfileveggrass_s.Get() );
	if ( OpenToWriteEx ( 3, t.tfileveggrass_s.Get() ) == true )
	{
		WriteMemblock ( 3, t.terrain.grassmemblock );
		CloseFile ( 3 );
	}

//	int iX, iZ;
//	for (int iObj = iGridObjectStart; iObj < iGridObjectEnd + 1; iObj++) {
//		if (ObjectExist(iObj) == 1) {
//			HideObject(iObj);
//		}
//	}

}

void grass_free ( void )
{
    PWGrassMix::ResetInstances();
	//  We used to DeleteVegetationGrid (  here to clear away veg. Now to save time during testing we HideVegetationGrid (  instead. ) )
	HideVegetationGrid (  );
}

void grass_changevegstyle(void)
{
	// replace vegetaion mesh and texture
	g.vegstyle_s = t.vegstylebank_s[g.vegstyleindex];
	if (PathExist(cstr(cstr("vegbank\\") + g.vegstyle_s).Get()) == 1)
	{
		grass_setgrassimage();
	}
}

//
// Separated out grass editing part from terrain
//

void grass_editcontrol(void)
{
	// this replaces the old terrain based paint system, retaining old grass approach for now
	// locate terrain coordinate
	t.terrain.X_f = t.inputsys.localx_f;
	t.terrain.Y_f = t.inputsys.localy_f;

	//  Height at camera coordinate
	if (t.terrain.TerrainID > 0)
		t.terrain.camheightatcoord_f = BT_GetGroundHeight(t.terrain.TerrainID, t.terrain.X_f, t.terrain.Y_f);
	else
		t.terrain.camheightatcoord_f = g.gdefaultterrainheight;

	// Only control keys while not editing entities
	if (t.conkit.entityeditmode == 0)
	{
		// Control grass painter mode
		t.tmin = 50;
		if (t.inputsys.k_s == "0")  t.terrain.terrainpaintermode = 10;
		if (t.inputsys.k_s == "-" && t.terrain.RADIUS_f > t.tmin)  t.terrain.RADIUS_f = t.terrain.RADIUS_f - (25 * t.terrain.ts_f);
		if (t.inputsys.k_s == "=" && t.terrain.RADIUS_f < g.fTerrainBrushSizeMax)  t.terrain.RADIUS_f = t.terrain.RADIUS_f + (25 * t.terrain.ts_f);

		// Any click means we modified the project
		t.mc = t.inputsys.mclick; if (t.mc == 2 || t.mc == 4)  t.mc = 0;
		if (t.mc != 0)
		{
			g.projectmodified = 1;
			gridedit_changemodifiedflag();
		}

		// record before area modified
		//if ( t.mc != 0 ) 
		//{ 
		//	// 301115 - wipe but can create new entity action below (for restoring entities displayed by terrain)
		//	if ( t.terrain.lastmc == 0 ) t.entityundo.action=0;
		//	terrain_recordbuffer ( );
		//}

		// Paint grass
		//if ( t.terrain.terrainpaintermode >= 6 && t.terrain.terrainpaintermode <= 10 ) 
		if (t.terrain.terrainpaintermode == 10)
		{
			// handle painting of the grass itself
			if (t.mc == 1 && (t.terrain.X_f != t.terrain.lastpaintx_f || t.terrain.Y_f != t.terrain.lastpaintz_f))
			{
				//  paint coord
				t.terrain.lastpaintx_f = t.terrain.X_f;
				t.terrain.lastpaintz_f = t.terrain.Y_f;
				if (t.terrain.terrainpaintermode == 10)
				{
					//  painting grass
					t.x = t.terrain.X_f / 50.0; t.z = t.terrain.Y_f / 50.0; t.r = 1 + (t.terrain.RADIUS_f / 50.0);
					if (t.terrain.grassregionupdate == 0)
					{
						t.terrain.grassregionupdate = 1;
						t.terrain.grassregionx1 = t.x;
						t.terrain.grassregionx2 = t.x;
						t.terrain.grassregionz1 = t.z;
						t.terrain.grassregionz2 = t.z;
					}
					if (t.x - t.r < t.terrain.grassregionx1)  t.terrain.grassregionx1 = t.x - t.r;
					if (t.x + t.r > t.terrain.grassregionx2)  t.terrain.grassregionx2 = t.x + t.r;
					if (t.z - t.r < t.terrain.grassregionz1)  t.terrain.grassregionz1 = t.z - t.r;
					if (t.z + t.r > t.terrain.grassregionz2)  t.terrain.grassregionz2 = t.z + t.r;
					if (t.terrain.grassregionx1 < 0)  t.terrain.grassregionx1 = 0;
					if (t.terrain.grassregionx2 > 1024)  t.terrain.grassregionx2 = 1024;
					if (t.terrain.grassregionz1 < 0)  t.terrain.grassregionz1 = 0;
					if (t.terrain.grassregionz2 > 1024)  t.terrain.grassregionz2 = 1024;

					// grass value stored in RED component
					#ifndef PRODUCTCLASSIC
					extern int iTerrainPaintMode;
					if (t.inputsys.keyshift == 1 || iTerrainPaintMode == 0)
						t.texselect = Rgb(0, 0, 0);
					else
						t.texselect = Rgb(255, 0, 0);

					extern bool bVegHasChanged;
					bVegHasChanged = true;
					#else
					if (t.inputsys.keyshift == 1 )
						t.texselect = Rgb(0, 0, 0);
					else
						t.texselect = Rgb(255, 0, 0);
					#endif
				}

				// finally paint to grass (old veghshadow) texture
				grass_paint();
			}
		}

		// allows second sculpt/paint to erase first undo buffer
		//if ( t.mc == 0  )  t.terrainundo.mode = 0;
	}
}

void grass_paint(void)
{
	// this replaces the old terrain based paint system, retaining old grass approach for now
	// receives terrain.X# Y# RADIUS# texselect texpainttype
	// create memblock for image manipulation
	int iMemblockForGrassPaint = 123;
	if (MemblockExist(iMemblockForGrassPaint) == 0)
	{
		image_setlegacyimageloading(true);
		if (ImageExist(t.terrain.imagestartindex + 2) == 0) LoadImage("vegbank\\AllGrass.png", t.terrain.imagestartindex + 2);
		image_setlegacyimageloading(false);
		CreateMemblockFromImage(iMemblockForGrassPaint, t.terrain.imagestartindex + 2);
	}
	if (MemblockExist(iMemblockForGrassPaint) == 1)
	{
		// memblock header
		t.twid = ReadMemblockDWord(iMemblockForGrassPaint, 0);
		t.thig = ReadMemblockDWord(iMemblockForGrassPaint, 4);
		t.tdep = ReadMemblockDWord(iMemblockForGrassPaint, 8);

		// predictable rotation seed through valid grass types to use
		int iPredRotGrassTypeIndex = 0;

		// calculate coordinates to draw to
		t.tcntx = int(t.terrain.X_f / 25.0) + 1;
		t.tcntz = int(t.terrain.Y_f / 25.0) + 1;
		t.trad_f = t.terrain.RADIUS_f / 35.0;
		t.trad = int(t.trad_f) - 1;
		t.tzs = t.tcntz - t.trad; if (t.tzs < 0) t.tzs = 0;
		t.tzf = t.tcntz + t.trad; if (t.tzf > 2047) t.tzf = 2047;
		for (t.tz = t.tzs; t.tz <= t.tzf; t.tz++)
		{
			t.tti = (((t.tz - t.tzs) + 0.0) / ((t.tzf + 0.0) - (t.tzs + 0.0))) * 100;
			if (t.tti < 0) t.tti = 0;
			if (t.tti > 100) t.tti = 100;
			t.txs = t.tcntx - (t.curve_f[t.tti] * t.trad); if (t.txs < 0) t.txs = 0;
			t.txf = t.tcntx + (t.curve_f[t.tti] * t.trad); if (t.txf > 2047) t.txf = 2047;
			for (t.tx = t.txs; t.tx <= t.txf; t.tx++)
			{
				t.mpos = 12 + (((t.tz * 2048) + t.tx) * 4);
				t.mrgb = ReadMemblockDWord(iMemblockForGrassPaint, t.mpos);
				t.tnewr = (RgbR(t.mrgb)*0.85f) + (RgbR(t.texselect)*0.15f);
				t.tnewg = RgbG(t.mrgb);
				t.tnewb = RgbB(t.mrgb);
				WriteMemblockDWord(iMemblockForGrassPaint, t.mpos, Rgb(t.tnewr, t.tnewg, t.tnewb));
			}
		}
		CreateImageFromMemblock(t.terrain.imagestartindex + 2, iMemblockForGrassPaint);
		TextureObject(t.terrain.terrainobjectindex, 0, t.terrain.imagestartindex + 2);

		// this triggers grass memblock to be updated
	}
}



// -------------------------------------------------------------------------
// Pixelwolf weighted multi-grass renderer (bounded draw budget).
// Classic's native vegetation module has one model+atlas only. This renderer
// instances the actual enabled grass-style meshes against its painted mask;
// it does NOT claim that a shader-only texture blend changes grass species.
// -------------------------------------------------------------------------
namespace PWGrassMix {
Slot slots[MaxStyles];
unsigned revision = 1;

struct RenderStyle { int mesh=0, image=0, slot=-1; bool owned=false; };
static RenderStyle render[MaxStyles];
static int nativeCount=0;
static bool ready=false, previewEnabled=false;
static std::string lastIssue;
bool Ready(){return ready;}
bool PreviewEnabled(){return previewEnabled;}
bool RenderEnabled(){return previewEnabled && Active();}
bool OwnsGrid(){return ready && nativeCount>1;}
bool ProbeEnabled(){return false;}
void SetProbeEnabled(bool){}
void SetInstancesVisible(bool){} // native grid visibility covers every batch
void InvalidateMask(){} // native painted mask is refreshed by Classic
void Refresh(float,float,int,bool,float,int,float){} // legacy call compatibility
void ResetInstances(){
    SetPWVegetationMix(0,nullptr,nullptr,nullptr);
    for(auto& rs:render){
        if(rs.owned){
            if(rs.mesh && GetMeshExist(rs.mesh)) DeleteMesh(rs.mesh);
            if(rs.image && ImageExist(rs.image)) DeleteImage(rs.image);
        }
        rs=RenderStyle();
    }
    nativeCount=0; ready=false;
}
void SetPreviewEnabled(bool enabled){if(previewEnabled!=enabled){previewEnabled=enabled; Dirty();}}
std::string Diagnostics(){
    std::ostringstream out;
    out << "Native grass mix " << (RenderEnabled()?"ON":"OFF");
    for(int s=0;s<nativeCount;++s)
        out << " | " << slots[render[s].slot].style << ": " << GetPWVegetationClumpCount(s) << " clumps/patch";
    if(!lastIssue.empty()) out << " | issue: " << lastIssue;
    return out.str();
}
static std::string ConfigPath(const std::string& folder) {
    if (folder.empty()) return "pw_grassmix.cfg";
    return folder + ((folder.back() == '\\' || folder.back() == '/') ? "" : "\\")
        + "pw_grassmix.cfg";
}
void Save(const std::string& levelFolder) {
    const std::string cfg = ConfigPath(levelFolder);
    if (!Active()) { DeleteFileA(cfg.c_str()); return; }
    std::ofstream out(cfg, std::ios::binary | std::ios::trunc);
    if (!out) return;
    out << "PW_GRASS_MIX_V1\n";
    for (int i = 0; i < MaxStyles; ++i)
        out << (slots[i].enabled ? 1 : 0) << "|" << slots[i].weight << "|" << slots[i].style << "\n";
}
void Load(const std::string& levelFolder) {
    // Reset runtime objects while switching maps. Valid saved multi-style
    // configurations now restore their secondary overlays automatically;
    // Classic native grass always remains visible as the safe base.
    previewEnabled = false;
    ResetInstances();
    lastIssue.clear();
    for (int i = 0; i < MaxStyles; ++i) slots[i] = Slot();
    std::ifstream in(ConfigPath(levelFolder), std::ios::binary);
    std::string line;
    if (!std::getline(in, line) || line != "PW_GRASS_MIX_V1") {
        ++revision; return;
    }
    for (int i = 0; i < MaxStyles && std::getline(in, line); ++i) {
        const size_t a = line.find('|'), b = line.find('|', a == std::string::npos ? 0 : a + 1);
        if (a == std::string::npos || b == std::string::npos) continue;
        const std::string style = line.substr(b + 1);
        float weight = 0.f;
        if (style.empty() || !PWStyleImport::SafeName(style) ||
            ::sscanf_s(line.substr(a + 1, b - a - 1).c_str(), "%f", &weight) != 1)
            continue;
        slots[i].enabled = line[0] == '1';
        slots[i].weight = (std::max)(0.f, (std::min)(100.f, weight));
        slots[i].style = style;
    }
    // Previously a correctly saved 2+-style mix silently reloaded in
    // Classic-only mode, so the owner always saw just one grass species.
    // Allow the optional overlay to resume for a valid saved configuration.
    // The UI can still turn it off for an immediate Classic-only fallback.
    previewEnabled = Active();
    ++revision;
}

bool PrepareNative(){
    lastIssue.clear();
    if(!RenderEnabled()) return false;
    int meshes[MaxStyles]={}, images[MaxStyles]={};
    float weights[MaxStyles]={};
    // Native base first; all other slots use distinct mesh and atlas IDs.
    int primary=-1;
    for(int s=0;s<MaxStyles;++s)
        if(slots[s].enabled && slots[s].weight>0 && PWStyleImport::Same(slots[s].style,g.vegstyle_s.Get())) {primary=s;break;}
    if(primary<0){lastIssue="native base style unavailable";return false;}
    render[0].mesh=t.tGrassObj; render[0].image=t.tGrassImg; render[0].slot=primary;
    meshes[0]=t.tGrassObj; images[0]=t.tGrassImg; weights[0]=slots[primary].weight; nativeCount=1;
    for(int s=0;s<MaxStyles;++s){
        if(s==primary || !slots[s].enabled || slots[s].weight<=0 || slots[s].style.empty()) continue;
        const std::string dir=std::string("vegbank\\")+slots[s].style;
        std::string model=dir+"\\veg.DBO";
        if(!FileExist((LPSTR)model.c_str())) model=dir+"\\veg.X";
        std::string atlas;
        const char* extensions[]={".dds",".png",".jpg",".jpeg",".bmp",".tga"};
        for(const char* ext:extensions){std::string candidate=dir+"\\grass"+ext;if(FileExist((LPSTR)candidate.c_str())){atlas=candidate;break;}}
        if(!FileExist((LPSTR)model.c_str()) || atlas.empty()){
            lastIssue="missing native mesh/atlas for "+slots[s].style; ResetInstances();return false;
        }
        auto& rs=render[nativeCount]; rs.owned=true;rs.slot=s;
        for(int id=229000;id<230000;++id) if(!ObjectExist(id)&&!GetMeshExist(id)){rs.mesh=id;break;}
        for(int id=125000;id<126000;++id) if(!ImageExist(id)){rs.image=id;break;}
        if(!rs.mesh||!rs.image){lastIssue="no free vegetation resource IDs";ResetInstances();return false;}
        LoadObject((LPSTR)model.c_str(),rs.mesh);
        if(!ObjectExist(rs.mesh)){lastIssue="could not load grass mesh for "+slots[s].style;ResetInstances();return false;}
        const auto opt=PWEnvironmentStyle::Read(dir);
        const float scale=200.f*opt.percent/100.f;
        ScaleObject(rs.mesh,scale,scale,scale);
        if(opt.yaw!=0.f) YRotateObject(rs.mesh,opt.yaw);
        MakeMeshFromObject(rs.mesh,rs.mesh);
        DeleteObject(rs.mesh);
        SetImageAutoMipMap(1);
        LoadImage((LPSTR)atlas.c_str(),rs.image);
        SetImageAutoMipMap(0);
        if(!GetMeshExist(rs.mesh)||!ImageExist(rs.image)){lastIssue="could not prepare native grass assets for "+slots[s].style;ResetInstances();return false;}
        meshes[nativeCount]=rs.mesh; images[nativeCount]=rs.image; weights[nativeCount]=slots[s].weight; ++nativeCount;
    }
    if(nativeCount<2){ResetInstances();return false;}
    SetPWVegetationMix(nativeCount,meshes,images,weights);
    ready=true;
    return true;
}
} // namespace PWGrassMix

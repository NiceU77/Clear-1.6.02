//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

#include <maya/MSimple.h>
#include <maya/MIOStream.h>
#include <maya/MRenderView.h>
#include <maya/M3dView.h>
#include <math.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

//
//	renderViewInteractiveRender command declaration
//
class renderViewInteractiveRender : public MPxCommand 
{							
public:					
	renderViewInteractiveRender() {};
	~renderViewInteractiveRender() {};

	virtual MStatus	doIt ( const MArgList& );
	
	static void*	creator();
	
	static MSyntax	newSyntax();
	MStatus parseSyntax (MArgDatabase &argData);

	static const char * cmdName;

	private:

	RV_PIXEL evaluate(int x, int y) const;

	bool fullRefresh;
	bool immediateRefresh;
	bool doNotClearBackground;
	bool verbose;
	double radius;
	unsigned int size[2];
	unsigned int tileSize[2];
	unsigned int numberLoops;
	RV_PIXEL color1;
	RV_PIXEL color2;
};

static const char * kVerbose					= "-v";
static const char * kVerboseLong				= "-verbose";
static const char * kDoNotClearBackground		= "-b";
static const char * kDoNotClearBackgroundLong	= "-background";
static const char * kRadius						= "-r";
static const char * kRadiusLong					= "-radius";
static const char * kSizeX						= "-sx";
static const char * kSizeXLong					= "-sizeX";
static const char * kSizeY						= "-sy";
static const char * kSizeYLong					= "-sizeY";
static const char * kSizeTileX					= "-tx";
static const char * kSizeTileXLong				= "-sizeTileX";
static const char * kSizeTileY					= "-ty";
static const char * kSizeTileYLong				= "-sizeTileY";
static const char * kNumberLoops				= "-nl";
static const char * kNumberLoopsLong			= "-numberLoops";
static const char * kImmediateRefresh			= "-ir";
static const char * kImmediateRefreshLong		= "-immediateRefresh";
static const char * kFullRefresh				= "-fr";
static const char * kFullRefreshLong			= "-fullRefresh";

const char * renderViewInteractiveRender::cmdName = "renderViewInteractiveRender";

void* renderViewInteractiveRender::creator()					
{
	return new renderViewInteractiveRender;
}													

MSyntax renderViewInteractiveRender::newSyntax()
{
	MStatus status;
	MSyntax syntax;

	syntax.addFlag(kDoNotClearBackground, kDoNotClearBackgroundLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kVerbose, kVerboseLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kImmediateRefresh, kImmediateRefreshLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kFullRefresh, kFullRefreshLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kRadius, kRadiusLong, MSyntax::kDouble);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kSizeX, kSizeXLong, MSyntax::kLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kSizeY, kSizeYLong, MSyntax::kLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kSizeTileX, kSizeTileXLong, MSyntax::kLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kSizeTileY, kSizeTileYLong, MSyntax::kLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	syntax.addFlag(kNumberLoops, kNumberLoopsLong, MSyntax::kLong);
	CHECK_MSTATUS_AND_RETURN(status, syntax);

	return syntax;
}

//
// Description:
//		Read the values of the additionnal flags for this command.
//
MStatus renderViewInteractiveRender::parseSyntax (MArgDatabase &argData)
{
	// Get the flag values, otherwise the default values are used.
	doNotClearBackground = argData.isFlagSet( kDoNotClearBackground );
	verbose = argData.isFlagSet( kVerbose );
	fullRefresh = argData.isFlagSet( kFullRefresh );
	immediateRefresh = argData.isFlagSet( kImmediateRefresh );

	radius = 50.;							// pattern frequency, in pixels
	if (argData.isFlagSet( kRadius ))
		argData.getFlagArgument(kRadius, 0, radius);

	size[0] = 640;
	size[1] = 480;
	if (argData.isFlagSet( kSizeX ))
		argData.getFlagArgument(kSizeX, 0, size[0]);
	if (argData.isFlagSet( kSizeY ))
		argData.getFlagArgument(kSizeY, 0, size[1]);

	tileSize[0] = 16;
	tileSize[1] = 16;
	if (argData.isFlagSet( kSizeTileX ))
		argData.getFlagArgument(kSizeTileX, 0, tileSize[0]);
	if (argData.isFlagSet( kSizeTileY ))
		argData.getFlagArgument(kSizeTileY, 0, tileSize[1]);

	numberLoops = 10;
	if (argData.isFlagSet( kNumberLoops ))
		argData.getFlagArgument(kNumberLoops, 0, numberLoops);

	return MS::kSuccess;
}

//
// Description:
//		register the command
//
MStatus initializePlugin( MObject obj )			
{															
	MFnPlugin	plugin( obj, PLUGIN_COMPANY, "4.5" );	
	MStatus		stat;										
	stat = plugin.registerCommand(	renderViewInteractiveRender::cmdName,
									renderViewInteractiveRender::creator,
									renderViewInteractiveRender::newSyntax);	
	if ( !stat )												
		stat.perror( "registerCommand" );							
	return stat;												
}																

//
// Description:
//		unregister the command
//
MStatus uninitializePlugin( MObject obj )						
{																
	MFnPlugin	plugin( obj );									
	MStatus		stat;											
	stat = plugin.deregisterCommand( renderViewInteractiveRender::cmdName );
	if ( !stat )									
		stat.perror( "deregisterCommand" );			
	return stat;									
}

RV_PIXEL renderViewInteractiveRender::evaluate(int x, int y) const
//
//	Description:
//		Generates a simple procedural circular pattern to be sent to the 
//		Render View.
//
//	Arguments:
//		x, y - coordinates in the current tile (the pattern is centred 
//			   around (0,0) ).
//
//	Return Value:
//		An RV_PIXEL structure containing the colour of pixel (x,y).
//
{
	double distance = sqrt(double((x*x) + (y*y))) / radius;
	float percent = (float)(cos(distance*2*3.1415927)/2.+.5);

	RV_PIXEL pixel;
	pixel.r = color1.r * percent + color2.r * (1-percent);
	pixel.g = color1.g * percent + color2.g * (1-percent);
	pixel.b = color1.b * percent + color2.b * (1-percent);
	pixel.a = 255.0f;

	return pixel;
}

MStatus renderViewInteractiveRender::doIt( const MArgList& args )
//
//	Description:
//		Implements the MEL renderViewInteractiveRender command. This command
//		Draws a 640x480 tiled pattern of circles into Maya's Render
//		View window.
//
//	Arguments:
//		args - The argument list that was passed to the command from MEL.
//
//	Return Value:
//		MS::kSuccess - command succeeded
//		MS::kFailure - command failed (returning this value will cause the 
//                     MEL script that is being run to terminate unless the
//                     error is caught using a "catch" statement.
//
{
	MStatus stat = MS::kSuccess;

	// Check if the render view exists. It should always exist, unless
	// Maya is running in batch mode.
	//
	if (!MRenderView::doesRenderEditorExist())
	{
		displayError( 
			"Cannot renderViewInteractiveRender in batch render mode.\n"
			"Run in interactive mode, so that the render editor exists." );
		return MS::kFailure;
	}
	
	// get optional flags
	MArgDatabase argData( syntax(), args );
	parseSyntax( argData );

	// We'll render a 640x480 image.  Tell the Render View to get ready
	// to receive 640x480 pixels of data.
	//
	unsigned int image_width = size[0], image_height = size[1];
	if (MRenderView::startRender( image_width, image_height, 
								  doNotClearBackground, 
								  immediateRefresh) != MS::kSuccess)
	{
		displayError("renderViewInteractiveRender: error occured in startRender.");
		return MS::kFailure;
	}

	// The image will be composed of tiles consisting of circular patterns.
	//

	// Draw each tile
	//
	static float colors[] = { 255, 150,  69, 
							  255,  84, 112,
							  255,  94, 249,
							   86,  62, 255,
							   46, 195, 255,
							  56, 255, 159,
							  130, 255, 64 };
	int indx1 = 0;
	int indx2 = 3*3;

	RV_PIXEL* pixels = new RV_PIXEL[tileSize[0] * tileSize[1]];
	for (unsigned int loopId = 0 ; loopId < numberLoops ; loopId++ )
	{
		color1.r = colors[indx1]; 
		color1.g = colors[indx1+1]; 
		color1.b = colors[indx1+2];
		color1.a = 255;
		indx1 += 3; if (indx1 >= 21) indx1 -= 21;

		color2.r = colors[indx2]; 
		color2.g = colors[indx2+1]; 
		color2.b = colors[indx2+2]; 
		color2.a = 255;
		indx2 += 6; if (indx2 >= 21) indx2 -= 21;

		for (unsigned int min_y = 0; min_y < size[1] ; min_y += tileSize[1] )
		{
			unsigned int max_y = min_y + tileSize[1] - 1;
			if (max_y >= size[1]) max_y = size[1]-1;

			for (unsigned int min_x = 0; min_x < size[0] ; min_x += tileSize[0] )
			{
				unsigned int max_x = min_x + tileSize[0] - 1;
				if (max_x >= size[0]) max_x = size[0]-1;

				// Fill up the pixel array with some the pattern, which is 
				// generated by the 'evaluate' function.  The Render View
				// accepts floating point pixel values only.
				//
				unsigned int index = 0;
				for (unsigned int j = min_y; j <= max_y; j++ )
				{
					for (unsigned int i = min_x; i <= max_x; i++)
					{
						pixels[index] = evaluate(i, j);
						index++;
					}
				}

				// Send the data to the render view.
				//
				if (MRenderView::updatePixels(min_x, max_x, min_y, max_y, pixels) 
					!= MS::kSuccess)
				{
					displayError( "renderViewInteractiveRender: error occured in updatePixels." );
					delete [] pixels;
					return MS::kFailure;
				}

				// Force the Render View to refresh the display of the
				// affected region.
				//
				MStatus st;
				if (fullRefresh)
					st =MRenderView::refresh(0, image_width-1, 0, image_height-1);
				else
					st = MRenderView::refresh(min_x, max_x, min_y, max_y);
				if (st != MS::kSuccess)
				{
					displayError( "renderViewInteractiveRender: error occured in refresh." );
					delete [] pixels;
					return MS::kFailure;
				}

				if (verbose)
					cerr << "Tile "<<min_x<<", "<<min_y<<
						" (iteration "<<loopId<<")completed\n";
			}
		}
	}

	delete [] pixels;

	// Inform the Render View that we have completed rendering the entire image.
	//
	if (MRenderView::endRender() != MS::kSuccess)
	{
		displayError( "renderViewInteractiveRender: error occured in endRender." );
		return MS::kFailure;
	}

	displayError( "renderViewInteractiveRender completed." );
	return stat;
}





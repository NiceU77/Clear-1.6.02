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

////////////////////////////////////////////////////////////////////////
// 
// swissArmyManip.cpp
// 
// This plug-in is an example of a user-defined manipulator,
// which is comprised of a variety of the base manipulators:
// - MFnCircleSweepManip.h
// - MFnDirectionManip.h
// - MFnDiscManip.h
// - MFnDistanceManip.h
// - MFnFreePointTriadManip.h
// - MFnStateManip.h
// - MFnToggleManip.h
// - MFnRotateManip.h
// - MFnScaleManip.h
//
// To use this plug-in:
// 
//     mel: loadPlugin "swissArmyManip.so";
//     mel: createNode swissArmyLocator;
//     click on the showManipTool
// 
////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <maya/MIOStream.h>

#include <maya/MPxNode.h>   

#include <maya/MPxLocatorNode.h> 

#include <maya/MString.h> 
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>
#include <maya/MVector.h>
#include <maya/MAngle.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MFnPlugin.h>
#include <maya/MDistance.h>

#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>

#include <maya/MFn.h>
#include <maya/MPxNode.h>
#include <maya/MPxManipContainer.h> 

#include <maya/MFnCircleSweepManip.h>
#include <maya/MFnDirectionManip.h>
#include <maya/MFnDiscManip.h>
#include <maya/MFnDistanceManip.h> 
#include <maya/MFnFreePointTriadManip.h>
#include <maya/MFnStateManip.h>
#include <maya/MFnToggleManip.h>
#include <maya/MFnRotateManip.h>
#include <maya/MFnScaleManip.h>

#include <maya/MPxContext.h>
#include <maya/MPxSelectionContext.h>

#include <maya/MFnNumericData.h>
#include <maya/MManipData.h>

#define e \
    counter++; \
    if (MS::kSuccess != s) { \
        cerr << "Status Error in method " << method \
             << " at checkpoint #" << counter << "." << "\n" \
			 << "    in File: " << __FILE__ \
			 << ", at Line: " << __LINE__ << endl; \
        s.perror("Error"); \
    }

const double delta1 = 0.01;
const double delta2 = 0.02;
const double delta3 = 0.03;
const double delta4 = 0.04;

// Locator Data
//
static float centre[][3] = { {  0.10f, 0.0f,  0.10f},
							 {  0.10f, 0.0f, -0.10f },
							 { -0.10f, 0.0f, -0.10f },
							 { -0.10f, 0.0f,  0.10f }, 
							 {  0.10f, 0.0f,  0.10f } }; 
static float state1[][3] = { {  1.00f, 0.0f,  1.00f},
							 {  1.00f, 0.0f,  0.50f },
							 {  0.50f, 0.0f,  0.50f },
							 {  0.50f, 0.0f,  1.00f }, 
							 {  1.00f, 0.0f,  1.00f } }; 
static float state2[][3] = { {  1.00f, 0.0f, -1.00f},
							 {  1.00f, 0.0f, -0.50f },
							 {  0.50f, 0.0f, -0.50f },
							 {  0.50f, 0.0f, -1.00f }, 
							 {  1.00f, 0.0f, -1.00f } }; 
static float state3[][3] = { { -1.00f, 0.0f, -1.00f},
							 { -1.00f, 0.0f, -0.50f },
							 { -0.50f, 0.0f, -0.50f },
							 { -0.50f, 0.0f, -1.00f }, 
							 { -1.00f, 0.0f, -1.00f } }; 
static float state4[][3] = { { -1.00f, 0.0f,  1.00f},
							 { -1.00f, 0.0f,  0.50f },
							 { -0.50f, 0.0f,  0.50f },
							 { -0.50f, 0.0f,  1.00f }, 
							 { -1.00f, 0.0f,  1.00f } }; 
static float arrow1[][3] = { {  0.00f, 0.0f,  1.00f},
							 {  0.10f, 0.0f,  0.20f },
							 { -0.10f, 0.0f,  0.20f },
							 {  0.00f, 0.0f,  1.00f } }; 
static float arrow2[][3] = { {  1.00f, 0.0f,  0.00f},
							 {  0.20f, 0.0f,  0.10f },
							 {  0.20f, 0.0f, -0.10f },
							 {  1.00f, 0.0f,  0.00f } }; 
static float arrow3[][3] = { {  0.00f, 0.0f, -1.00f},
							 {  0.10f, 0.0f, -0.20f },
							 { -0.10f, 0.0f, -0.20f },
							 {  0.00f, 0.0f, -1.00f } }; 
static float arrow4[][3] = { { -1.00f, 0.0f,  0.00f},
							 { -0.20f, 0.0f,  0.10f },
							 { -0.20f, 0.0f, -0.10f },
							 { -1.00f, 0.0f,  0.00f } }; 
static float perimeter[][3] = { {  1.10f, 0.0f,  1.10f},
								{  1.10f, 0.0f, -1.10f },
								{ -1.10f, 0.0f, -1.10f },
								{ -1.10f, 0.0f,  1.10f }, 
								{  1.10f, 0.0f,  1.10f } }; 

static int centreCount = 5;
static int state1Count = 5;
static int state2Count = 5;
static int state3Count = 5;
static int state4Count = 5;
static int arrow1Count = 4;
static int arrow2Count = 4;
static int arrow3Count = 4;
static int arrow4Count = 4;
static int perimeterCount = 5;


class swissArmyLocatorManip : public MPxManipContainer
{
public:
    swissArmyLocatorManip();
    virtual ~swissArmyLocatorManip();
    
    static void * creator();
    static MStatus initialize();
    virtual MStatus createChildren();
    virtual MStatus connectToDependNode(const MObject & node);

    virtual void draw(M3dView & view, 
					  const MDagPath & path, 
					  M3dView::DisplayStyle style,
					  M3dView::DisplayStatus status);
	MManipData startPointCallback(unsigned index) const;
	MVector nodeTranslation() const;

    MDagPath fCircleSweepManip;
    MDagPath fDirectionManip;
    MDagPath fDiscManip;
    MDagPath fDistanceManip;
    MDagPath fFreePointTriadManip;
    MDagPath fStateManip;
    MDagPath fToggleManip;
	MDagPath fRotateManip;
    MDagPath fScaleManip;

	MDagPath fNodePath;

public:
    static MTypeId id;
};


MManipData swissArmyLocatorManip::startPointCallback(unsigned /*index*/) 
const
{
	MManipData manipData;
	MFnNumericData numData;
	MObject numDataObj = numData.create(MFnNumericData::k3Double);
	MVector vec = nodeTranslation();
	numData.setData(vec.x, vec.y, vec.z);
	manipData = numDataObj;
	return manipData;
}


MVector swissArmyLocatorManip::nodeTranslation() const
{
	MFnDagNode dagFn(fNodePath);
	MDagPath path;
	dagFn.getPath(path);
	path.pop();  // pop from the shape to the transform
	MFnTransform transformFn(path);
	return transformFn.translation(MSpace::kWorld);
}


MTypeId swissArmyLocatorManip::id( 0x8001e );


swissArmyLocatorManip::swissArmyLocatorManip() 
{ 
    // Do not call createChildren from here 
}


swissArmyLocatorManip::~swissArmyLocatorManip() 
{
}


void* swissArmyLocatorManip::creator()
{
     return new swissArmyLocatorManip();
}


MStatus swissArmyLocatorManip::initialize()
{ 
    MStatus stat;
    stat = MPxManipContainer::initialize();
    return stat;
}


MStatus swissArmyLocatorManip::createChildren()
{
    MStatus stat = MStatus::kSuccess;
	MString method("swissArmyLocatorManip::createChildren");
	MStatus s;
	int counter = 0;

	// FreePointTriadManip
	//
	fFreePointTriadManip = addFreePointTriadManip("freePointTriadManip",
												  "point");
	MFnFreePointTriadManip freePointTriadManipFn(fFreePointTriadManip);

	// DirectionManip
	//
	fDirectionManip = addDirectionManip("directionManip",
										"direction");
	MFnDirectionManip directionManipFn(fDirectionManip);

	// ToggleManip
	//
	fToggleManip = addToggleManip("toggleManip", "toggle");
	MFnToggleManip toggleManipFn(fToggleManip);

	// StateManip
	//
	fStateManip = addStateManip("stateManip", "state");
	MFnStateManip stateManipFn(fStateManip);

	// DiscManip
	//
	fDiscManip = addDiscManip("discManip",
							  "angle");
	MFnDiscManip discManipFn(fDiscManip);

	// CircleSweepManip
	//
	fCircleSweepManip = addCircleSweepManip("circleSweepManip",
											"angle");
	MFnCircleSweepManip circleSweepManipFn(fCircleSweepManip, &s); e;
	circleSweepManipFn.setCenterPoint(MPoint(0, 0, 0));
	circleSweepManipFn.setNormal(MVector(0, 1, 0));
	circleSweepManipFn.setRadius(2.0);
	circleSweepManipFn.setDrawAsArc(true);

	// DistanceManip
	//
    MString manipName("distanceManip");
    MString distanceName("distance");
    MPoint startPoint(0.0, 0.0, 0.0);
    MVector direction(0.0, 1.0, 0.0);
    fDistanceManip = addDistanceManip(manipName,
									  distanceName);
	MFnDistanceManip distanceManipFn(fDistanceManip);
	distanceManipFn.setStartPoint(startPoint);
	distanceManipFn.setDirection(direction);

	// RotateManip
	//
    MString RotateManipName("RotateManip");
    MString rotateName("rotation");
    fRotateManip = addRotateManip(RotateManipName, rotateName);
	MFnRotateManip rotateManipFn(fRotateManip);

	// ScaleManip
	//
    MString scaleManipName("scaleManip");
    MString scaleName("scale");
    fScaleManip = addScaleManip(scaleManipName, scaleName);
	MFnScaleManip scaleManipFn(fScaleManip);

    return stat;
}


MStatus swissArmyLocatorManip::connectToDependNode(const MObject &node)
{
    MStatus stat;

	// Get the DAG path
	//
	MFnDagNode dagNodeFn(node);
	dagNodeFn.getPath(fNodePath);
	MObject parentNode = dagNodeFn.parent(0);
	MFnDagNode parentNodeFn(parentNode);

    // Connect the plugs
    //    
    MFnDependencyNode nodeFn;
    nodeFn.setObject(node);    

	// FreePointTriadManip
	//
    MFnFreePointTriadManip freePointTriadManipFn(fFreePointTriadManip);
	MPlug translationPlug = parentNodeFn.findPlug("t", &stat);
    if (MStatus::kFailure != stat) {
	    freePointTriadManipFn.connectToPointPlug(translationPlug);
	}

	// DirectionManip
	//
    MFnDirectionManip directionManipFn;
    directionManipFn.setObject(fDirectionManip);
	MPlug directionPlug = nodeFn.findPlug("arrow2Direction", &stat);
    if (MStatus::kFailure != stat) {
	    directionManipFn.connectToDirectionPlug(directionPlug);
		unsigned startPointIndex = directionManipFn.startPointIndex();
	    addPlugToManipConversionCallback(startPointIndex, 
										 (plugToManipConversionCallback) 
										 &swissArmyLocatorManip::startPointCallback);
	}

	// DistanceManip
	//
    MFnDistanceManip distanceManipFn;
    distanceManipFn.setObject(fDistanceManip);
	MPlug sizePlug = nodeFn.findPlug("size", &stat);
    if (MStatus::kFailure != stat) {
	    distanceManipFn.connectToDistancePlug(sizePlug);
		unsigned startPointIndex = distanceManipFn.startPointIndex();
	    addPlugToManipConversionCallback(startPointIndex, 
										 (plugToManipConversionCallback) 
										 &swissArmyLocatorManip::startPointCallback);
	}

	// CircleSweepManip
	//
	MFnCircleSweepManip circleSweepManipFn(fCircleSweepManip);
	MPlug arrow1AnglePlug = nodeFn.findPlug("arrow1Angle", &stat);
    if (MStatus::kFailure != stat) {
	    circleSweepManipFn.connectToAnglePlug(arrow1AnglePlug);
		unsigned centerIndex = circleSweepManipFn.centerIndex();
	    addPlugToManipConversionCallback(centerIndex, 
										 (plugToManipConversionCallback) 
										 &swissArmyLocatorManip::startPointCallback);
	}

	// DiscManip
	//
	MFnDiscManip discManipFn(fDiscManip);
	MPlug arrow3AnglePlug = nodeFn.findPlug("arrow3Angle", &stat);
    if (MStatus::kFailure != stat) {
	    discManipFn.connectToAnglePlug(arrow3AnglePlug);
		unsigned centerIndex = discManipFn.centerIndex();
	    addPlugToManipConversionCallback(centerIndex, 
										 (plugToManipConversionCallback) 
										 &swissArmyLocatorManip::startPointCallback);
	}

	// StateManip
	//
	MFnStateManip stateManipFn(fStateManip);

	MPlug statePlug = nodeFn.findPlug("state", &stat);
    if (MStatus::kFailure != stat) {
	    stateManipFn.connectToStatePlug(statePlug);
		unsigned positionIndex = stateManipFn.positionIndex();
	    addPlugToManipConversionCallback(positionIndex, 
										 (plugToManipConversionCallback) 
										 &swissArmyLocatorManip::startPointCallback);
	}

	// ToggleManip
	//
	MFnToggleManip toggleManipFn(fToggleManip);

	MPlug togglePlug = nodeFn.findPlug("toggle", &stat);
    if (MStatus::kFailure != stat) {
	    toggleManipFn.connectToTogglePlug(togglePlug);
		unsigned startPointIndex = toggleManipFn.startPointIndex();
	    addPlugToManipConversionCallback(startPointIndex, 
										 (plugToManipConversionCallback) 
										 &swissArmyLocatorManip::startPointCallback);
	}

	// Determine the transform node for the locator
	//
	MDagPath transformPath(fNodePath);
	transformPath.pop();

	MFnTransform transformNode(transformPath);

	// RotateManip
	//
	MFnRotateManip rotateManipFn(fRotateManip);

	MPlug rotatePlug = transformNode.findPlug("rotate", &stat);
	if (MStatus::kFailure != stat) {
	    rotateManipFn.connectToRotationPlug(rotatePlug);
		rotateManipFn.displayWithNode(node);
	}

	// ScaleManip
	//
	MFnScaleManip scaleManipFn(fScaleManip);

	MPlug scalePlug = transformNode.findPlug("scale", &stat);
    if (MStatus::kFailure != stat) {
	    scaleManipFn.connectToScalePlug(scalePlug);
		scaleManipFn.displayWithNode(node);
	}

	finishAddingManips();
	MPxManipContainer::connectToDependNode(node);
	
    return stat;
}


void swissArmyLocatorManip::draw(M3dView & view, 
								 const MDagPath &path, 
								 M3dView::DisplayStyle style,
								 M3dView::DisplayStatus status)
{ 
    MPxManipContainer::draw(view, path, style, status);
    view.beginGL(); 

    MPoint textPos = nodeTranslation();
	char str[100];
    sprintf(str, "Swiss Army Manipulator"); 
    MString distanceText(str);
    view.drawText(distanceText, textPos, M3dView::kLeft);
    view.endGL();
}


class swissArmyLocator : public MPxLocatorNode
{
public:
	swissArmyLocator();
	virtual ~swissArmyLocator(); 

    virtual MStatus   		compute(const MPlug& plug, MDataBlock &data);

	virtual void            draw(M3dView &view, const MDagPath &path, 
								 M3dView::DisplayStyle style,
								 M3dView::DisplayStatus status);

	virtual bool            isBounded() const;
	virtual MBoundingBox    boundingBox() const; 

	static  void *          creator();
	static  MStatus         initialize();

	static  MObject         aSize;         // The size of the locator
	static	MObject			aPoint;
	static	MObject			aPointX;
	static	MObject			aPointY;
	static	MObject			aPointZ;
	static	MObject			aArrow1Angle;
	static	MObject			aArrow2Direction;
	static	MObject			aArrow2DirectionX;
	static	MObject			aArrow2DirectionY;
	static	MObject			aArrow2DirectionZ;
	static	MObject			aArrow3Angle;
	static	MObject			aArrow4Distance;
	static	MObject			aState;
	static	MObject			aToggle;

public: 
	static	MTypeId		id;
};


MTypeId swissArmyLocator::id( 0x8001f );
MObject swissArmyLocator::aSize;
MObject swissArmyLocator::aPoint;
MObject swissArmyLocator::aPointX;
MObject swissArmyLocator::aPointY;
MObject swissArmyLocator::aPointZ;
MObject swissArmyLocator::aArrow1Angle;
MObject swissArmyLocator::aArrow2Direction;
MObject swissArmyLocator::aArrow2DirectionX;
MObject swissArmyLocator::aArrow2DirectionY;
MObject swissArmyLocator::aArrow2DirectionZ;
MObject swissArmyLocator::aArrow3Angle;
MObject swissArmyLocator::aArrow4Distance;
MObject swissArmyLocator::aState;
MObject swissArmyLocator::aToggle;


swissArmyLocator::swissArmyLocator() 
{}


swissArmyLocator::~swissArmyLocator() 
{}


MStatus swissArmyLocator::compute(const MPlug &/*plug*/, MDataBlock &/*data*/)
{ 
	return MS::kUnknownParameter;
}


void swissArmyLocator::draw(M3dView &view, const MDagPath &/*path*/, 
							M3dView::DisplayStyle style,
							M3dView::DisplayStatus status)
{ 
	// Get the size
	//
	MObject thisNode = thisMObject();

	MPlug plug(thisNode, aSize);
	MDistance sizeVal;
	plug.getValue(sizeVal);

	MPlug arrow1AnglePlug(thisNode, aArrow1Angle);
	MAngle arrow1Angle;
	arrow1AnglePlug.getValue(arrow1Angle);
	double angle1 = -arrow1Angle.asRadians() - 3.1415927/2.0;

	MPlug arrow3AnglePlug(thisNode, aArrow3Angle);
	MAngle arrow3Angle;
	arrow3AnglePlug.getValue(arrow3Angle);
	double angle3 = arrow3Angle.asRadians();

	MPlug statePlug(thisNode, aState);
	int state;
	statePlug.getValue(state);

	MPlug togglePlug(thisNode, aToggle);
	bool toggle;
	togglePlug.getValue(toggle);

	MPlug directionXPlug(thisNode, aArrow2DirectionX);
	MPlug directionYPlug(thisNode, aArrow2DirectionY);
	MPlug directionZPlug(thisNode, aArrow2DirectionZ);
	double dirX;
	double dirY;
	double dirZ;
	directionXPlug.getValue(dirX);
	directionYPlug.getValue(dirY);
	directionZPlug.getValue(dirZ);
	double angle2 = atan2(dirZ,dirX);
	angle2 += 3.1415927;

	float multiplier = (float) sizeVal.asCentimeters();

	view.beginGL(); 

	if ((style == M3dView::kFlatShaded) || 
		(style == M3dView::kGouraudShaded)) 
	{  
		// Push the color settings
		// 
		glPushAttrib(GL_CURRENT_BIT);

		if (status == M3dView::kActive) {
			view.setDrawColor(13, M3dView::kActiveColors);
		} else {
			view.setDrawColor(13, M3dView::kDormantColors);
		}  

		int i;
		int last;

		if (toggle) {
		if (status == M3dView::kActive)
			view.setDrawColor(15, M3dView::kActiveColors);
		else
			view.setDrawColor(15, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = centreCount - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f(centre[i][0] * multiplier,
						   centre[i][1] * multiplier,
						   centre[i][2] * multiplier);
			}
		glEnd();
		}

		if (state == 0) {
		if (status == M3dView::kActive)
			view.setDrawColor(19, M3dView::kActiveColors);
		else
			view.setDrawColor(19, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = state1Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f(state1[i][0] * multiplier,
						   state1[i][1] * multiplier,
						   state1[i][2] * multiplier);
			}
		glEnd();
		}

		if (state == 1) {
		if (status == M3dView::kActive)
			view.setDrawColor(21, M3dView::kActiveColors);
		else
			view.setDrawColor(21, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = state2Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f(state2[i][0] * multiplier,
						   state2[i][1] * multiplier,
						   state2[i][2] * multiplier);
			}
		glEnd();
		}

		if (state == 2) {
		if (status == M3dView::kActive)
			view.setDrawColor(18, M3dView::kActiveColors);
		else
			view.setDrawColor(18, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = state3Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f(state3[i][0] * multiplier,
						   state3[i][1] * multiplier,
						   state3[i][2] * multiplier);
			}
		glEnd();
		}

		if (state == 3) {
		if (status == M3dView::kActive)
			view.setDrawColor(17, M3dView::kActiveColors);
		else
			view.setDrawColor(17, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = state4Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f(state4[i][0] * multiplier,
						   state4[i][1] * multiplier,
						   state4[i][2] * multiplier);
			}
		glEnd();
		}

		if (status == M3dView::kActive)
			view.setDrawColor(12, M3dView::kActiveColors);
		else
			view.setDrawColor(12, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = arrow1Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f( (float) (- arrow1[i][0] * multiplier * cos(angle1) 
						   - arrow1[i][2] * multiplier * sin(angle1)),
						   (float) (arrow1[i][1] * multiplier + delta1),
						   (float) (arrow1[i][2] * multiplier * cos(angle1) -
						   arrow1[i][0] * multiplier * sin(angle1)));
			}
		glEnd();

		if (status == M3dView::kActive)
			view.setDrawColor(16, M3dView::kActiveColors);
		else
			view.setDrawColor(16, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = arrow2Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f( (float) (- arrow2[i][0] * multiplier * cos(angle2) 
						   - arrow2[i][2] * multiplier * sin(angle2)),
						   (float) (arrow2[i][1] * multiplier + delta2),
						   (float) (arrow2[i][2] * multiplier * cos(angle2) -
						   arrow2[i][0] * multiplier * sin(angle2)));
			}
		glEnd();

		if (status == M3dView::kActive)
			view.setDrawColor(13, M3dView::kActiveColors);
		else
			view.setDrawColor(13, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = arrow3Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f( (float) (- arrow3[i][0] * multiplier * cos(angle3) 
						   - arrow3[i][2] * multiplier * sin(angle3)),
						   (float) (arrow3[i][1] * multiplier + delta3),
						   (float) (arrow3[i][2] * multiplier * cos(angle3) -
						   arrow3[i][0] * multiplier * sin(angle3)));
			}
		glEnd();

		if (status == M3dView::kActive)
			view.setDrawColor(5, M3dView::kActiveColors);
		else
			view.setDrawColor(5, M3dView::kDormantColors);
		glBegin(GL_TRIANGLE_FAN);
			last = arrow4Count - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f((float) (arrow4[i][0] * multiplier),
						   (float) (arrow4[i][1] * multiplier + delta4),
						   (float) (arrow4[i][2] * multiplier));
			}
		glEnd();

		glPopAttrib();
	}

	// Draw the outline of the locator
	//
	glBegin(GL_LINES);
	    int i;
		int last;

		if (toggle) {
		last = centreCount - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(centre[i][0] * multiplier, 
					   centre[i][1] * multiplier, 
					   centre[i][2] * multiplier);
			glVertex3f(centre[i+1][0] * multiplier, 
					   centre[i+1][1] * multiplier, 
					   centre[i+1][2] * multiplier);
		}
		}

		if (state == 0) {
		last = state1Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(state1[i][0] * multiplier, 
					   state1[i][1] * multiplier, 
					   state1[i][2] * multiplier);
			glVertex3f(state1[i+1][0] * multiplier, 
					   state1[i+1][1] * multiplier, 
					   state1[i+1][2] * multiplier);
		}
		}

		if (state == 1) {
		last = state2Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(state2[i][0] * multiplier, 
					   state2[i][1] * multiplier, 
					   state2[i][2] * multiplier);
			glVertex3f(state2[i+1][0] * multiplier, 
					   state2[i+1][1] * multiplier, 
					   state2[i+1][2] * multiplier);
		}
		}

		if (state == 2) {
		last = state3Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(state3[i][0] * multiplier, 
					   state3[i][1] * multiplier, 
					   state3[i][2] * multiplier);
			glVertex3f(state3[i+1][0] * multiplier, 
					   state3[i+1][1] * multiplier, 
					   state3[i+1][2] * multiplier);
		}
		}

		if (state == 3) {
		last = state4Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(state4[i][0] * multiplier, 
					   state4[i][1] * multiplier, 
					   state4[i][2] * multiplier);
			glVertex3f(state4[i+1][0] * multiplier, 
					   state4[i+1][1] * multiplier, 
					   state4[i+1][2] * multiplier);
		}
		}

		last = arrow1Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f((float) (- arrow1[i][0] * multiplier * cos(angle1) 
					   - arrow1[i][2] * multiplier * sin(angle1)),
					   (float) (arrow1[i][1] * multiplier + delta1),
					   (float) (arrow1[i][2] * multiplier * cos(angle1) -
					   arrow1[i][0] * multiplier * sin(angle1)));
			glVertex3f((float) (- arrow1[i+1][0] * multiplier * cos(angle1) 
					   - arrow1[i+1][2] * multiplier * sin(angle1)),
					   (float) (arrow1[i+1][1] * multiplier + delta1),
					   (float) (arrow1[i+1][2] * multiplier * cos(angle1) -
					   arrow1[i+1][0] * multiplier * sin(angle1)));
		}

		last = arrow2Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f((float) (- arrow2[i][0] * multiplier * cos(angle2) 
					   - arrow2[i][2] * multiplier * sin(angle2)),
					   (float) (arrow2[i][1] * multiplier + delta2),
					   (float) (arrow2[i][2] * multiplier * cos(angle2) -
					   arrow2[i][0] * multiplier * sin(angle2)));
			glVertex3f((float) (- arrow2[i+1][0] * multiplier * cos(angle2) 
					   - arrow2[i+1][2] * multiplier * sin(angle2)),
					   (float) (arrow2[i+1][1] * multiplier + delta2),
					   (float) (arrow2[i+1][2] * multiplier * cos(angle2) -
					   arrow2[i+1][0] * multiplier * sin(angle2)));
		}

		last = arrow3Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f((float) (- arrow3[i][0] * multiplier * cos(angle3) 
					   - arrow3[i][2] * multiplier * sin(angle3)),
					   (float) (arrow3[i][1] * multiplier + delta3),
					   (float) (arrow3[i][2] * multiplier * cos(angle3) -
					   arrow3[i][0] * multiplier * sin(angle3)));
			glVertex3f((float) (- arrow3[i+1][0] * multiplier * cos(angle3) 
					   - arrow3[i+1][2] * multiplier * sin(angle3)),
					   (float) (arrow3[i+1][1] * multiplier + delta3),
					   (float) (arrow3[i+1][2] * multiplier * cos(angle3) -
					   arrow3[i+1][0] * multiplier * sin(angle3)));
		}

		last = arrow4Count - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f((float) (arrow4[i][0] * multiplier), 
					   (float) (arrow4[i][1] * multiplier + delta4), 
					   (float) (arrow4[i][2] * multiplier));
			glVertex3f((float) (arrow4[i+1][0] * multiplier), 
					   (float) (arrow4[i+1][1] * multiplier + delta4), 
					   (float) (arrow4[i+1][2] * multiplier));
		}

		last = perimeterCount - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(perimeter[i][0] * multiplier, 
					   perimeter[i][1] * multiplier, 
					   perimeter[i][2] * multiplier);
			glVertex3f(perimeter[i+1][0] * multiplier, 
					   perimeter[i+1][1] * multiplier, 
					   perimeter[i+1][2] * multiplier);
		}

	glEnd();

	view.endGL();
}


bool swissArmyLocator::isBounded() const
{ 
	return true;
}


MBoundingBox swissArmyLocator::boundingBox() const
{   
	// Get the size
	//
	MObject thisNode = thisMObject();
	MPlug plug(thisNode, aSize);
	MDistance sizeVal;
	plug.getValue(sizeVal);

	double multiplier = sizeVal.asCentimeters();
 
	MPoint corner1(-1.1, 0.0, -1.1);
	MPoint corner2(1.1, 0.0, 1.1);

	corner1 = corner1 * multiplier;
	corner2 = corner2 * multiplier;

	return MBoundingBox(corner1, corner2);
}


void* swissArmyLocator::creator()
{
	return new swissArmyLocator();
}


MStatus swissArmyLocator::initialize()
{ 
	MFnUnitAttribute unitFn;
	MFnNumericAttribute numericFn;
	MStatus			 stat;
	MString method("swissArmyLocator::initialize");
	MStatus s;
	int counter = 0;

	// aSize
	aSize = unitFn.create("size", "sz", MFnUnitAttribute::kDistance, 
						  0.0, &s); e;
	unitFn.setDefault(10.0);
	unitFn.setStorable(true);
	unitFn.setWritable(true);

	// aPoint
	aPointX = numericFn.create("pointX", "ptx", 
							   MFnNumericData::kDouble, 0.0, &s); e;
	aPointY = numericFn.create("pointY", "pty",
							   MFnNumericData::kDouble, 0.0, &s); e;
	aPointZ = numericFn.create("pointZ", "ptz",
							   MFnNumericData::kDouble, 0.0, &s); e;
	aPoint = numericFn.create("point", "pt",
							  aPointX,
							  aPointY,
							  aPointZ, &s); e;

	// aArrow1Angle
	aArrow1Angle = unitFn.create("arrow1Angle", "a1a", 
								 MFnUnitAttribute::kAngle, 0.0, &s); e;

	// aArrow2Direction
	aArrow2DirectionX = numericFn.create("arrow2DirectionX", "a2x", 
										 MFnNumericData::kDouble, 1.0, &s); e;
	aArrow2DirectionY = numericFn.create("arrow2DirectionY", "a2y",
										 MFnNumericData::kDouble, 0.0, &s); e;
	aArrow2DirectionZ = numericFn.create("arrow2DirectionZ", "a2z",
										 MFnNumericData::kDouble, 0.0, &s); e;
	aArrow2Direction = numericFn.create("arrow2Direction", "dir",
										aArrow2DirectionX,
										aArrow2DirectionY,
										aArrow2DirectionZ, &s); e;

	// aArrow3Angle
	aArrow3Angle = unitFn.create("arrow3Angle", "a3a", 
								 MFnUnitAttribute::kAngle, 0.0, &s); e;
	// aArrow4Distance
	aArrow4Distance = unitFn.create("arrow2Distance", "dis", 
									MFnUnitAttribute::kDistance, 0.0, &s); e;

	// aState;
	aState = numericFn.create("state", "s",
							  MFnNumericData::kLong, 0, &s); e;

	// aToggle;
	aToggle = numericFn.create("toggle", "t",
							   MFnNumericData::kBoolean, false, &s); e;

	s = addAttribute(aPoint); e;
	s = addAttribute(aArrow1Angle); e;
	s = addAttribute(aArrow2Direction); e;
	s = addAttribute(aArrow3Angle); e;
	s = addAttribute(aArrow4Distance); e;
	s = addAttribute(aState); e;
	s = addAttribute(aToggle); e;

	stat = addAttribute(aSize);
	if (!stat) {
		stat.perror("addAttribute");
		return stat;
	}
	
	MPxManipContainer::addToManipConnectTable(id);

	return MS::kSuccess;
}


MStatus initializePlugin(MObject obj)
{ 
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "6.0", "Any");

	status = plugin.registerNode("swissArmyLocator", 
								 swissArmyLocator::id, 
								 &swissArmyLocator::creator, 
								 &swissArmyLocator::initialize,
								 MPxNode::kLocatorNode);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

    status = plugin.registerNode("swissArmyLocatorManip", 
								 swissArmyLocatorManip::id, 
								 &swissArmyLocatorManip::creator, 
								 &swissArmyLocatorManip::initialize,
								 MPxNode::kManipContainer);
    if (!status) {
        status.perror("registerNode");
        return status;
    }

	return status;
}


MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode(swissArmyLocator::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	status = plugin.deregisterNode(swissArmyLocatorManip::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}

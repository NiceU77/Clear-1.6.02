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

//
//  This example plugin demonstrates how to implement a Maya File Translator.
//

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MIOStream.h> 
#include <maya/MFStream.h>
#include <string.h> 

class LepTranslator : public MPxFileTranslator {
public:
					LepTranslator () {};
	virtual			~LepTranslator () {};
	static void*	creator();

	MStatus			reader ( const MFileObject& file,
							 const MString& optionsString,
							 MPxFileTranslator::FileAccessMode mode);
	MStatus			writer ( const MFileObject& file,
							 const MString& optionsString,
							 MPxFileTranslator::FileAccessMode mode);
	bool			haveReadMethod () const;
	bool			haveWriteMethod () const;
	MString         defaultExtension () const;
	bool            canBeOpened() const;
	MFileKind		identifyFile (	const MFileObject& fileName,
									const char* buffer,
									short size) const;

private:
	void			getPosition( MObject& transform,
								 double& tx, double& ty, double& tz );
	static MString	magic;
};

void* LepTranslator::creator()
{
	return new LepTranslator();
}

// Initialize our magic "number"
MString LepTranslator::magic("<LEP>");

// An LEP file is an ascii whose first line contains the string <LEP>.
// The read does not support comments, and assumes that the each
// subsequent line of the file contains a valid MEL command that can
// be executed via the "executeCommand" method of the MGlobal class.
//
MStatus LepTranslator::reader ( const MFileObject& file,
								const MString& options,
								MPxFileTranslator::FileAccessMode mode)
{
	#if defined (OSMac_)
		char nameBuffer[MAXPATHLEN];
		strcpy (nameBuffer, file.fullName().asChar());
		const MString fname(nameBuffer);
	#else
		const MString fname = file.fullName();
	#endif	

	MStatus rval(MS::kSuccess);
	const int maxLineSize = 1024;
	char buf[maxLineSize];

	ifstream inputfile(fname.asChar(), ios::in);
	if (!inputfile) {
		// open failed
		cerr << fname << ": could not be opened for reading\n";
		return MS::kFailure;
	}

	if (!inputfile.getline (buf, maxLineSize)) {
		cerr << "file " << fname << " contained no lines ... aborting\n";
		return MS::kFailure;
	}

	if (0 != strncmp(buf, magic.asChar(), magic.length())) {
		cerr << "first line of file " << fname;
		cerr << " did not contain " << magic.asChar() << " ... aborting\n";
		return MS::kFailure;
	}

	while (inputfile.getline (buf, maxLineSize)) {
		MString cmdString;

		cmdString.set(buf);
		if (!MGlobal::executeCommand(cmdString))
			rval = MS::kFailure;
	}
	inputfile.close();

	return rval;
}

// Write out the contents of the DAG in LEP format.  This is by no
// means a full implementation.  It only writes out transforms for the
// nurbs primitives listed below, and ignores all other objects.  If
// anything other than the default Maya naming (primitiveName#) is used
// for the primitives, the write routine will not recognise them.

// The currently recognised primitives.
const char* primitiveStrings[] = {
	"nurbsSphere",
	"nurbsCone",
	"nurbsCylinder",
};
const unsigned numPrimitives = sizeof(primitiveStrings) / sizeof(char*);

// Corresponding commands to create the primitives
const char* primitiveCommands[] = {
	"sphere",
	"cone",
	"cylinder",
};

MStatus LepTranslator::writer ( const MFileObject& file,
								const MString& options,
								MPxFileTranslator::FileAccessMode mode)
{
	MStatus status;
	bool showPositions = false;
	unsigned int  i;
	const MString fname = file.fullName();

	ofstream newf(fname.asChar(), ios::out);
	if (!newf) {
		// open failed
		cerr << fname << ": could not be opened for reading\n";
		return MS::kFailure;
	}
	newf.setf(ios::unitbuf);

	if (options.length() > 0) {
		// Start parsing.
		MStringArray optionList;
		MStringArray theOption;
		options.split(';', optionList);	// break out all the options.
		
		for( i = 0; i < optionList.length(); ++i ){
			theOption.clear();
			optionList[i].split( '=', theOption );
			if( theOption[0] == MString("showPositions") &&
													theOption.length() > 1 ) {
				if( theOption[1].asInt() > 0 ){
					showPositions = true;
				}else{
					showPositions = false;
				}
			}
		}
	}

	// output our magic number
	newf << "<LEP>\n";

	MItDag dagIterator( MItDag::kBreadthFirst, MFn::kInvalid, &status);

	if ( !status) {
		status.perror ("Failure in DAG iterator setup");
		return MS::kFailure;
	}

	MSelectionList selection;
	MGlobal::getActiveSelectionList (selection);
	MItSelectionList selIterator (selection, MFn::kDagNode);

	bool done = false;

	while (true) {
		MObject currentNode;
		switch (mode)
		{
			case MPxFileTranslator::kSaveAccessMode:
			case MPxFileTranslator::kExportAccessMode:
				if (dagIterator.isDone ())
					done = true;
				else {
					currentNode = dagIterator.item ();
					dagIterator.next ();
				}
				break;
			case MPxFileTranslator::kExportActiveAccessMode:
				if (selIterator.isDone ())
					done = true;
				else {
					selIterator.getDependNode (currentNode);
					selIterator.next ();
				}
				break;
			default:
				cerr << "Unrecognized write mode: " << mode << endl;
				break;
		}
		if (done)
			break;

		MFnDagNode dagNode(currentNode, &status);
		if ( !status ) {
			status.perror ("Attaching MFnDagNode");
			continue;
		}

		MString nodeName   = dagNode.name();
		MFn::Type nodeType = currentNode.apiType();
		MStringArray primitives(primitiveStrings, numPrimitives);
		MStringArray commands(primitiveCommands, numPrimitives);

		if (nodeType == MFn::kTransform) {
			// We are only dealing with transforms
			// And actually, only the transforms with names that
			// match the primitive names specified above.
			for (i = 0; i < primitives.length(); ++i) {
				unsigned len = primitives[i].length();
				if (nodeName.length() < len)
					continue; // Can't possibly match
				if (primitives[i] == nodeName.substring(0, len - 1)) {
					// This is a node we support
					newf << commands[i] << " -n " << nodeName << endl;

					if (showPositions) {
						double tx, ty, tz;
						getPosition(currentNode, tx, ty, tz);
						newf << "move " << tx << " " << ty << " " << tz << endl;
					}
				}
			}
			    
		}
	}

	newf.close();
	return MS::kSuccess;
}

bool LepTranslator::haveReadMethod () const
{
	return true;
}

bool LepTranslator::haveWriteMethod () const
{
	return true;
}

// Whenever Maya needs to know the preferred extension of this file format,
// it calls this method. For example, if the user tries to save a file called
// "test" using the Save As dialog, Maya will call this method and actually
// save it as "test.lep". Note that the period should *not* be included in
// the extension.
MString LepTranslator::defaultExtension () const
{
	return "lep";
}

// This method tells Maya whether the translator can open and import files
// (returns true) or only import files (returns false)
bool LepTranslator::canBeOpened () const
{
	return true;
}

MPxFileTranslator::MFileKind LepTranslator::identifyFile (
										const MFileObject& fileName,
										const char* buffer,
										short size) const
{
	// Check the buffer for the "LEP" magic number, the
	// string "<LEP>\n"

	MFileKind rval = kNotMyFileType;

	if ((size >= (short)magic.length()) &&
		(0 == strncmp(buffer, magic.asChar(), magic.length()))) {

		/*
		cout << "LepTranslator::identifyFile\n";
		cout << "\tname: \"" << fileName.name() << "\"\n";
		cout << "\tpath: \"" << fileName.path() << "\"\n";
		cout << "\trawPath: \"" << fileName.rawPath() << "\"\n";
		cout << "\tfullName: \"" << fileName.fullName() << "\"\n";
		cout << "\tsize: " << size << endl;
		*/

		rval = kIsMyFileType;
	}
	return rval;
}

void LepTranslator::getPosition( MObject& transform,
								 double& tx, double& ty, double& tz )
{
	MObject attr;
	MFnDependencyNode dgNode(transform);

	tx = ty = tz = 0.0;

	// Get plugs to the x, y, and z position attributes of the transform
	attr = dgNode.attribute( MString("translateX"));
	MPlug txPlug( transform, attr );

	attr = dgNode.attribute( MString("translateY"));
	MPlug tyPlug( transform, attr );

	attr = dgNode.attribute( MString("translateZ"));
	MPlug tzPlug( transform, attr );

	// Extract the position data from this transform
	txPlug.getValue( tx );
	tzPlug.getValue( tz );
	tyPlug.getValue( ty );
}

MStatus initializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	// Register the translator with the system
	status =  plugin.registerFileTranslator( "Lep",
										"lepTranslator.rgb",
										LepTranslator::creator,
										"lepTranslatorOpts",
										"showPositions=1",
										true );
	if (!status) {
		status.perror("registerFileTranslator");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status =  plugin.deregisterFileTranslator( "Lep" );
	if (!status) {
		status.perror("deregisterFileTranslator");
		return status;
	}

	return status;
}

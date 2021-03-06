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

#include <maya/MGlobal.h>

#include "Platform.h"
#include "hlslFXShader.h"

#include "ResourceManager.h"

//
//
//
////////////////////////////////////////////////////////////////////////////////
hlslFXShader::hlslFXShader() :  m_valid(false), m_fShader(0), m_vShader(0), m_activePass(0), m_activeTechnique(0),
  m_stale(false), m_color(true), m_normal(true), m_tangent(true), m_binormal(true), m_texMask(0x1), m_error("") {

  
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
hlslFXShader::~hlslFXShader() {

  for (std::vector<passState*>::iterator it=m_stateList.begin(); it<m_stateList.end(); it++) {
    delete *it;
  }

  //assumes deleting 0 is OK
  deleteShaders();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::deleteShaders() {

  {
  for (std::vector<GLuint>::iterator it = m_vShader.begin(); it < m_vShader.end(); it++) {
    ResourceManager::destroyAsmProgram( *it);
  }
  }

  m_vShader.clear();

  {
  for (std::vector<GLuint>::iterator it = m_fShader.begin(); it < m_fShader.end(); it++) {
    ResourceManager::destroyAsmProgram( *it);
  }
  }

  m_fShader.clear();

}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::createFromFile( const char *filename) {
  m_error = ""; 
  IAshliFX *fx = new IAshliFX;
  fx->init();
  fx->setBinding( IAshliFX::DXSL);
  fx->setFX( filename);  //may need to do some path searching here
  m_stale = true;
  

  try {
    if (!fx->parse()) {
      //should store a string describing the failure for query
      m_error = "Unable to load FX file\n  ";
      m_error += fx->getError();
      delete fx;
      return false;
    }
  } catch(...){
    printf("Unexpected Error!!\n");
  };


  if ( fx->getNumTechniques()) {
  }
  else {
    //message about no valid first technique/incorrect number of passes
    delete fx;
    m_error = "FX file lacked a valid shader";
    return false;
  }
  
  //this will move to bind time
  //if (!buildShaders())
  //  return false;

  if (!parseParameters(fx))
    return false;

  //need to extract all shader data from the fx file here 
  //these are to hold the ASHLIFX data
  m_techniqueCount = fx->getNumTechniques();
  m_techniqueNames.clear();
  m_passCount.clear();
  m_techniqueOffset.clear();
  m_vertexShaders.clear();
  m_fragmentShaders.clear();
  m_orgVertexShaders.clear();
  m_orgFragmentShaders.clear();
  for (std::vector<passState*>::iterator it=m_stateList.begin(); it<m_stateList.end(); it++) {
    delete *it;
  }
  m_stateList.clear();
  int totalPasses =0;

  stateObserver *observer = new stateObserver;

  fx->attach( (IObserveFX*)observer);

  for (int ii=0; ii<m_techniqueCount; ii++) {
    int passCount = fx->getNumPasses(ii);
    m_techniqueOffset.push_back(totalPasses);
    ITechniqueFX tech;
    fx->getTechnique(ii, tech);
    m_techniqueNames.push_back(tech.getId());
    m_passCount.push_back(passCount);
    for (int jj=0; jj<passCount; jj++) {
      std::string vString;
      std::string fString;
      std::string orgVString;
      std::string orgFString;
      
      //allocate an ashli for this pass
      IAshli *ashli = new IAshli;
      ashli->setNative(IAshli::DXSL);
      ashli->setFlag( IAshli::UnpackedScalars, true);
      ashli->setFlag( IAshli::ContiguousMatrix, true);
      ashli->setFlag( IAshli::ContiguousArray, true);
      ashli->setFlag( IAshli::MultiPass, false);
      ashli->init( IAshli::Stream, IAshli::GL, IAshli::Stream);
      if ( !fx->isVertexNull( ii, jj)) {
        orgVString = fx->getVertexShader( ii, jj);
        ashli->addShaderItem( orgVString.c_str());
        ashli->addShaderInstance( ashli->addShader( IAshli::Vertex, fx->getVertexEntry( ii, jj)));
      }
      if ( !fx->isPixelNull( ii, jj)) {
        orgFString = fx->getPixelShader( ii, jj);
        ashli->addShaderItem( orgFString.c_str());
        ashli->addShaderInstance( ashli->addShader( IAshli::Pixel, fx->getPixelEntry( ii, jj)));
      }
      if ( ashli->invoke("")) {
        //compile succeeded 
        fx->setMetadata( ii, jj, ashli->getFormals());
        fx->setVertexAssembly( ii, jj, ashli->getVertexShader());
        fx->setPixelAssembly( ii, jj, ashli->getPixelShader());
        if (fx->getVertexAssembly( ii, jj))
          vString = fx->getVertexAssembly( ii, jj);
        if (fx->getPixelAssembly( ii, jj))
          fString = fx->getPixelAssembly( ii, jj);
      }
      else {
        //compile failed
        const char* error = ashli->getError();
        m_error = "Shader failed to compile\n";
        m_error += error;
        m_error += std::string("\n");
        // would like to print the offending text, but the parsing seems to difficult
      }

      m_vertexShaders.push_back(vString);
      m_fragmentShaders.push_back(fString);

      m_orgVertexShaders.push_back(orgVString);
      m_orgFragmentShaders.push_back(orgFString);

      //accumulate the state data
      m_stateList.push_back(new passState);
      observer->setPassMonitor(m_stateList.back());
      for (int kk=0; kk<fx->getNumStates( ii,jj); kk++) {
        fx->getStateItem( ii, jj, kk);
      }
      observer->finalizePassMonitor();
      delete ashli;
    }
    totalPasses += passCount;
  }
    
  fx->attach(NULL);
  delete observer;
  delete fx;

  m_valid = true;

  return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::createFromFX( IAshliFX *fx) {
  m_error = ""; 
  m_stale = true;
  m_valid = false;
  int maxVInstructions, maxVTemps, maxVParameters, maxALU, maxTexInst, maxTexIndirections,
    maxFInstructions, maxFTemps, maxFParameters; /* maxTextures, maxVAttribs, */

  if (fx->getNumTechniques()) {
  }
  else {
    //message about no valid first technique/incorrect number of passes
    m_error = "FX file lacked a valid shader";
    return false;
  }
  
  if (!parseParameters(fx))
    return false;

  //query for all the limits
  glGetProgramivARB( GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &maxVInstructions);
  glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &maxFInstructions);
  glGetProgramivARB( GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &maxVTemps);
  glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &maxFTemps);
  glGetProgramivARB( GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &maxVParameters);
  glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &maxFParameters);
  glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &maxALU);
  glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB, &maxTexInst);
  glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &maxTexIndirections);
/*
  maxFTemps += maxFTemps>>2 ;
  maxFInstructions += maxFInstructions>>2 ;
  maxALU += maxALU>>2 ;
*/
  //need to extract all shader data from the fx file here 
  //these are to hold the ASHLIFX data
  m_techniqueCount = fx->getNumTechniques();
  m_techniqueNames.clear();
  m_passCount.clear();
  m_techniqueOffset.clear();
  m_vertexShaders.clear();
  m_fragmentShaders.clear();
  m_orgVertexShaders.clear();
  m_orgFragmentShaders.clear();
  for (std::vector<passState*>::iterator it=m_stateList.begin(); it<m_stateList.end(); it++) {
    delete *it;
  }
  m_stateList.clear();
  int totalPasses =0;

  stateObserver *observer = new stateObserver;

  fx->attach( (IObserveFX*)observer);

  bool success = true;

  for (int ii=0; ii<m_techniqueCount; ii++) {
    int passCount = fx->getNumPasses(ii);
    m_techniqueOffset.push_back(totalPasses);
    ITechniqueFX tech;
    fx->getTechnique(ii, tech);
    m_techniqueNames.push_back(tech.getId());
    m_passCount.push_back(passCount);
    for (int jj=0; jj<passCount; jj++) {
      std::string vString;
      std::string fString;
      std::string orgVString;
      std::string orgFString;
      
      //allocate an ashli for this pass
      IAshli *ashli = new IAshli;
      ashli->setNative(IAshli::DXSL);
      ashli->setFlag( IAshli::UnpackedScalars, true);
      ashli->setFlag( IAshli::ContiguousMatrix, true);
      ashli->setFlag( IAshli::ContiguousArray, true);
      ashli->setFlag( IAshli::MultiPass, false);
      ashli->init( IAshli::Stream, IAshli::GL, IAshli::Stream);
      
      ashli->setResource( IAshli::Vertex, IAshli::InstructionSlots, maxVInstructions);
      ashli->setResource( IAshli::Vertex, IAshli::ALUInstructions, maxVInstructions);
      ashli->setResource( IAshli::Vertex, IAshli::FloatConstantRegisters, maxVParameters);
      ashli->setResource( IAshli::Vertex, IAshli::TemporaryRegisters, maxVTemps);
      ashli->setResource( IAshli::Pixel, IAshli::InstructionSlots, maxFInstructions);
      ashli->setResource( IAshli::Pixel, IAshli::ALUInstructions, maxALU);
      ashli->setResource( IAshli::Pixel, IAshli::TextureInstructions, maxTexInst);
      ashli->setResource( IAshli::Pixel, IAshli::TextureDependency, maxTexIndirections);
      ashli->setResource( IAshli::Pixel, IAshli::FloatConstantRegisters, maxFParameters);
      ashli->setResource( IAshli::Pixel, IAshli::TemporaryRegisters, maxFTemps);
      
      
      //ashli->setResource( IAshli::Vertex, IAshli::AllResources, 2048);
      //ashli->setResource( IAshli::Pixel, IAshli::AllResources, 2048);
      
      if ( !fx->isVertexNull( ii, jj)) {
        orgVString = fx->getVertexShader( ii, jj);
        ashli->addShaderItem( orgVString.c_str());
        ashli->addShaderInstance( ashli->addShader( IAshli::Vertex, fx->getVertexEntry( ii, jj)));
      }
      if ( !fx->isPixelNull( ii, jj)) {
        orgFString = fx->getPixelShader( ii, jj);
        ashli->addShaderItem( orgFString.c_str());
        ashli->addShaderInstance( ashli->addShader( IAshli::Pixel, fx->getPixelEntry( ii, jj)));
      }
      if ( ashli->invoke("")) {
        if ( ashli->getNumSegments() == 1 ) {
        //compile succeeded 
        fx->setMetadata( ii, jj, ashli->getFormals());
        fx->setVertexAssembly( ii, jj, ashli->getVertexShader());
        fx->setPixelAssembly( ii, jj, ashli->getPixelShader());
        if (fx->getVertexAssembly( ii, jj))
          vString = fx->getVertexAssembly( ii, jj);
        if (fx->getPixelAssembly( ii, jj))
          fString = fx->getPixelAssembly( ii, jj);
      }
      else {
        //compile failed
          success = false;
          m_error = "Shader exceeded available resources\n";
        }
      }
      else {
        //compile failed
        success = false;
        const char* error = ashli->getError();
        m_error = "Shader failed to compile\n";
        m_error += error;
        m_error += std::string("\n");
        // would like to print the offending text, but the parsing seems to difficult
      }

      m_vertexShaders.push_back(vString);
      m_fragmentShaders.push_back(fString);

      m_orgVertexShaders.push_back(orgVString);
      m_orgFragmentShaders.push_back(orgFString);

      //accumulate the state data
      m_stateList.push_back(new passState);
      observer->setPassMonitor(m_stateList.back());
      for (int kk=0; kk<fx->getNumStates( ii,jj); kk++) {
        fx->getStateItem( ii, jj, kk);
      }
      observer->finalizePassMonitor();
	  // Commented out as this causes a access violation.
      //delete ashli;
    }
    totalPasses += passCount;
  }
    
  fx->attach(NULL);
  delete observer;

  m_valid = success;

  return success;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
//new method, parsing parameters from AshliFX
bool hlslFXShader::parseParameters(const IAshliFX *fx) {

  int passCount =0, ii;
  
  for (ii=0; ii<fx->getNumTechniques(); ii++) {
    passCount = std::max<int>( fx->getNumPasses(ii), passCount);
  }

  //process the exposed shader parameters
  for (ii=0; ii<fx->getNumParameters(); ii++) {
    IParameterFX parm;
    fx->getParameter( "", ii, parm);
    const char *usage = parm.getUsage();
    const char *type = parm.getType();
    const char *name = parm.getId();
    const char *semantic = parm.getSemantic();
    const char *expression = parm.getExpression();
    if (strcmp( "const", usage)) {
      if (strncmp( "sampler", type, 7)) {
        // this is a regular uniform (not a sampler)

        //should verify the compatibility of the following pieces of data
        shader::DataType dt = parseUniformType( type);
        shader::Semantic sm = parseUniformSemantic( semantic);
        uniform u;
        u.type = dt;
        u.fReg.resize( passCount, -1);
        u.vReg.resize( passCount, -1);
        u.usage = sm;
        u.name = name;
        //parse the expression to extract a default value
        parseExpression( u, expression);

        //skip adding strings for now, maybe add them somewhere else later
        if (dt != shader::dtString)
        m_uniformList.push_back(u);
      }
      else {
        // this is a sampler uniform
        shader::SamplerType st = parseSamplerType( type);
        sampler s;
        s.texUnit.resize( passCount, -1);
        s.type = st;
        s.name = name;
        m_samplerList.push_back(s);
      }
    }
    else {
      //don't know what else we could encounter, might want to create warnings
    }
  }

  return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::DataType hlslFXShader::parseUniformType( const char *type) {
  if (!strcmp( "float", type)) {
    return shader::dtFloat;
  }
  else if (!strcmp( "float1", type)) {
    return shader::dtFloat;
  }
  else if (!strcmp( "float2", type)) {
    return shader::dtVec2;
  }
  else if (!strcmp( "float3", type)) {
    return shader::dtVec3;
  }
  else if (!strcmp( "float4", type)) {
    return shader::dtVec4;
  }
  else if (!strcmp( "int", type)) {
    return shader::dtInt;
  }
  else if (!strcmp( "int1", type)) {
    return shader::dtInt;
  }
  else if (!strcmp( "int2", type)) {
    return shader::dtIVec2;
  }
  else if (!strcmp( "int3", type)) {
    return shader::dtIVec3;
  }
  else if (!strcmp( "int4", type)) {
    return shader::dtIVec4;
  }
  else if (!strcmp( "bool", type)) {
    return shader::dtBool;
  }
  else if (!strcmp( "bool1", type)) {
    return shader::dtBVec2;
  }
  else if (!strcmp( "bool2", type)) {
    return shader::dtBVec2;
  }
  else if (!strcmp( "bool3", type)) {
    return shader::dtBVec3;
  }
  else if (!strcmp( "bool4", type)) {
    return shader::dtBVec4;
  }
  else if (!strcmp( "float2x2", type)) {
    return shader::dtMat2;
  }
  else if (!strcmp( "float3x3", type)) {
    return shader::dtMat3;
  }
  else if (!strcmp( "float4x4", type)) {
    return shader::dtMat4;
  }
  
  return shader::dtUnknown;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::Semantic hlslFXShader::parseUniformSemantic( const char *semantic) {

  if (semantic == NULL)
    return shader::smNone;

  //this would be more efficient as a map
  if (!strcasecmp( "World", semantic)) {
    return shader::smWorld;
  }
  else if (!strcasecmp( "View", semantic)) {
    return shader::smView;
  }
  else if (!strcasecmp( "Projection", semantic)) {
    return shader::smProjection;
  }
  else if (!strcasecmp( "WorldView", semantic)) {
    return shader::smWorldView;
  }
  else if (!strcasecmp( "ViewProjection", semantic)) {
    return shader::smViewProjection;
  }
  else if (!strcasecmp( "WorldViewProjection", semantic)) {
    return shader::smWorldViewProjection;
  }
  else if ( (!strcasecmp( "WorldI", semantic)) || (!strcasecmp( "WorldInverse", semantic))) {
    return shader::smWorldI;
  }
  else if ( (!strcasecmp( "ViewI", semantic)) || (!strcasecmp( "ViewInverse", semantic)) ) {
    return shader::smViewI;
  }
  else if ( (!strcasecmp( "ProjectionI", semantic)) || (!strcasecmp( "ProjectionInverse", semantic)) ) {
    return shader::smProjectionI;
  }
  else if ( (!strcasecmp( "WorldViewI", semantic)) || (!strcasecmp( "WorldViewInverse", semantic)) ) {
    return shader::smWorldViewI;
  }
  else if ( (!strcasecmp( "ViewProjectionI", semantic)) || (!strcasecmp( "ViewProjectionInverse", semantic)) ) {
    return shader::smViewProjectionI;
  }
  else if ( (!strcasecmp( "WorldViewProjectionI", semantic)) || (!strcasecmp( "WorldViewProjectionInverse", semantic)) ) {
    return shader::smWorldViewProjectionI;
  }
  else if ( (!strcasecmp( "WorldT", semantic)) || (!strcasecmp( "WorldTranspose", semantic)) ) {
    return shader::smWorldT;
  }
  else if ( (!strcasecmp( "ViewT", semantic)) || (!strcasecmp( "ViewTranspose", semantic)) ) {
    return shader::smViewT;
  }
  else if ( (!strcasecmp( "ProjectionT", semantic)) || (!strcasecmp( "ProjectionTranspose", semantic)) ) {
    return shader::smProjectionT;
  }
  else if ( (!strcasecmp( "WorldViewT", semantic)) || (!strcasecmp( "WorldViewTranspose", semantic)) ) {
    return shader::smWorldViewT;
  }
  else if ( (!strcasecmp( "ViewProjectionT", semantic)) || (!strcasecmp( "ViewProjectionTranspose", semantic)) ) {
    return shader::smViewProjectionT;
  }
  else if ( (!strcasecmp( "WorldViewProjectionT", semantic)) || (!strcasecmp( "WorldViewProjectionTranspose", semantic)) ) {
    return shader::smWorldViewProjectionT;
  }
  else if ( (!strcasecmp( "WorldIT", semantic)) || (!strcasecmp( "WorldInverseTranspose", semantic)) ) {
    return shader::smWorldIT;
  }
  else if ( (!strcasecmp( "ViewIT", semantic)) || (!strcasecmp( "ViewInverseTranspose", semantic)) ) {
    return shader::smViewIT;
  }
  else if ( (!strcasecmp( "ProjectionIT", semantic)) || (!strcasecmp( "ProjectionInverseTranspose", semantic)) ) {
    return shader::smProjectionIT;
  }
  else if ( (!strcasecmp( "WorldViewIT", semantic)) || (!strcasecmp( "WorldViewInverseTranspose", semantic)) ) {
    return shader::smWorldViewIT;
  }
  else if ( (!strcasecmp( "ViewProjectionIT", semantic)) || (!strcasecmp( "ViewProjectionInverseTranspose", semantic)) ) {
    return shader::smViewProjectionIT;
  }
  else if ( (!strcasecmp( "WorldViewProjectionIT", semantic)) || (!strcasecmp( "WorldViewProjectionInverseTranspose", semantic)) ) {
    return shader::smWorldViewProjectionIT;
  }
  else if (!strcasecmp( "ViewPosition", semantic)) {
    return smViewPosition;
  }
  else if (!strcasecmp( "Time", semantic)) {
    return smTime;
  }
  else if (!strcasecmp( "RenderTargetDimensions", semantic)) {
    return smViewportSize;
  }
  else if (!strcasecmp( "Ambient", semantic)) {
    return smAmbient;
  }
  else if (!strcasecmp( "Diffuse", semantic)) {
    return smDiffuse;
  }
  else if (!strcasecmp( "Emissive", semantic)) {
    return smEmissive;
  }
  else if (!strcasecmp( "Specular", semantic)) {
    return smSpecular;
  }
  else if (!strcasecmp( "Opacity", semantic)) {
    return smOpacity;
  }
  else if (!strcasecmp( "SpecularPower", semantic)) {
    return smSpecularPower;
  }
  else if (!strcasecmp( "Height", semantic)) {
    return smHeight;
  }
  else if (!strcasecmp( "Normal", semantic)) {
    return smNormal;
  }

  return shader::smUnknown;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::parseExpression( hlslFXShader::uniform &u, const char *exp) {
  //this function should call a real parser, until then only handle a subset of types

  for (int ii=0; ii<16; ii++)
    u.defaultVal[ii] = 0.0f;

  switch ( u.type) {
    case shader::dtInt:
      if ( 1 == sscanf( exp, " int ( %f )", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " int1 ( %f )", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " %f ", &u.defaultVal[0])) {
      }
      else {
        u.defaultVal[0] = 0.0f;
      }
      break;
    case shader::dtFloat:
      if ( 1 == sscanf( exp, " float ( %f )", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " float1 ( %f )", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " %f ", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " float ( %ff )", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " float1 ( %ff )", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " %ff ", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " { %f } ", &u.defaultVal[0])) {
      }
      else if ( 1 == sscanf( exp, " { %ff } ", &u.defaultVal[0])) {
      }
      else {
        u.defaultVal[0] = 0.0f;
      }
      break;
    case shader::dtVec2:
      if ( 2 == sscanf( exp, " float2 ( %f , %f )", &u.defaultVal[0], &u.defaultVal[1])) {
      }
      else if ( 2 == sscanf( exp, " { %f , %f }", &u.defaultVal[0], &u.defaultVal[1])) {
      }
      else if ( 2 == sscanf( exp, " float2 ( %ff , %ff )", &u.defaultVal[0], &u.defaultVal[1])) {
      }
      else if ( 2 == sscanf( exp, " { %ff , %ff }", &u.defaultVal[0], &u.defaultVal[1])) {
      }
      else {
        u.defaultVal[0] = 0.0f;
        u.defaultVal[1] = 0.0f;
      }
      break;
    case shader::dtVec3:
      if ( 3 == sscanf( exp, " float3 ( %f , %f , %f )", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2])) {
      }
      else if ( 3 == sscanf( exp, " { %f , %f , %f }", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2])) {
      }
      else if ( 3 == sscanf( exp, " float3 ( %ff , %ff , %ff )", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2])) {
      }
      else if ( 3 == sscanf( exp, " { %ff , %ff , %ff }", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2])) {
      }
      else {
        u.defaultVal[0] = 0.0f;
        u.defaultVal[1] = 0.0f;
        u.defaultVal[2] = 0.0f;
      }
      break;
    case shader::dtVec4:
      if ( 4 == sscanf( exp, " float4 ( %f , %f , %f , %f )", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2], &u.defaultVal[3])) {
      }
      else if ( 4 == sscanf( exp, " float4 ( %ff , %ff , %ff , %ff )", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2], &u.defaultVal[3])) {
      }
      else if ( 4 == sscanf( exp, " { %f , %f , %f , %f }", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2], &u.defaultVal[3])) {
      }
      else if ( 4 == sscanf( exp, " { %ff , %ff , %ff , %ff }", &u.defaultVal[0], &u.defaultVal[1], &u.defaultVal[2], &u.defaultVal[3])) {
      }
      else {
        u.defaultVal[0] = 0.0f;
        u.defaultVal[1] = 0.0f;
        u.defaultVal[2] = 0.0f;
        u.defaultVal[3] = 0.0f;
      }
      break;
    default:
      //make gcc happy
      break;
  };
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::SamplerType hlslFXShader::parseSamplerType( const char *type) {
  if (!strcmp( "sampler1D", type)) {
    return shader::st1D;
  }
  else if (!strcmp( "sampler2D", type)) {
    return shader::st2D;
  }
  else if (!strcmp( "sampler3D", type)) {
    return shader::st3D;
  }
  else if (!strcmp( "samplerCUBE", type)) {
    return shader::stCube;
  }

  //add sampler RECT stuff here

  return shader::stUnknown;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateHandles() {
  int offset = m_techniqueOffset[m_activeTechnique];

  for ( int jj=0; jj<m_passCount[m_activeTechnique]; jj++) {


    //now apply the information
    for (std::vector<uniform>::iterator it = m_uniformList.begin(); it < m_uniformList.end(); it++) {
      std::map< std::string, int>::const_iterator it2 = m_stateList[offset+jj]->m_vRegMap.find(it->name);
      if ( it2 != m_stateList[offset+jj]->m_vRegMap.end()) 
        it->vReg[jj] = it2->second;
      else
        it->vReg[jj] = -1;

      it2 = m_stateList[offset+jj]->m_fRegMap.find(it->name);
      if ( it2 != m_stateList[offset+jj]->m_fRegMap.end()) 
        it->fReg[jj] = it2->second;
      else
        it->fReg[jj] = -1;

      it->dirty = true;
    }

	  {
    for (std::vector<sampler>::iterator it = m_samplerList.begin(); it < m_samplerList.end(); it++) {
      std::map< std::string, int>::const_iterator it2 = m_stateList[offset+jj]->m_fRegMap.find(it->name);

      if ( it2 != m_stateList[offset+jj]->m_fRegMap.end()) 
        it->texUnit[jj] = it2->second;
      else {
        it->texUnit[jj] = -1;
      }

      //this should only happen once per compile, we never remap samplers
      it->dirty = true;
    }
	  }
/*
    //workaround for AshliFX bug    
    {
      
      for (std::vector<sampler>::iterator it = m_samplerList.begin(); it < m_samplerList.end(); it++) {
        const char *formals=m_ashliList[jj]->getFormals();
        formals = strstr( formals, "s ");
        int i0;
        char c0[64], c1[64], c2[64];
        if (formals) {
          while (true) {
            sscanf( formals, " s %d %s %s %s", &i0, c0, c1, c2);
            if ( c2 == it->name) {
              break;
            }
            formals++;
            formals = strstr( formals, "s ");
            if (formals == NULL) {
              i0 = -1;
              break;
            }
          }
        }

        it->texUnit[jj] = i0;

        //this should only happen once per compile, we never remap samplers
        it->dirty = true;
      }
    }
*/

    for (std::vector<attribute>::iterator it2 = m_attributeList.begin(); it2 < m_attributeList.end(); it2++) {
      //it->handle = glGetAttribLocation( m_program, it->name);
      //nothing to mark dirty
    }
  }
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::buildShaders() {

  //clean up any old shaders
  void deleteShaders();

  m_vShader.resize(m_passCount[m_activeTechnique], 0);
  m_fShader.resize(m_passCount[m_activeTechnique], 0);

  bool fail = false;

  int offset = m_techniqueOffset[m_activeTechnique];

  GL_CHECK;

  for (int ii=0; ii<m_passCount[m_activeTechnique]; ii++) {

    if ( m_vertexShaders[offset+ii].length() > 0) {
      //compile the vertex program
      const char *program = m_vertexShaders[offset+ii].c_str();
      //clear any the errors
      while ( glGetError() != GL_NO_ERROR) ;

      glGenProgramsARB( 1, &m_vShader[ii]);
      glBindProgramARB( GL_VERTEX_PROGRAM_ARB, m_vShader[ii]);
      glProgramStringARB( GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei) strlen(program), program);

      if (glGetError() != GL_NO_ERROR) {
        //program failed compile, get the info
        fail = true;
        const char* error = (const char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        m_error += "Vertex shader failed compile\n  ";
        m_error += error;
        m_error += "\n";
      }
    }
    if ( m_fragmentShaders[offset+ii].length() > 0) {
      //compile the fragment program
      const char *program = m_fragmentShaders[offset+ii].c_str();
      //clear any the errors
      while ( glGetError() != GL_NO_ERROR) ;

      glGenProgramsARB( 1, &m_fShader[ii]);
      glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, m_fShader[ii]);
      glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei) strlen(program), program);

      if (glGetError() != GL_NO_ERROR) {
        //program failed compile, get the info
        fail = true;
        const char* error = (const char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        m_error += "Fragment shader failed compile\n  ";
        m_error += error;
      }
    }
  }

  if (fail)
    return false;


  return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::valid() {
  return m_valid;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int hlslFXShader::passCount() {
  if (m_techniqueCount)
    return m_passCount[m_activeTechnique];
  return 0;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int hlslFXShader::techniqueCount() {
  return m_techniqueCount;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* hlslFXShader::techniqueName( int n) {
  if ( n<m_techniqueCount)
    return m_techniqueNames[n].c_str();

  return NULL;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::build() {

  //need to try to compile the shaders here if they don't exist yet

  if (m_stale) {
    m_stale = false;
    if (!buildShaders())
      return false;
    updateHandles();
  }

  return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::bind() {

  GL_CHECK;

  if (m_fShader[m_activePass]) {
    glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, m_fShader[m_activePass]);
    glEnable( GL_FRAGMENT_PROGRAM_ARB);
  }

  if (m_vShader[m_activePass]) {
    glBindProgramARB( GL_VERTEX_PROGRAM_ARB, m_vShader[m_activePass]);
    glEnable( GL_VERTEX_PROGRAM_ARB);
  }

  GL_CHECK;

  GLenum targets[] = { GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_1D, GL_TEXTURE_2D};

  //update dirty samplers
  for (std::vector<sampler>::iterator it2 = m_samplerList.begin(); it2<m_samplerList.end(); it2++) {
    if (it2->texUnit[m_activePass] >= 0) {
      glActiveTexture( GL_TEXTURE0_ARB + it2->texUnit[m_activePass]);
      glBindTexture( targets[it2->type], it2->texObject);
    }
  }
  glActiveTexture( GL_TEXTURE0_ARB);

  GL_CHECK;

  //set up the render states
  m_stateList[m_techniqueOffset[m_activeTechnique] + m_activePass]->setState();

  return;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::setShapeDependentState() {

  GL_CHECK;

  //update dirty values
  for (std::vector<uniform>::iterator it = m_uniformList.begin(); it<m_uniformList.end(); it++) {
    if ( it->dirty) {
      if (m_activePass == (m_passCount[m_activeTechnique]-1)) {
        //only mark it clean on the last pass
      it->dirty = false;
      }
      
      switch (it->type) {
        case shader::dtBool:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[0], it->fVal[0], it->fVal[0]);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[0], it->fVal[0], it->fVal[0]);
          break;
        case shader::dtBVec2:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[1], 0.0f, 0.0f);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[1], 0.0f, 0.0f);
          break;
        case shader::dtBVec3:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[1], it->fVal[2], 0.0f);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[1], it->fVal[2], 0.0f);
          break;
        case shader::dtBVec4:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal);
          break;
        case shader::dtInt:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[0], it->fVal[0], it->fVal[0]);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[0], it->fVal[0], it->fVal[0]);
          break;
        case shader::dtIVec2:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[1], 0.0f, 0.0f);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[1], 0.0f, 0.0f);
          break;
        case shader::dtIVec3:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[1], it->fVal[2], 0.0f);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[1], it->fVal[2], 0.0f);
          break;
        case shader::dtIVec4:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal);
          break;
        case shader::dtFloat:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[0], it->fVal[0], it->fVal[0]);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[0], it->fVal[0], it->fVal[0]);
          break;
        case shader::dtVec2:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[1], 0.0f, 0.0f);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[1], 0.0f, 0.0f);
          break;
        case shader::dtVec3:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[1], it->fVal[2], 0.0f);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[1], it->fVal[2], 0.0f);
          break;
        case shader::dtVec4:
          if ( it->vReg[m_activePass] >= 0) 
            glProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal);
          if ( it->fReg[m_activePass] >= 0) 
            glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal);
          break;
        case shader::dtMat2:
          if ( it->vReg[m_activePass] >= 0) {
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[2], 0.0f, 0.0f);
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass]+1, it->fVal[1], it->fVal[3], 0.0f, 0.0f);
          }
          if ( it->fReg[m_activePass] >= 0) {
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[2], 0.0f, 0.0f);
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass]+1, it->fVal[1], it->fVal[3], 0.0f, 0.0f);
          }
          break;
        case shader::dtMat3:
          if ( it->vReg[m_activePass] >= 0) {
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[3], it->fVal[6], 0.0f);
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass]+1, it->fVal[1], it->fVal[4], it->fVal[7], 0.0f);
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass]+2, it->fVal[2], it->fVal[5], it->fVal[8], 0.0f);
          }
          if ( it->fReg[m_activePass] >= 0) {
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[3], it->fVal[6], 0.0f);
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass]+1, it->fVal[1], it->fVal[4], it->fVal[7], 0.0f);
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass]+2, it->fVal[2], it->fVal[5], it->fVal[8], 0.0f);
          }
          break;
        case shader::dtMat4:
          if ( it->vReg[m_activePass] >= 0) {
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass], it->fVal[0], it->fVal[4], it->fVal[8], it->fVal[12]);
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass]+1, it->fVal[1], it->fVal[5], it->fVal[9], it->fVal[13]);
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass]+2, it->fVal[2], it->fVal[6], it->fVal[10], it->fVal[14]);
            glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, it->vReg[m_activePass]+3, it->fVal[3], it->fVal[7], it->fVal[11], it->fVal[15]);
          }
          if ( it->fReg[m_activePass] >= 0) {
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass], it->fVal[0], it->fVal[4], it->fVal[8], it->fVal[12]);
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass]+1, it->fVal[1], it->fVal[5], it->fVal[9], it->fVal[13]);
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass]+2, it->fVal[2], it->fVal[6], it->fVal[10], it->fVal[14]);
            glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, it->fReg[m_activePass]+3, it->fVal[3], it->fVal[7], it->fVal[11], it->fVal[15]);
          }
          break;
        default:
          //make gcc happy
          break;
      };
      GL_CHECK;
      
    }
  }
  return;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::unbind() {
  glDisable( GL_FRAGMENT_PROGRAM_ARB);
  glDisable( GL_VERTEX_PROGRAM_ARB);
  GL_CHECK;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::setTechnique( int t) {
  if ( t != m_activeTechnique) {
    m_activeTechnique = t;
    m_stale = true;
  }
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::setPass( int p) {
  m_activePass = p;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int hlslFXShader::uniformCount() {
  return (int)m_uniformList.size();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int hlslFXShader::samplerCount() {
  return (int)m_samplerList.size();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int hlslFXShader::attributeCount() {
  return (int)m_attributeList.size();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* hlslFXShader::uniformName(int i) {
  //should check for array bounds
  return m_uniformList[i].name.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::DataType hlslFXShader::uniformType(int i) {
  //should check for array bounds
  return m_uniformList[i].type;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::Semantic hlslFXShader::uniformSemantic(int i) {
  //might want to check array bounds
  return m_uniformList[i].usage;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
float* hlslFXShader::uniformDefault(int i) {
  //might want to check array bounds
  return m_uniformList[i].defaultVal;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* hlslFXShader::samplerName(int i) {
  //should check for array bounds
  return m_samplerList[i].name.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::SamplerType hlslFXShader::samplerType(int i) {
  //should check for array bounds
  return m_samplerList[i].type;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* hlslFXShader::attributeName(int i) {
  //should check for array bounds
  return m_attributeList[i].name.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::DataType hlslFXShader::attributeType(int i) {
  return m_attributeList[i].type;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformBool( int i, bool val) {
  m_uniformList[i].fVal[0] = val ? 1.0f : 0.0f;
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformInt( int i, int val) {
  m_uniformList[i].fVal[0] = (float)val;
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformFloat( int i, float val) {
  m_uniformList[i].fVal[0] = val;
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformBVec( int i, const bool* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = val[ii] ? 0.0f : 1.0f;
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformIVec( int i, const int* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = (float)val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformVec( int i, const float* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformMat( int i, const float* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateUniformMat( int i, const double* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = (float)val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void hlslFXShader::updateSampler( int i, unsigned int val) {
  m_samplerList[i].texObject = val;
  //m_samplerList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::usesColor() {
  return m_color;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::usesNormal() {
  return m_normal;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::usesTexCoord( int set) {
  return ((m_texMask>>set) & 0x1) == 1;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::usesTangent() {
  return m_tangent;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool hlslFXShader::usesBinormal() {
  return m_binormal;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int hlslFXShader::tangentSlot() {
  return m_tangentSlot;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int hlslFXShader::binormalSlot() {
  return m_binormalSlot;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* hlslFXShader::errorString() {
  return m_error.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* hlslFXShader::getVertexShader( int pass) {
  if (m_activeTechnique < m_techniqueCount) {
    if ( pass < m_passCount[m_activeTechnique]) {
      return m_orgVertexShaders[m_techniqueOffset[m_activeTechnique]+pass].c_str();
    }
  }

  return NULL;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* hlslFXShader::getPixelShader( int pass) {
  if (m_activeTechnique < m_techniqueCount) {
    if ( pass < m_passCount[m_activeTechnique]) {
      return m_orgFragmentShaders[m_techniqueOffset[m_activeTechnique]+pass].c_str();
    }
  }

  return NULL;
}


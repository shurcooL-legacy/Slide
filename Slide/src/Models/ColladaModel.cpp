#include "../Globals.h"

#include <cstdio>
#include <string>

// FCollada
#define NO_LIBXML
// disable deprecated warning
//#pragma warning( disable : 4996 )
#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUObject.h"

#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDMaterial.h"
#include "FUtils/FUUri.h"

#include "../Wm5/Wm5Math.h"
#include "../Wm5/Wm5Vector4.h"
#include "../Wm5/Wm5Matrix4.h"

#include "Opcode.h"

#include "../Arcball/trackball_bell.h"

#include "ModelLoader.h"
#include "ColladaModel.h"

ColladaModel::ColladaModel(std::string sFilename)
	: ModelLoader(),
	  m_pDocument(FCollada::NewDocument())
{
	//printf("ColladaModel(\"%s\") Ctor.\n", sFilename.c_str());

	double dStartTime = glfwGetTime();
	if (FCollada::LoadDocumentFromFile(m_pDocument, sFilename.c_str())) {
		//printf("Loaded Collada file successfully in %f secs.\n", glfwGetTime() - dStartTime);
	} else {
		printf("ERROR: Failed to load Collada file!\n");
		throw -1;
	}
}

ColladaModel::ColladaModel()
	: ModelLoader(),
	  m_pDocument(nullptr)
{
	//printf("ColladaModel(null) Ctor.\n");
}

ColladaModel::~ColladaModel()
{
	delete m_pDocument; m_pDocument = nullptr;

	//printf("ColladaModel() ~Dtor.\n");
}

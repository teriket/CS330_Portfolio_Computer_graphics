///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3 && filename)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	//load images from a file into openGL
	LoadScenetexture();
	
	DefineObjectMaterials();

	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	MakeDesk();
	MakeBackWall();
	MakeDeskStand();
	MakeMug();
	MakeBooks();
	MakeLamp();
	MakePenHolder();
}

//load textures from a .jpg into openGL
void SceneManager::LoadScenetexture() {
	
	CreateGLTexture("./Source/brick.jpg", "brick");
	CreateGLTexture("./Source/desk.jpg", "desk");
	CreateGLTexture("./Source/wood.jpg", "wood");
	CreateGLTexture("./Source/plastic.jpg", "plastic");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

//define the materials for objects in the scene.  This includes their ambient, diffuse, and specular lighting.
void SceneManager::DefineObjectMaterials() {
	const float matMult = 0.25;
	OBJECT_MATERIAL woodMat;
	woodMat.ambientColor = glm::vec3(0.38, 0.26, 0.1);
	woodMat.ambientStrength = 0.2f * matMult;
	woodMat.diffuseColor = glm::vec3(0.36, 0.24, 0.12);
	woodMat.specularColor = glm::vec3(0.12, 0.14, 0.08);
	woodMat.shininess = 0.3;
	woodMat.tag = "wood";
	m_objectMaterials.push_back(woodMat);

	OBJECT_MATERIAL plasticMat;
	plasticMat.ambientColor = glm::vec3(0.0005, 0.0005, 0.0005);
	plasticMat.ambientStrength = 0.3;
	plasticMat.diffuseColor = glm::vec3(0.05, 0.05, 0.06);
	plasticMat.specularColor = glm::vec3(0.06, 0.05, 0.05);
	plasticMat.shininess = 0.2;
	plasticMat.tag = "plastic";
	m_objectMaterials.push_back(plasticMat);

	OBJECT_MATERIAL rubberMat;
	rubberMat.ambientColor = glm::vec3(0.92, 0.24, 0.90);
	rubberMat.ambientStrength = 0.3f * matMult;
	rubberMat.diffuseColor = glm::vec3(0.93, 0.28, 0.92);
	rubberMat.specularColor = glm::vec3(0.94, 0.30, 0.93);
	rubberMat.shininess = 0;
	rubberMat.tag = "rubber";
	m_objectMaterials.push_back(rubberMat);

	OBJECT_MATERIAL glassMat;
	glassMat.ambientColor = glm::vec3(0.7, 0.7, 0.7);
	glassMat.ambientStrength = 0.1f * matMult;
	glassMat.diffuseColor = glm::vec3(0.84, 0.84, 0.84);
	glassMat.specularColor = glm::vec3(0.92, 0.92, 0.92);
	glassMat.shininess = 32;
	glassMat.tag = "glass";
	m_objectMaterials.push_back(glassMat);

	OBJECT_MATERIAL brickMat;
	brickMat.ambientColor = glm::vec3(0.8, 0.8, 0.8);
	brickMat.ambientStrength = 0.2f * matMult;
	brickMat.diffuseColor = glm::vec3(0.84, 0.84, 0.84);
	brickMat.specularColor = glm::vec3(0.92, 0.92, 0.92);
	brickMat.shininess = 0.1f;
	brickMat.tag = "brick";
	m_objectMaterials.push_back(brickMat);

	OBJECT_MATERIAL paperMat;
	paperMat.ambientColor = glm::vec3(0.8, 0.8, 0.8);
	paperMat.ambientStrength = 0.3f * matMult;
	paperMat.diffuseColor = glm::vec3(0.84, 0.84, 0.84);
	paperMat.specularColor = glm::vec3(0.92, 0.92, 0.92);
	paperMat.shininess = 0.1f;
	paperMat.tag = "paper";
	m_objectMaterials.push_back(paperMat);

	OBJECT_MATERIAL topBookCoverMat;
	topBookCoverMat.ambientColor = glm::vec3(0.3, 0.3, 0.3);
	topBookCoverMat.ambientStrength = 0.5f * matMult;
	topBookCoverMat.diffuseColor = glm::vec3(0, 0.3, 0.3);
	topBookCoverMat.specularColor = glm::vec3(0.3, 0.3, 0.3);
	topBookCoverMat.shininess = 0.4f;
	topBookCoverMat.tag = "top_cover";
	m_objectMaterials.push_back(topBookCoverMat);

	OBJECT_MATERIAL bottomBookCoverMat;
	bottomBookCoverMat.ambientColor = glm::vec3(0.84, 0.726, 0.012);
	bottomBookCoverMat.ambientStrength = 0.5f * matMult;
	bottomBookCoverMat.diffuseColor = glm::vec3(0.89, 0.73, 0.02);
	bottomBookCoverMat.specularColor = glm::vec3(0.895, 0.73, 0.03);
	bottomBookCoverMat.shininess = 0.4f;
	bottomBookCoverMat.tag = "bottom_cover";
	m_objectMaterials.push_back(bottomBookCoverMat);
}

//generates point(s) of light in the scene with a specific vertex, color, and intensity
void SceneManager::SetupSceneLights() {
	//create a white light in the middle of the objects in the scene
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 6.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.10f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	//create an orange light in the back of the scene
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 1.0f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.08f, 0.08f, 0.113f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.568f, 0.388f, 0.133f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.588f, 0.408f, 0.153f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 20.1f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 1.01f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}

void SceneManager::MakeDesk() {
	//declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/

/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.                       ***/
/******************************************************************/

// THE BASE PLANE, REPRESENTING THE DESK
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30, 1, 10);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0, 0, 0);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("desk");
	SetTextureUVScale(2, 2);
	SetShaderMaterial("wood");

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::MakeBackWall() {
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	//declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// THE BACKDROP PLANE, REPRESENTING THE BACK WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30, 1, 10);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0, 10, -10);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("brick");
	SetTextureUVScale(7, 3);
	SetShaderMaterial("brick");

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::MakeDeskStand() {
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	//declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// TOP OF DESK STAND
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14, 1, 4);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0, 2, -6);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("desk");
	SetTextureUVScale(1, 1);
	SetShaderMaterial("wood");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	// RIGHT LEG OF DESK STAND
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1, 2, 4);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.5, 1, -6);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("desk");
	SetTextureUVScale(1, 1);
	SetShaderMaterial("wood");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	// LEFT LEG OF DESK STAND
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1, 2, 4);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5, 1, -6);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("desk");
	SetTextureUVScale(1, 1);
	SetShaderMaterial("wood");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::MakeMug() {
	//declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	// TOP OF MUG HANDLE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.125, 0.75, 0.125);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 90;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5, 4.1, -5);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("plastic");
	SetShaderMaterial("glass");

	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	// BOTTOM OF MUG HANDLE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.125, 0.75, 0.125);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 90;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.25, 3.1, -5);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	// OUTER RIM OF MUG HANDLE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.125, 1.3, 0.125);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 165.86;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5, 4.2, -5);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.                       ***/
/******************************************************************/

// MUG BODY
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1, 2, 1);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 180;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0, 4.5, -5);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/
}

void SceneManager::MakeBooks() {
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	//declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// bottom book
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3, 1, 4);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 20;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.5, 0.5, 0);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("paper");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();
	
	/***********************************************************************************************************
	*************************************************************************************************************
	************************************************************************************************************/

	// bottom book cover
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5, 0.5, 2);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 20;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.5, 1.001, 0);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("bottom_cover");

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();

	/***********************************************************************************************************
	*************************************************************************************************************
	************************************************************************************************************/

	// bottom book left face
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5, 1, 2);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 20;
	ZrotationDegrees = 90;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.93, 0.5, 0.495);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("bottom_cover");

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();

	/***********************************************************************************************************
*************************************************************************************************************
************************************************************************************************************/

// top book
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.6, 1, 3.467);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = -20;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.5, 1.5, 0);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("paper");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/***********************************************************************************************************
	*************************************************************************************************************
	************************************************************************************************************/

	// top book cover
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.35, 0.5, 1.75);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = -20;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.5, 2.001, 0);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("top_cover");

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();

	/***********************************************************************************************************
	*************************************************************************************************************
	************************************************************************************************************/

	// top book left face
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5, 1, 1.75);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = -20;
	ZrotationDegrees = 90;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.73, 1.5, -0.535);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("top_cover");

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();
	/***********************************************************************************************************
*************************************************************************************************************
************************************************************************************************************/

	// top book bottom cover
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.35, 0.5, 1.75);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = -20;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.5, 0.999, 0);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("top_cover");

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::MakeLamp() {
	//declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	const float offset[] = {4, 0, -1};

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// Base of the lamp
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5, 0.35, 1.5);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.65 + offset[0], 0 + offset[1], 0 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// Top arm of lamp
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25, 3.75, 0.25);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 60;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1 + offset[0], 3.3 + offset[1], 0 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.08, 0.08, 0.08, 1);
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// Bottom arm of lamp
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25, 4, 0.25);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = -60;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1 + offset[0], 1.3 + offset[1], 0 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// Head of lamp
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.25, 2, 1.25);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -15;
	YrotationDegrees = 0;
	ZrotationDegrees = -30;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1 + offset[0], 3 + offset[1], 0.25 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawConeMesh();

}

void SceneManager::MakePenHolder() {
	//declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	const float offset[] = { -3, 3.5, -6 };

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// Front Face
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5, 2, .25);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0 + offset[0], 0 + offset[1], 0.6 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(0.25, 0.25);
	SetShaderMaterial("wood");
	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// Right Face
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25, 2, 1.4);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.6 + offset[0], 0 + offset[1], 0 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(0.25, 0.25);
	SetShaderMaterial("wood");
	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// Left Face
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25, 2, 1.4);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.6 + offset[0], 0 + offset[1], 0 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(0.25, 0.25);
	SetShaderMaterial("wood");
	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// back Face
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5, 2, .25);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 0;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0 + offset[0], 0 + offset[1], -0.6 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(0.25, 0.25);
	SetShaderMaterial("wood");
	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// pencil Leaning Left
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.125, 3, .125);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 15;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.1 + offset[0], -0.5 + offset[1], 0 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("plastic");
	SetShaderMaterial("wood");
	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// pencil Leaning Forward Right
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.125, 3, .125);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 20;
	YrotationDegrees = 0;
	ZrotationDegrees = -10;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.1 + offset[0], -0.5 + offset[1], -0.5 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderMaterial("wood");
	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// pencil Leaning Left Eraser
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12, 0.2, .12);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0;
	YrotationDegrees = 0;
	ZrotationDegrees = 15;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.66 + offset[0], 2.35 + offset[1], 0 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("rubber");
	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	/********************************************************************************************************
	********************************************************************************************************
	********************************************************************************************************/

	// pencil Leaning Forward Right Eraser
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12, 0.2, .12);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 20;
	YrotationDegrees = 0;
	ZrotationDegrees = -10;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.61 + offset[0], 2.21 + offset[1], 0.49 + offset[2]);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("rubber");
	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();
}
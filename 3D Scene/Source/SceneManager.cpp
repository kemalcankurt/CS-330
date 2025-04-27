///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
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
	std::cout << "CreateGLTexture:" << std::endl;

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
		if (colorChannels == 3)
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}


/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/wood_floor.jpg",
		"floor");

	bReturn = CreateGLTexture(
		"textures/guitar_wood.jpg",
		"guitar_wood");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/


	// defining the main surface material here
	OBJECT_MATERIAL wood_material;
	wood_material.tag = "wood";
	wood_material.diffuseColor = glm::vec3(1.0f, 0.5f, 0.5f);
	wood_material.specularColor = glm::vec3(0.8f, 0.9f, 0.8f);
	wood_material.shininess = 64.0f;
	m_objectMaterials.push_back(wood_material);

	// material for mug
	OBJECT_MATERIAL porcelain_material;
	porcelain_material.tag = "porcelain";
	porcelain_material.diffuseColor = glm::vec3(1.0f, 0.5f, 0.5f);
	porcelain_material.specularColor = glm::vec3(0.8f, 0.9f, 0.8f);
	porcelain_material.shininess = 64.0f;
	m_objectMaterials.push_back(porcelain_material);

	// material for guitar body
	OBJECT_MATERIAL guitar_material;
	guitar_material.tag = "guitar_wood";
	guitar_material.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f); 
	guitar_material.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	guitar_material.shininess = 16.0f;
	m_objectMaterials.push_back(guitar_material);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// === Enable Lighting ===
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// === Directional Light: soft sun style ===
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.3f, -1.0f, -0.3f));
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.10f, 0.08f, 0.07f));
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.6f, 0.5f, 0.4f));
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.2f, 0.2f, 0.2f));
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// === Point Light 0: Mug glow ===
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(1.0f, 1.5f, 1.8f));
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.12f, 0.10f, 0.08f));
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(1.0f, 0.85f, 0.7f));
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.5f, 0.4f, 0.3f));
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.03f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.003f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// === Point Light 1: Back warm glow (guitar accent) ===
	m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(-5.0f, 2.0f, 0.0f));
	m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.05f, 0.04f, 0.03f));
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(0.8f, 0.6f, 0.3f));
	m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(0.4f, 0.3f, 0.2f));
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.05f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.01f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// === Point Light 2: Notebook glow ===
	m_pShaderManager->setVec3Value("pointLights[2].position", glm::vec3(2.5f, 1.2f, 3.2f));
	m_pShaderManager->setVec3Value("pointLights[2].ambient", glm::vec3(0.04f, 0.05f, 0.08f));
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", glm::vec3(0.4f, 0.5f, 1.0f));
	m_pShaderManager->setVec3Value("pointLights[2].specular", glm::vec3(0.4f, 0.5f, 1.0f));
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.05f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.02f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	// === Spot Light: Focused on mug ===
	m_pShaderManager->setVec3Value("spotLight.position", glm::vec3(0.5f, 3.0f, 2.5f));
	m_pShaderManager->setVec3Value("spotLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(15.0f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(22.0f)));
	m_pShaderManager->setVec3Value("spotLight.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
	m_pShaderManager->setVec3Value("spotLight.diffuse", glm::vec3(1.0f, 0.9f, 0.75f));
	m_pShaderManager->setVec3Value("spotLight.specular", glm::vec3(0.6f, 0.5f, 0.4f));
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.05f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.01f);
	m_pShaderManager->setBoolValue("spotLight.bActive", true);


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
	LoadSceneTextures();

	DefineObjectMaterials();

	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	m_basicMeshes->LoadTaperedCylinderMesh(); // Mug body
	m_basicMeshes->LoadTorusMesh();           // Mug handle
	m_basicMeshes->LoadCylinderMesh();        // Coffee surface
	m_basicMeshes->LoadSphereMesh();		  // For steam puffs
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.87f, 0.72f, 0.53f, 1.0f); // light brown table tone

	SetShaderTexture("floor");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// ========== Mug Body ==========
	glm::vec3 scaleBody = glm::vec3(1.2f, 1.2f, 1.2f); 
	XrotationDegrees = 180.0f;
	glm::vec3 positionBody = glm::vec3(-0.2f, 1.5f, 1.5f);

	SetTransformations(scaleBody, XrotationDegrees, 0.0f, 0.0f, positionBody);
	SetShaderColor(1.0f, 0.6f, 0.0f, 1.0f);
	SetShaderMaterial("porcelain");
	m_basicMeshes->DrawTaperedCylinderMesh();


	// ========== Mug Handle ==========
	glm::vec3 scaleHandle = glm::vec3(-0.4f, 0.4f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -30.0f;
	ZrotationDegrees = 0.0f;
	glm::vec3 positionHandle = glm::vec3(1.0f, 1.3f, 2.1f);

	SetTransformations(scaleHandle, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionHandle);
	SetShaderColor(1.0f, 0.6f, 0.0f, 1.0f);
	SetShaderMaterial("porcelain");
	m_basicMeshes->DrawTorusMesh();


	// ========== Coffee Surface ==========
	glm::vec3 scaleCoffee = glm::vec3(1.0f, 0.04f, 1.0f); 
	glm::vec3 positionCoffee = glm::vec3(-0.3f, 1.46f, 1.5f);

	SetTransformations(scaleCoffee, 0.0f, 0.0f, 0.0f, positionCoffee);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); 
	m_basicMeshes->DrawCylinderMesh();


	// ========== Steam ==========
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.4f);
	glm::vec3 scaleSteam = glm::vec3(0.1f, 0.1f, 0.1f);

	std::vector<glm::vec3> steamPositions = {
		glm::vec3(0.0f, 1.8f, 1.5f),
		glm::vec3(0.05f, 2.0f, 1.53f),
		glm::vec3(-0.03f, 2.25f, 1.48f)
	};

	for (const auto& pos : steamPositions) {
		SetTransformations(scaleSteam, 0.0f, 0.0f, 0.0f, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// === Guitar Body: Lower Bout ===
	glm::vec3 scaleGuitarLower = glm::vec3(2.8f, 0.3f, 1.7f);
	glm::vec3 posGuitarLower = glm::vec3(-6.2f, 0.2f, 2.5f);
	SetTransformations(scaleGuitarLower, 0.0f, -45.0f, 0.0f, posGuitarLower);
	SetShaderMaterial("guitar_wood");
	SetShaderTexture("guitar_wood");
	m_basicMeshes->DrawSphereMesh();

	// === Guitar Body: Upper Bout ===
	glm::vec3 scaleGuitarUpper = glm::vec3(2.2f, 0.3f, 1.2f);
	glm::vec3 posGuitarUpper = glm::vec3(-4.6f, 0.2f, 1.9f);
	SetTransformations(scaleGuitarUpper, 0.0f, -45.0f, 0.0f, posGuitarUpper);
	SetShaderMaterial("guitar_wood");
	SetShaderTexture("guitar_wood");
	m_basicMeshes->DrawSphereMesh();

	// === Guitar Neck ===
	glm::vec3 scaleNeck = glm::vec3(0.3f, 0.1f, 8.2f);
	glm::vec3 posNeck = glm::vec3(-3.7f, 0.3f, 1.8f);
	SetTransformations(scaleNeck, 0.0f, -60.0f, 0.0f, posNeck);
	SetShaderColor(0.3f, 0.15f, 0.05f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// === Guitar Sound Hole ===
	glm::vec3 scaleHole = glm::vec3(0.5f, 0.04f, 0.5f);
	glm::vec3 posHole = glm::vec3(-4.5f, 0.55f, 2.0f);
	SetTransformations(scaleHole, 10.0f, 75.0f, 0.0f, posHole);
	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// === Guitar Binding Edge ===
	glm::vec3 scaleBinding = glm::vec3(2.82f, 0.01f, 1.72f);
	glm::vec3 posBinding = glm::vec3(-6.2f, 0.35f, 2.5f);
	SetTransformations(scaleBinding, 0.0f, -45.0f, 0.0f, posBinding);
	SetShaderColor(1.0f, 0.9f, 0.7f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// === Guitar Strings ===
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f); 
	glm::vec3 scaleString = glm::vec3(0.02f, 0.02f, 3.7f);  

	for (int i = 0; i < 3; ++i)
	{
		float yOffset = 0.38f + i * 0.03f;  
		glm::vec3 posString = glm::vec3(-3.85f + (i * 0.2f), 0.35f, 1.8f); 

		SetTransformations(scaleString, 0.0f, -60.0f, 0.0f, posString);
		m_basicMeshes->DrawCylinderMesh();
	}

	// ========== Notebook ==========
	glm::vec3 scaleNotebook = glm::vec3(2.0f, 0.1f, 3.0f);
	glm::vec3 posNotebook = glm::vec3(1.8f, 0.05f, 3.2f);
	SetTransformations(scaleNotebook, 0.0f, -20.0f, 0.0f, posNotebook);
	SetShaderColor(0.1f, 0.4f, 0.8f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ========== Pens ==========
	glm::vec3 scalePen = glm::vec3(0.1f, 0.1f, 0.55f);

	// Pen 1
	SetTransformations(scalePen, 10.0f, 15.0f, 15.0f, glm::vec3(1.7f, 0.15f, 3.4f));
	SetShaderColor(0.5f, 0.1f, 0.1f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Pen 2
	SetTransformations(scalePen, 10.0f, -30.0f, -25.0f, glm::vec3(2.3f, 0.15f, 3.6f));
	SetShaderColor(0.3f, 0.7f, 1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
}

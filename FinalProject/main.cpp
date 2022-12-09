/*
 * Final Project
 * Eric Slutz
 */

#include <iostream> // cout, cerr
#include <cstdlib> // EXIT_FAILURE
#include <GL/glew.h> // GLEW library
#include <GLFW/glfw3.h> // GLFW library
#include <numbers> // pi
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // Image loading Utility functions

 // GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h" // Camera class

using namespace std; // Standard namespace

/* Shader program Macro */
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    // Macro for window title
    const char* const WINDOW_TITLE = "Final Project - Desk with items";

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao; // Handle for the vertex array object
        GLuint vbos[2]; // Handles for the vertex buffer objects
        GLuint nIndices; // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Mesh data
    GLMesh gCubeMesh;
    GLMesh gCylinderMesh;
    GLMesh gPlaneMesh;
    GLMesh gPyramidMesh;
    GLMesh gSphereMesh;
    GLMesh gWedgeMesh;
    // Textures
    GLuint gDeskTextureId;
    GLuint gMeshFabricTextureId;
    GLuint gRubberBaseTextureId;
    GLuint gMousePadTextureId;
    GLuint gInfinityCubeTextureId;
    // Shader programs
    GLuint gProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 3.0f, 18.0f));
    
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Light color, position and scale
    glm::vec3 gLightColor(0.639f, 0.592f, 0.512f); // White
    glm::vec3 gLightPosition(12.0f, 2.0f, 5.0f);
    glm::vec3 gLightScale(0.3f);

    // Light 2 color, position and scale
    glm::vec3 gLightColor2(0.639f, 0.592f, 0.512f); // White
    glm::vec3 gLightPosition2(-12.0f, 5.0f, 5.0f);
    glm::vec3 gLightScale2(0.3f);

    // Lamp animation
    bool gIsLampOrbiting = false;

    // View mode
    bool gOrthoView = false;
}

/*
 * User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateCubeMesh(GLMesh& mesh, float frontHeight = 1, float backHeight = 1);
void UCreateCylinderMesh(GLMesh& mesh);
void UCreatePlaneMesh(GLMesh& mesh);
void UCreatePyramidMesh(GLMesh& mesh);
void UCreateSphereMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code */
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
    layout(location = 1) in vec3 normal; // Normal data from Vertex Attrib Pointer 1
    layout(location = 2) in vec2 textureCoordinate; // Texture data from Vertex Attrib Pointer 2

    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec2 vertexTextureCoordinate; // For outgoing texture coordinate

    // Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices to clip coordinates
        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
        vertexNormal = mat3(transpose(inverse(model))) * normal; // Gets normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate; // Gets texture coordinate
    }
);

/* Fragment Shader Source Code */
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec3 vertexNormal; // For incoming normals
    in vec2 vertexTextureCoordinate; // For incoming texture coordinate

    out vec4 fragmentColor; // For outgoing cube color to the GPU

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 lightColor;
    uniform vec3 lightColor2;
    uniform vec3 lightPos;
    uniform vec3 lightPos2;
    uniform vec3 viewPosition;
    uniform vec3 viewPosition2;
    uniform sampler2D uTexture; // Useful when working with multiple textures
    uniform vec2 uvScale;

    void main()
    {
        /* Phong lighting model calculations to generate ambient, diffuse, and specular components */

        // LAMP 1: Calculate ambient lighting
        float ambientStrength = 0.8f; // Set ambient or global lighting strength 80%
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        // LAMP 2: Calculate ambient lighting
        float ambientStrength2 = 0.7f; // Set ambient or global lighting strength 70%
        vec3 ambient2 = ambientStrength2 * lightColor2; // Generate ambient light color

        // LAMP 1: Calculate diffuse lighting
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        // LAMP 2: Calculate diffuse lighting
        vec3 norm2 = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection2 = normalize(lightPos2 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact2 = max(dot(norm2, lightDirection2), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse2 = impact2 * lightColor2; // Generate diffuse light color

        // LAMP 1: Calculate specular lighting
        float specularIntensity = 0.1f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

        // LAMP 2: Calculate specular lighting
        float specularIntensity2 = 0.1f; // Set specular light strength
        float highlightSize2 = 16.0f; // Set specular highlight size
        vec3 viewDir2 = normalize(viewPosition2 - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir2 = reflect(-lightDirection2, norm2);// Calculate reflection vector

        // LAMP 1: Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // LAMP 2: Calculate specular component
        float specularComponent2 = pow(max(dot(viewDir2, reflectDir2), 0.0), highlightSize2);
        vec3 specular2 = specularIntensity2 * specularComponent2 * lightColor2;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate phong result
        vec3 phong = (ambient + ambient2 + diffuse + diffuse2 + specular + specular2) * textureColor.xyz;

        // Send lighting results to GPU
        fragmentColor = vec4(phong, 1.0);
    }
);

/* Lamp Shader Source Code */
const GLchar* lampVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0

    // Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    }
    );

    /* Lamp Fragment Shader Source Code */
    const GLchar* lampFragmentShaderSource = GLSL(440,
        out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

    void main()
    {
        fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    }
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
    {
        return EXIT_FAILURE;
    }

    // Create the scene meshes
    // -----------------------
    UCreateCubeMesh(gCubeMesh);
    UCreateCubeMesh(gWedgeMesh, 0.4f, 1.0f);
    UCreateCylinderMesh(gCylinderMesh);
    UCreatePlaneMesh(gPlaneMesh);
    UCreateSphereMesh(gSphereMesh);

    // Create the shader programs
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
    {
        cout << "Failed to create shader" << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
    {
        cout << "Failed to create lamp shader" << endl;
        return EXIT_FAILURE;
    }

    // Load desk texture
    const char* texFilename = "../textures/desk.png";
    if (!UCreateTexture(texFilename, gDeskTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    glUseProgram(gProgramId); // tell opengl texture unit sample belongs to
    glUniform1i(glGetUniformLocation(gProgramId, "deskTexture"), 0); // Set the texture as texture unit 0

    // Load mesh fabric texture
    texFilename = "../textures/black_mesh.png";
    if (!UCreateTexture(texFilename, gMeshFabricTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    glUseProgram(gProgramId); // tell opengl texture unit sample belongs to
    glUniform1i(glGetUniformLocation(gProgramId, "meshFabricTexture"), 0); // Set the texture as texture unit 0

    // Load black rubber texture
    texFilename = "../textures/black_rubber.png";
    if (!UCreateTexture(texFilename, gRubberBaseTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    glUseProgram(gProgramId); // tell opengl texture unit sample belongs to
    glUniform1i(glGetUniformLocation(gProgramId, "rubberBaseTexture"), 0); // Set the texture as texture unit 0

    // Load mouse pad texture
    texFilename = "../textures/mouse_pad.png";
    if (!UCreateTexture(texFilename, gMousePadTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    glUseProgram(gProgramId); // tell opengl texture unit sample belongs to
    glUniform1i(glGetUniformLocation(gProgramId, "mousePadTexture"), 0); // Set the texture as texture unit 0

    // Load mouse pad texture
    texFilename = "../textures/infinity_cube.png";
    if (!UCreateTexture(texFilename, gInfinityCubeTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    glUseProgram(gProgramId); // tell opengl texture unit sample belongs to
    glUniform1i(glGetUniformLocation(gProgramId, "infinityCubeTexture"), 0); // Set the texture as texture unit 0

    // Sets the background color of the window (it will be implicitely used by glClear)
    glClearColor(0.412f, 0.412f, 0.412f, 1.0f);

    // Render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // Per-frame timing
        // ----------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // Input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gCubeMesh);
    UDestroyMesh(gCylinderMesh);
    UDestroyMesh(gPlaneMesh);
    UDestroyMesh(gSphereMesh);
    UDestroyMesh(gWedgeMesh);
    

    // Release texture
    UDestroyTexture(gDeskTextureId);
    UDestroyTexture(gMeshFabricTextureId);
    UDestroyTexture(gRubberBaseTextureId);
    UDestroyTexture(gMousePadTextureId);
    UDestroyTexture(gInfinityCubeTextureId);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        cerr << glewGetErrorString(GlewInitResult) << endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    // Exit the program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { glfwSetWindowShouldClose(window, true); }

    // Keyboard movement keys
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { gCamera.ProcessKeyboard(FORWARD, gDeltaTime); }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { gCamera.ProcessKeyboard(BACKWARD, gDeltaTime); }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { gCamera.ProcessKeyboard(LEFT, gDeltaTime); }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { gCamera.ProcessKeyboard(RIGHT, gDeltaTime); }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) { gCamera.ProcessKeyboard(UP, gDeltaTime); }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) { gCamera.ProcessKeyboard(DOWN, gDeltaTime); }

    // Switch view mode
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !gOrthoView)
    {
        gOrthoView = true;
        cout << "Switched to orthographic view" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && gOrthoView)
    {
        gOrthoView = false;
        cout << "Switched to projection view" << endl;
    }

    // Enable/disable lamp orbiting
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
    {
        gIsLampOrbiting = true;
        cout << "Light orbit enabled" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
    {
        gIsLampOrbiting = false;
        cout << "Light orbit disabled" << endl;
    }

}

// GLFW: whenever the window size changed (by OS or user resize) this callback function executes
// -------------------------------------------------------
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// GLFW: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// GLFW: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
    {
        cout << "Camera zoom adjusted" << endl;
        gCamera.AdjustZoom(yoffset);
    }
    else
    {
        cout << "Camera movement speed adjusted" << endl;
        gCamera.AdjustMovementSpeed(yoffset);
    }
}

// GLFW: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
        if (action == GLFW_PRESS)
        {
            cout << "Camera position reset" << endl;
            gCamera.ResetCameraPosition();
        }
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        if (action == GLFW_PRESS &&
            (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS))
        {
            cout << "Camera zoom reset" << endl;
            gCamera.ResetCameraZoom();
        }
        else if (action == GLFW_PRESS)
        {
            cout << "Camera movement speed reset" << endl;
            gCamera.ResetCameraSpeed();
        }
        break;
    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

// Function called to render a frame
void URender()
{
    // Lamp orbits around the origin
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting)
    {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition, 1.0f);
        gLightPosition.x = newPosition.x;
        gLightPosition.y = newPosition.y;
        gLightPosition.z = newPosition.z;

        glm::vec4 newPosition2 = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(1.0f, 0.0f, 1.0f)) * glm::vec4(gLightPosition2, 1.0f);
        gLightPosition2.x = newPosition2.x;
        gLightPosition2.y = newPosition2.y;
        gLightPosition2.z = newPosition2.z;
    }

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.412f, 0.412f, 0.412f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Declare variables for rendering
    const glm::vec3 cameraPosition = gCamera.Position;
    glm::vec2 uvScale;
    glm::mat4 view,
        projection,
        scale,
        rotation,
        translation,
        model;
    GLint texWrapMode = GL_REPEAT,
        uvScaleLoc,
        viewLoc,
        projLoc,
        modelLoc,
        lightColorLoc,
        lightPositionLoc,
        viewPositionLoc,
        lightColorLoc2,
        lightPositionLoc2,
        viewPositionLoc2;

    if (gOrthoView)
    {
        // Camera/view transformation
        view = gCamera.GetViewMatrix();
        // Creates an orthographic (2D) projection
        projection = glm::ortho(-(GLfloat)WINDOW_WIDTH * 0.01f, (GLfloat)WINDOW_WIDTH * 0.01f, -(GLfloat)WINDOW_HEIGHT * 0.01f, (GLfloat)WINDOW_HEIGHT * 0.01f, 0.1f, 100.0f);
    }
    else
    {
        // Camera/view transformation
        view = gCamera.GetViewMatrix();
        // Creates a perspective (3D) projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // DESK: draw desk
    //----------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gPlaneMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(10.0f, 1.0f, 4.0f));
    translation = glm::translate(glm::vec3(0.0f, -1.0f, 0.0f));
    model = translation * scale; // Creates transform matrix

    // Reference matrix uniforms from the shader program
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    modelLoc = glGetUniformLocation(gProgramId, "model");

    // Pass matrix data to the shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    viewPositionLoc2 = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Set texture scale
    uvScale = glm::vec2(1.0f, 1.0f);
    uvScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gDeskTextureId);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gPlaneMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

    // HOMEPOD: draw speaker
    //----------------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gSphereMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(0.75f, 0.75f, 0.75f));
    translation = glm::translate(glm::vec3(6.5f, -0.25f, -2.0f));
    model = translation * scale; // Creates transform matrix

    // Reference matrix uniforms from the shader program
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    modelLoc = glGetUniformLocation(gProgramId, "model");

    // Pass matrix data to the shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    viewPositionLoc2 = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Set texture scale
    uvScale = glm::vec2(15.0f, 15.0f);
    uvScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gMeshFabricTextureId);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gSphereMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

    // HOMEPOD: draw base
    //-------------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gCylinderMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(0.5f, 0.25f, 0.5f));
    rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    translation = glm::translate(glm::vec3(6.5f, -0.4999f, -2.0f));
    model = translation * rotation * scale; // Creates transform matrix

    // Reference matrix uniforms from the shader program
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    modelLoc = glGetUniformLocation(gProgramId, "model");

    // Pass matrix data to the shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    viewPositionLoc2 = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Set texture scale
    uvScale = glm::vec2(1.0f, 1.0f);
    uvScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gRubberBaseTextureId);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gCylinderMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

    // MOUSE PAD: draw mouse pad
    //----------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gPlaneMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(2.25f, 1.0f, 2.0f));
    rotation = glm::rotate(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    translation = glm::translate(glm::vec3(6.0f, -0.999f, 1.9f));
    model = translation * rotation * scale; // Creates transform matrix

    // Reference matrix uniforms from the shader program
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    modelLoc = glGetUniformLocation(gProgramId, "model");

    // Pass matrix data to the shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    viewPositionLoc2 = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Set texture scale
    uvScale = glm::vec2(1.0f, 1.0f);
    uvScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gMousePadTextureId);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gPlaneMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

    // INFINITY CUBE: draw infinity cube
    //----------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gCubeMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(0.375f, 0.375f, 0.375f));
    rotation = glm::rotate(glm::radians(25.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-6.0f, -0.6249f, -1.0f));
    model = translation * rotation * scale; // Creates transform matrix

    // Reference matrix uniforms from the shader program
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    modelLoc = glGetUniformLocation(gProgramId, "model");

    // Pass matrix data to the shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    viewPositionLoc2 = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Set texture scale
    uvScale = glm::vec2(1.0f, 1.0f);
    uvScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gInfinityCubeTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nIndices);

    // WHITEBOARD: draw whiteboard
    //----------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gWedgeMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(4.5f, 0.625f, 1.5f));
    translation = glm::translate(glm::vec3(0.0f, -0.3749, -2.0f));
    model = translation * scale; // Creates transform matrix

    // Reference matrix uniforms from the shader program
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    modelLoc = glGetUniformLocation(gProgramId, "model");

    // Pass matrix data to the shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    viewPositionLoc2 = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Set texture scale
    uvScale = glm::vec2(1.0f, 1.0f);
    uvScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gInfinityCubeTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gWedgeMesh.nIndices);

    // Keyboard: draw keyboard
    //----------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gWedgeMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(4.125f, 0.125f, 1.125f));
    translation = glm::translate(glm::vec3(-0.75f, -0.8749, 2.0f));
    model = translation * scale; // Creates transform matrix

    // Reference matrix uniforms from the shader program
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");
    modelLoc = glGetUniformLocation(gProgramId, "model");

    // Pass matrix data to the shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms
    lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    viewPositionLoc2 = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Set texture scale
    uvScale = glm::vec2(1.0f, 1.0f);
    uvScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gInfinityCubeTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gWedgeMesh.nIndices);

    // LAMP 1: draw lamp
    //----------------
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gSphereMesh.vao);

    // Set the shader to be used
    glUseProgram(gLampProgramId);

    // Set scale, rotation, and translation
    scale = glm::scale(glm::vec3(1.0f, 0.5f, 1.0f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    model = translation * rotation * scale; // Creates transform matrix

    // Transform the smaller sphere used as a visual clue for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the lamp shader program
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");
    modelLoc = glGetUniformLocation(gLampProgramId, "model");

    // Pass matrix data to the lamp shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gSphereMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // LAMP 2: draw lamp
    //------------------
    // Set the shader to be used
    glUseProgram(gLampProgramId);

    // Transform the smaller sphere used as a visual clue for the light source
    model = glm::translate(gLightPosition2) * glm::scale(gLightScale2);

    // Reference matrix uniforms from the lamp shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the lamp shader program's matrix uniforms
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gSphereMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // Deactivate the Vertex Array Object and shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // GLFW: Swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow); // Flips the the back buffer with the front buffer every frame.
}

// Create cube mesh, specifying height of front and back (0 to 1, default to 1)
void UCreateCubeMesh(GLMesh& mesh, float frontHeight, float backHeight)
{
    if (frontHeight < 0 || frontHeight > 1 || backHeight < 0 || backHeight > 1)
    {
        throw invalid_argument("Height must be between 0 and 1");
    }

    float fh = -1 + (frontHeight * 2), bh = -1 + (backHeight * 2);

    GLfloat verts[] = {
        // Positions           // Normals            //Textures
        // ----------------------------------------------------
        // Back Face          Negative Z Normal     Texture Coords.
       -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
        1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
        1.0f,  bh,   -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
        1.0f,  bh,   -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
       -1.0f,  bh,   -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
       -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,

        // Front Face         Positive Z Normal     Texture Coords.
       -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
        1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
        1.0f,  fh,    1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
        1.0f,  fh,    1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
       -1.0f,  fh,    1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
       -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,

        // Left Face           Negative X Normal    Texture Coords.
       -1.0f,  fh,    1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
       -1.0f,  bh,   -1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
       -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
       -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
       -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
       -1.0f,  fh,    1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,

        // Right Face         Positive X Normal     Texture Coords.
        1.0f,  fh,    1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
        1.0f,  bh,   -1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
        1.0f,  fh,    1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,

        // Bottom Face        Negative Y Normal     Texture Coords.
       -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
        1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
        1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
        1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
       -1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
       -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,

        // Top Face           Positive Y Normal     Texture Coords.
       -1.0f,  bh,   -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
        1.0f,  bh,   -1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
        1.0f,  fh,    1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
        1.0f,  fh,    1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
       -1.0f,  fh,    1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
       -1.0f,  bh,   -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nIndices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Create cylinder mesh
void UCreateCylinderMesh(GLMesh& mesh)
{
    // Set number of sections making up the cylinder
    const int slices = 24;
    const float angle = 2 * numbers::pi / slices;

    const int secCircOffset = slices + 1;
    const int triangleSize = 3;

    glm::vec3 vertex[(slices + 1) * 2];

    const int numVerts = sizeof(vertex) / sizeof(vertex[0]) * 8;
    const int numIndices = slices * triangleSize * 4;

    GLfloat verts[numVerts];
    GLushort indices[numIndices];

    for (int i = 0; i <= slices; i++)
    {
        if (i == slices)
        {
            vertex[i] = glm::vec3(0, 0, 1);
            vertex[i + secCircOffset] = glm::vec3(0, 0, -1);
        }
        else
        {
            vertex[i] = glm::vec3(glm::cos(angle * i), glm::sin(angle * i), 1);
            vertex[i + secCircOffset] = glm::vec3(glm::cos(angle * i), glm::sin(angle * i), -1);
        }
    }

    // Position, normal, and texture data
    for (int i = 0; i < numVerts / 8; i++)
    {
        int pointer = i * 8;

        // Vertex positions
        verts[pointer] = vertex[i].x;
        verts[pointer + 1] = vertex[i].y;
        verts[pointer + 2] = vertex[i].z;

        // Normal
        verts[pointer + 3] = vertex[i].x;
        verts[pointer + 4] = vertex[i].y;
        verts[pointer + 5] = vertex[i].z;

        // Texture
        verts[pointer + 6] = vertex[i].x;
        verts[pointer + 7] = vertex[i].y;
    }

    // Index data to share position data
    for (int i = 0; i < slices; i++)
    {
        int pointer = i * triangleSize;

        indices[pointer] = i;
        indices[pointer + 1] = slices;
        indices[pointer + 2] = i + 1 < slices ? i + 1 : 0;

        pointer += slices * triangleSize;

        indices[pointer] = i + secCircOffset;
        indices[pointer + 1] = slices + secCircOffset;
        indices[pointer + 2] = i + 1 < slices ? i + 1 + secCircOffset : secCircOffset;

        pointer += slices * triangleSize;

        indices[pointer] = i;
        indices[pointer + 1] = i + secCircOffset;
        indices[pointer + 2] = i + 1 < slices ? i + 1 : 0;

        pointer += slices * triangleSize;

        indices[pointer] = i + secCircOffset;
        indices[pointer + 1] = i + 1 < slices ? i + 1 : 0;
        indices[pointer + 2] = i + 1 < slices ? i + 1 + secCircOffset : secCircOffset;
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV); // The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Create plane mesh
void UCreatePlaneMesh(GLMesh& mesh)
{
    GLfloat verts[] = {
        // Positions           // Normals            //Textures
        // ----------------------------------------------------
       -1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f, // Index 0 - top left
        1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f, // Index 1 - top right
        1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f, // Index 2 - bottom right
       -1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f, // Index 3 - bottom left
    };

    // Index data to share position data
    GLushort indices[] = {
        3, 0, 1, 1, 2, 3  // plane
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV); // The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Create pyramid mesh
void UCreatePyramidMesh(GLMesh& mesh)
{
    GLfloat verts[] = {
        // Positions           // Normals            //Textures
        // ----------------------------------------------------
        0.0f,  0.75f,  0.0f,   0.0f,  1.0f,  0.0f,   0.5f, 0.5f, // Index 0 - top point
        0.5f, -0.75f,  0.5f,   1.0f,  0.5f,  1.0f,   0.0f, 0.0f, // Index 1 - base point 1
       -0.5f, -0.75f,  0.5f,  -1.0f,  0.5f,  1.0f,   1.0f, 0.0f, // Index 2 - base point 2
        0.5f, -0.75f, -0.5f,   1.0f,  0.5f, -1.0f,   0.0f, 1.0f, // Index 3 - base point 3
       -0.5f, -0.75f, -0.5f,  -1.0f,  0.5f, -1.0f,   0.0f, 0.0f, // Index 4 - base point 4

    };

    // Index data to share position data
    GLushort indices[] = {
        1, 0, 3, // Triangle 1, pyramid side 1
        3, 0, 4, // Triangle 2, pyramid side 2
        4, 0, 2, // Triangle 3, pyramid side 3
        2, 0, 1, // Triangle 4, pyramid side 4
        2, 1, 3, // Triangle 5, pyramid bottom 1
        2, 4, 3, // Triangle 6, pyramid bottom 2
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV); // The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Create sphere mesh
void UCreateSphereMesh(GLMesh& mesh)
{
    const int complexity = 32;

    const float latAngle = 2 * numbers::pi / complexity;
    const float logAngle = numbers::pi / complexity;

    glm::vec3 vertex[complexity + 1][complexity + 1];

    const int numVerts = (complexity + 1) * (complexity + 1) * 8;
    const int numIndices = numVerts / 8 * 2 * 3;

    GLfloat verts[numVerts];
    GLushort indices[numIndices];

    // Position, normal, and texture data
    for (int i = 0; i < complexity + 1; i++)
    {
        float lat = (((i - 0) * (numbers::pi + numbers::pi)) / complexity) - numbers::pi;

        for (int j = 0; j < complexity + 1; j++)
        {
            float log = (((j - 0) * (numbers::pi / 2 + numbers::pi / 2)) / complexity) - numbers::pi / 2;

            vertex[i][j] = glm::vec3
            (
                glm::sin(lat) * glm::cos(log),
                glm::sin(lat) * glm::sin(log),
                glm::cos(lat)
            );

            int pointer = ((i * (complexity + 1)) + j) * 8;

            // Vertex positions
            verts[pointer] = vertex[i][j].x;
            verts[pointer + 1] = vertex[i][j].y;
            verts[pointer + 2] = vertex[i][j].z;

            // Normal
            verts[pointer + 3] = vertex[i][j].x;
            verts[pointer + 4] = vertex[i][j].y;
            verts[pointer + 5] = vertex[i][j].z;

            // Texture
            verts[pointer + 6] = vertex[i][j].x / 2 + 0.5;
            verts[pointer + 7] = vertex[i][j].y / 2 + 0.5;

            pointer = ((i * (complexity + 1)) + j) * 6;

            indices[pointer] = i * (complexity + 1) + j;
            indices[pointer + 1] = (i + 1) * (complexity + 1) + j;
            indices[pointer + 2] = i * (complexity + 1) + (j + 1);

            indices[pointer + 3] = (i + 1) * (complexity + 1) + (j + 1);
            indices[pointer + 4] = (i + 1) * (complexity + 1) + j;
            indices[pointer + 5] = i * (complexity + 1) + (j + 1);
        }
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV); // The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

// Generate and load the texture
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        }
        else if (channels == 4)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        }
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // Compile the vertex shader
    // Check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // Compile the fragment shader
    // Check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId); // Links the shader program
    // Check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;

        return false;
    }

    glUseProgram(programId); // Uses the shader program

    return true;
}

void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

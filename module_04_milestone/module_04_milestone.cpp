/*
 * Module 4 Milestone
 * Eric Slutz
 */

#include <iostream> // cout, cerr
#include <cstdlib> // EXIT_FAILURE
#include <GL/glew.h> // GLEW library
#include <GLFW/glfw3.h> // GLFW library
#include <numbers> // pi

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Camera class
#include <learnOpengl/camera.h>

using namespace std; // Standard namespace

/* Shader program Macro */
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    // Macro for window title
    const char* const WINDOW_TITLE = "Module 4 Milestone - HomePod mini on Desk";

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
    // Plane mesh data
    GLMesh planeMesh;
    // Cube mesh data
    GLMesh cubeMesh;
    // Cylinder mesh data
    GLMesh cylinderMesh;
    // Sphere mesh data
    GLMesh sphereMesh;
    // Shader program
    GLuint gProgramId;
}
    // Camera
    Camera gCamera(glm::vec3(0.0f, 2.0f, 15.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // Timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // View mode
    bool orthoView = false;

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
void CreatePlaneMesh(GLMesh& mesh);
void CreateCubeMesh(GLMesh& mesh);
void CreateCylinderMesh(GLMesh& mesh);
void CreateSphereMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
void RenderMesh(GLMesh& mesh, glm::vec3 scale, glm::vec3 rotation, glm::vec3 translation); 
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

/* Vertex Shader Source Code */
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
layout(location = 1) in vec4 color;  // Color data from Vertex Attrib Pointer 1

out vec4 vertexColor; // Variable to transfer color data to the fragment shader

// Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices to clip coordinates
    vertexColor = color; // References incoming color data
}
);

/* Fragment Shader Source Code */
const GLchar* fragmentShaderSource = GLSL(440, in vec4 vertexColor; // Variable to hold incoming color data from vertex shader

out vec4 fragmentColor;

void main()
{
    fragmentColor = vec4(vertexColor);
}
);

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
    {
        return EXIT_FAILURE;
    }

    // Create the mesh
    CreatePlaneMesh(planeMesh);
    CreateCubeMesh(cubeMesh);
    CreateCylinderMesh(cylinderMesh);
    CreateSphereMesh(sphereMesh);

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
    {
        return EXIT_FAILURE;
    }

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.412f, 0.412f, 0.412f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(planeMesh);
    UDestroyMesh(cubeMesh);
    UDestroyMesh(cylinderMesh);
    UDestroyMesh(sphereMesh);

    // Release shader program
    UDestroyShaderProgram(gProgramId);

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

    // Tell GLFW to capture the mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        orthoView = !orthoView;
        cout << (orthoView ? "Switched to orthographic view" : "Switched to projection view") << endl;
    }
}

// GLFW: whenever the window size changed (by OS or user resize) this callback function executes
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
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.412f, 0.412f, 0.412f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Declare view and projection
    glm::mat4 view;
    glm::mat4 projection;

    if (orthoView)
    {
        // Set width and height for orthographic view
        GLfloat orthoWidth = (GLfloat)WINDOW_WIDTH * 0.01f;
        GLfloat orthoHeight = (GLfloat)WINDOW_HEIGHT * 0.01f;
        // Camera/view transformation
        view = gCamera.GetViewMatrix();
        // Creates an orthographic (2D) projection
        projection = glm::ortho((GLfloat)WINDOW_WIDTH * 0.01f, (GLfloat)WINDOW_WIDTH * 0.01f, -(GLfloat)WINDOW_HEIGHT * 0.01f, (GLfloat)WINDOW_HEIGHT * 0.01f, 0.1f, 100.0f);
    }
    else
    {
        // Camera/view transformation
        view = gCamera.GetViewMatrix();
        // Creates a perspective (3D) projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // Retrieves and passes view and projection matrices to the Shader program
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    /* Desk */
    // Render plane
    RenderMesh(planeMesh, glm::vec3(10.0f, 1.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    
    /* HomePod mini */
    // Render cylinder
    RenderMesh(cylinderMesh, glm::vec3(1.0f, 0.5f, 1.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    // Render sphere
    RenderMesh(sphereMesh, glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // GLFW: Swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow); // Flips the the back buffer with the front buffer every frame.
}

// Create plane mesh
void CreatePlaneMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        // Vertex Positions      // Colors (r,g,b,a)
         -1.0f,  0.0f,  1.0f,   0.0f, 1.0f, 0.0f, 1.0f, // Index 0 - top left
          1.0f,  0.0f,  1.0f,   1.0f, 0.0f, 0.0f, 1.0f, // Index 1 - top right
          1.0f,  0.0f, -1.0f,   0.0f, 1.0f, 0.0f, 1.0f, // Index 2 - bottom right
         -1.0f,  0.0f, -1.0f,   1.0f, 0.0f, 0.0f, 1.0f, // Index 3 - bottom left
    };

    // Index data to share position data
    GLushort indices[] = {
        3, 0, 1, 1, 2, 3  // plane
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;

    glGenVertexArrays(1, &mesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor); // The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}

// Create cube mesh
void CreateCubeMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        // Vertex Positions      // Colors (r,g,b,a)
          1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 0.0f, 1.0f, // Index 0 - top point, quad 1
         -1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 0.0f, 1.0f, // Index 1 - top point, quad 2
         -1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 1.0f, 1.0f, // Index 2 - top point, quad 3
          1.0f, -1.0f,  1.0f,   1.0f, 1.0f, 0.0f, 1.0f, // Index 3 - top point, quad 4

          1.0f,  1.0f, -1.0f,   1.0f, 0.0f, 0.0f, 1.0f, // Index 4 - bottom point, quad 1
         -1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 0.0f, 1.0f, // Index 5 - bottom point, quad 2
         -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 1.0f, 1.0f, // Index 6 - bottom point, quad 3
          1.0f, -1.0f, -1.0f,   1.0f, 1.0f, 0.0f, 1.0f, // Index 7 - bottom point, quad 4
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 2, 2, 3, 0,  // cube top
        4, 5, 6, 6, 7, 4,  // cube bottom
        1, 0, 4, 4, 5, 1,  // cube side 1
        2, 1, 5, 5, 6, 2,  // cube side 2
        3, 2, 6, 6, 7, 3,  // cube side 3
        0, 3, 7, 7, 4, 0,  // cube side 4
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;

    glGenVertexArrays(1, &mesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor); // The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}

// Create cylinder mesh
void CreateCylinderMesh(GLMesh& mesh)
{
    // Set number of sections making up the cylinder
    const int slices = 24;
    const float angle = 2 * numbers::pi / slices;

    const int secCircOffset = slices + 1;
    const int triangleSize = 3;

    glm::vec3 vertex[(slices + 1) * 2];

    const int numVerts = sizeof(vertex) / sizeof(vertex[0]) * 7;
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

    // Position and Color data
    for (int i = 0; i < numVerts / 7; i++)
    {
        int pointer = i * 7;

        // Vertex Positions
        verts[pointer] = vertex[i].x;
        verts[pointer + 1] = vertex[i].y;
        verts[pointer + 2] = vertex[i].z;

        // Colors (r,g,b,a)
        verts[pointer + 3] = i % 3 == 0 ? 1.0f : 0.0f;
        verts[pointer + 4] = i % 3 == 1 ? 1.0f : 0.0f;
        verts[pointer + 5] = i % 3 == 2 ? 1.0f : 0.0f;
        verts[pointer + 6] = 1.0f;
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
    const GLuint floatsPerColor = 4;

    glGenVertexArrays(1, &mesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor); // The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}

// Create cylinder mesh
void CreateSphereMesh(GLMesh& mesh)
{
    const int complexity = 32;

    const float latAngle = 2 * numbers::pi / complexity;
    const float logAngle = numbers::pi / complexity;

    glm::vec3 vertex[complexity + 1][complexity + 1];

    const int numVerts = (complexity + 1) * (complexity + 1) * 7;
    const int numIndices = numVerts / 7 * 2 * 3;

    GLfloat verts[numVerts];
    GLushort indices[numIndices];

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

            int pointer = ((i * (complexity + 1)) + j) * 7;

            verts[pointer] = vertex[i][j].x;
            verts[pointer + 1] = vertex[i][j].y;
            verts[pointer + 2] = vertex[i][j].z;

            verts[pointer + 3] = i % 3 == 0 ? 1.0f : 0.0f;
            verts[pointer + 4] = i % 3 == 1 ? 1.0f : 0.0f;
            verts[pointer + 5] = i % 3 == 2 ? 1.0f : 0.0f;
            verts[pointer + 6] = 1.0f;

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
    const GLuint floatsPerColor = 4;

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}

// Renders the passed mesh with any transformations
void RenderMesh(GLMesh& mesh, glm::vec3 scale, glm::vec3 rotation, glm::vec3 translation)
{
    // Creates rotations matrices
    glm::mat4 rotateX = glm::rotate(glm::radians(rotation.x), glm::vec3(1, 0, 0));
    glm::mat4 rotateY = glm::rotate(glm::radians(rotation.y), glm::vec3(0, 1, 0));
    glm::mat4 rotateZ = glm::rotate(glm::radians(rotation.z), glm::vec3(0, 0, 1));

    // Creates transform matrix
    glm::mat4 model = glm::translate(translation) * (rotateX * rotateY * rotateZ) * glm::scale(scale);

    // Set the shader to be used
    glUseProgram(gProgramId);
 
    // Gemotry shader parameters
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh.vao);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, mesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Destroys the mesh
void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
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
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success); // Check for shader compile errors
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // Compile the fragment shader
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success); // Check for shader compile errors
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
    glGetProgramiv(programId, GL_LINK_STATUS, &success); // Check for linking errors
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;

        return false;
    }

    glUseProgram(programId); // Uses the shader program

    return true;
}

// Destroys the shader
void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

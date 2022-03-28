#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int loadCubeMap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    bool spotlight = true;
    PointLight pointLight;
    float whiteAmbientLightStrength = 0.0f;
    bool grayscaleEnabled = false;
    bool AAEnabled = true;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
    glm::vec3 tempPosition=glm::vec3(0.0f, 2.0f, 0.0f);
    float tempScale=1.0f;
    float tempRotation=0.0f;
};
void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << spotlight << '\n'
        << whiteAmbientLightStrength << '\n';
}
void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> spotlight
           >> whiteAmbientLightStrength;
    }
}
ProgramState *programState;

void DrawImGui(ProgramState *programState);

glm::mat4 CalcFlashlightPosition();


int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Projekat", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // dozvoljava da prozor menja velicinu, ali cuva 4:3 odnos
    glfwSetWindowAspectRatio(window, 4, 3);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    // tell stb_image.h to flip loaded textures on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // build and compile shaders
    Shader objShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader screenShader("resources/shaders/anti-aliasing.vs", "resources/shaders/anti-aliasing.fs");
    Shader simpleDepthShader("resources/shaders/depthShader.vs", "resources/shaders/depthShader.fs", "resources/shaders/depthShader.gs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    screenShader.setInt("screenTexture", 0);
    //dodat shader za travu
    Shader discardShader("resources/shaders/discard_shader.vs", "resources/shaders/discard_shader.fs");

    // load models
    Model ourModel("resources/objects/backpack/backpack.obj");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model ulicnaSvetiljkaModel("resources/objects/Street Lamp2/StreetLamp.obj");
    ulicnaSvetiljkaModel.SetShaderTextureNamePrefix("material.");

    // iz nekog razloga mora da se obrne tekstura
    stbi_set_flip_vertically_on_load(false);
    Model flashlightModel("resources/objects/flashlight/flashlight.obj");
    flashlightModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    //postavljamo vertexe
    //za travu
    float transparentVertices2[] = {
            // positions                        // texture Coords
            0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  1.0f,

            0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    //podloga

    float podlogaVertices[] = {
            // positions          // texture coords
            200.0f,  0.0f, -200.0f, 0.0f, 1.0f, 0.0f,   200.0f, 200.0f, // top right
            200.0f, 0.0f, 200.0f, 0.0f, 1.0f, 0.0f,  200.0f, 0.0f, // bottom right
            -200.0f, 0.0f, 200.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
            -200.0f,  0.0f, -200.0f, 0.0f, 1.0f, 0.0f,  0.0f, 200.0f  // top left
    };
    unsigned int podlogaIndices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    //skybox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // transparent VAO for grass
    unsigned int transparentVAO2, transparentVBO2;
    glGenVertexArrays(1, &transparentVAO2);
    glGenBuffers(1, &transparentVBO2);
    glBindVertexArray(transparentVAO2);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices2), transparentVertices2, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    //podloga VAO
    unsigned int podlogaVBO, podlogaVAO, podlogaEBO;
    glGenVertexArrays(1, &podlogaVAO);
    glGenBuffers(1, &podlogaVBO);
    glGenBuffers(1, &podlogaEBO);

    glBindVertexArray(podlogaVAO);

    glBindBuffer(GL_ARRAY_BUFFER, podlogaVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(podlogaVertices), podlogaVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, podlogaEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(podlogaIndices), podlogaIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normale coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);

    //load textures
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/front.png"),
                    FileSystem::getPath("resources/textures/skybox/back.png"),
                    FileSystem::getPath("resources/textures/skybox/top.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom.png"),
                    FileSystem::getPath("resources/textures/skybox/left.png"),
                    FileSystem::getPath("resources/textures/skybox/right.png")
            };
    vector<glm::vec3> vegetation
            {
                    glm::vec3(-1.5f, 0.5f, -0.48f),
                    glm::vec3( 1.5f, 0.5f, 0.51f),
                    glm::vec3( 0.0f, 0.5f, 0.7f),
                    glm::vec3(-0.7f, 0.5f, -2.3f),
                    glm::vec3 (1.0f, 0.5f, -1.2f),
                    glm::vec3 (-0.1f, 0.5f, -0.63f),
                    glm::vec3 (-1.75f, 0.5f, 1.0f),
                    glm::vec3 (-0.6f, 0.5f, -2.0f)
            };

    //texture loading
    // -------------------------
    unsigned int diffuseMap = TextureFromFile("grass_diffuse.png", "resources/textures/");
    unsigned int specularMap = TextureFromFile("grass_specular.png", "resources/textures/");
    unsigned int transparentTexture = TextureFromFile("grass.png", "resources/textures/");
    unsigned int cubeMapTexture = loadCubeMap(faces);
    // uklonio sam loadTexture funkciju jer vec imamo njen ekvivalent u model.h fajlu
    unsigned int podlogaDiffuseMap = TextureFromFile("grass_diffuse.png", "resources/textures");
    unsigned int podlogaSpecularMap = TextureFromFile("grass_specular.png", "resources/textures");

    // --------------------------------------------- ANTI-ALIASING ------------------------------------------------------------
    // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
    float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    unsigned int textureColorBufferMultiSampled;
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER Framebuffer is not complete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // ------------------------------------------------------------------------------------------------------------------------


    // za pointlajt
    glm::vec3 lightPos(-5.0f, 4.0f, -5.0f);

    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    unsigned int depthCubeMap;
    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
    for(int i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthCubeMap, 0);
    // u ovaj framebuffer necemo da renderujemo boju, jer ce da cuva samo dubinu fragmenata
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // konfiguracija shadera
    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    objShader.use();
    objShader.setInt("material.texture_diffuse1", 0);
    objShader.setInt("material.texture_specular1", 1);
    objShader.setInt("depthMap", 2);

    //oke zar ne treba ovde da se setuje redni broj semplera??
    discardShader.use();
    discardShader.setInt("texture1", 0);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        glm::mat4 model = glm::mat4(1.0f);

        // renderovanje scene iz pozicije svetla
        // -------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);


        float near_plane = 1.0f;
        float far_plane  = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);

            simpleDepthShader.use();
            for(unsigned int i = 0; i < 6; i++)
                simpleDepthShader.setMat4("shadowMatrices[" + std::to_string((i)) +"]", shadowTransforms[i]);
            simpleDepthShader.setFloat("far_plane", far_plane);
            simpleDepthShader.setVec3("lightPos", lightPos);

            // renderovanje ranca:
            model = glm::mat4(1.0f);
            model = glm::translate(model,programState->tempPosition);
            model = glm::scale(model, glm::vec3(programState->tempScale));
            simpleDepthShader.setMat4("model", model);
            ourModel.Draw(simpleDepthShader);

    
            //renderovanje lampe
            for(int i = 0; i < 5; i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-4.0f, 0.0f, i * 4.0f));
                model = glm::scale(model, glm::vec3(0.4f));
                model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0, 1, 0));
                simpleDepthShader.setMat4("model", model);
                ulicnaSvetiljkaModel.Draw(simpleDepthShader);
            }
    
            //podloga

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, podlogaDiffuseMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, podlogaSpecularMap);

            model = glm::mat4(1.0f);
            simpleDepthShader.setMat4("model", model);

            //za blinfonga treba shinnes 4* veci al trava ne sme da se presijava bas tako da tu treba obratiti paznju
            //simpleDepthShader.setFloat("material.shininess", 32.0f);
            glBindVertexArray(podlogaVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // -------------------------------------------

        // render

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ANTI-ALIASING: preusmeravamo renderovanje na nas framebuffer da bismo imali MSAA
        // *************************************************************************************************************
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        // *************************************************************************************************************


        //object shader
        objShader.use();
        objShader.setVec3("viewPosition", programState->camera.Position);
        objShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        objShader.setMat4("projection", projection);
        objShader.setMat4("view", view);


        // directional light
        objShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        objShader.setVec3("dirLight.ambient", glm::vec3(programState->whiteAmbientLightStrength));
        objShader.setVec3("dirLight.diffuse", 0.05f, 0.05f, 0.05);
        objShader.setVec3("dirLight.specular", 0.2f, 0.2f, 0.2f);

        //ovde treba pointlajtovi
        //nasao sam kul nacin da obradjujemo vise pointlatova ako nam treba

        // jedan pointlajt za testiranje senki
//        lightPos.x = sin(glfwGetTime()) * 3.0f;
//        lightPos.z = cos(glfwGetTime()) * 2.0f;
//        lightPos.y = 5.0 + cos(glfwGetTime()) * 1.0f;
        objShader.setVec3("pointLight.position", lightPos);
        objShader.setVec3("pointLight.ambient", glm::vec3(1.0f));
        objShader.setVec3("pointLight.diffuse", 0.05f, 0.05f, 0.05);
        objShader.setVec3("pointLight.specular", 0.2f, 0.2f, 0.2f);
        objShader.setFloat("pointLight.constant", 1.0f);
        objShader.setFloat("pointLight.linear", 0.09f);
        objShader.setFloat("pointLight.quadratic", 0.032f);

        //spotlight
        objShader.setVec3("lampa.position", programState->camera.Position);
        objShader.setVec3("lampa.direction", programState->camera.Front);
        objShader.setVec3("lampa.ambient", 0.0f, 0.0f, 0.0f);
        if(programState->spotlight) {
            objShader.setVec3("lampa.diffuse", 1.0f, 1.0f, 1.0f);
            objShader.setVec3("lampa.specular", 1.0f, 1.0f, 1.0f);
        }
        else {
            objShader.setVec3("lampa.diffuse", 0.0f, 0.0f, 0.0f);
            objShader.setVec3("lampa.specular", 0.0f, 0.0f, 0.0f);
        }
        objShader.setFloat("lampa.constant", 1.0f);
        objShader.setFloat("lampa.linear", 0.09f);
        objShader.setFloat("lampa.quadratic", 0.032f);
        objShader.setFloat("lampa.cutOff", glm::cos(glm::radians(10.0f)));
        objShader.setFloat("lampa.outerCutOff", glm::cos(glm::radians(15.0f)));

        objShader.setFloat("far_plane", far_plane);

        // renderovanje ranca:
        model = glm::mat4(1.0f);
        model = glm::translate(model,programState->tempPosition);
        model = glm::scale(model, glm::vec3(programState->tempScale));
        objShader.setMat4("model", model);
        ourModel.Draw(objShader);

        // renderovanje baterijske lampe:
        model = CalcFlashlightPosition();
        objShader.setMat4("model", model);
        flashlightModel.Draw(objShader);

        //renderovanje lampe
        for(int i = 0; i < 5; i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.0f, 0.0f, i * 4.0f));
            model = glm::scale(model, glm::vec3(0.4f));
            model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0, 1, 0));
            objShader.setMat4("model", model);
            ulicnaSvetiljkaModel.Draw(objShader);
        }

        glDisable(GL_CULL_FACE);
        discardShader.use();
        discardShader.setMat4("projection", projection);
        discardShader.setMat4("view", view);
        glBindVertexArray(transparentVAO2);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        model = glm::mat4(1.0f);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            //model = glm::rotate(model, (float)i*60.0f, glm::vec3(0.0, 0.1, 0.0));
            discardShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        //podloga


        objShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, podlogaDiffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, podlogaSpecularMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);

        model = glm::mat4(1.0f);
        objShader.setMat4("model", model);

        //za blinfonga treba shinnes 4* veci al trava ne sme da se presijava bas tako da tu treba obratiti paznju
        //objShader.setFloat("material.shininess", 32.0f);
        glBindVertexArray(podlogaVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glEnable(GL_CULL_FACE);


        /* kada ucitavas novi model koristis ovaj templejt
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(programState->tempPosition));
        model = glm::scale(model, glm::vec3(programState->tempScale));
        model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        x.Draw(objShader);
         */



        //object rendering end, start of skybox rendering
        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);

        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content

        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // set depth function back to default


        // ANTI-ALIASING: ukljucivanje
        // *************************************************************************************************************
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        screenShader.use();

        screenShader.setFloat("SCR_WIDTH", SCR_WIDTH);
        screenShader.setFloat("SCR_HEIGHT", SCR_HEIGHT);
        screenShader.setBool("grayscaleEnabled", programState->grayscaleEnabled);

        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // *************************************************************************************************************

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);
    glDeleteVertexArrays(1, &transparentVAO2);
    glDeleteBuffers(1, &transparentVAO2);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            programState->CameraMouseMovementUpdateEnabled = false;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            programState->CameraMouseMovementUpdateEnabled = true;
        }

    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS && programState->ImGuiEnabled) {
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
        if(programState->CameraMouseMovementUpdateEnabled == true)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // ANTI-ALIASING key callbacks:
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        programState->AAEnabled = !programState->AAEnabled;
        if(programState->AAEnabled)
            glEnable(GL_MULTISAMPLE);
        if(!programState->AAEnabled)
            glDisable(GL_MULTISAMPLE);
    }
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
        programState->grayscaleEnabled = !programState->grayscaleEnabled;


    // reset the camera to default position
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        programState->camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
        programState->camera.Yaw = -90.0f;
        programState->camera.Pitch = 0.0f;
        programState->camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        programState->spotlight = !programState->spotlight;
    }

}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(500, 150));
        ImGui::Begin("Settings:");
        ImGui::DragFloat("Ambient Light Strength", (float *) &programState->whiteAmbientLightStrength, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat3("Backpack position", (float*)&programState->tempPosition);
        ImGui::DragFloat("Backpack scale", &programState->tempScale, 0.05, 0.1, 4.0);
        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(0,200));
        ImGui::SetNextWindowSize(ImVec2(500, 150), ImGuiCond_Once);
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Spacing();
        ImGui::Bullet();
        ImGui::Text("Toggle camera movement on/off: C");
        ImGui::Bullet();
        ImGui::Text("Reset camera position: P");
        ImGui::Bullet();
        ImGui::Text("Toggle camera spotlight on/off: K");
        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(0,400));
        ImGui::SetNextWindowSize(ImVec2(500, 150), ImGuiCond_Once);
        ImGui::Begin("Anti-aliasing settings");
        ImGui::Checkbox("Anti-Aliasing (shortcut: F2)", &programState->AAEnabled);
        ImGui::Checkbox("Grayscale (shortcut: F3)", &programState->grayscaleEnabled);
        ImGui::End();
    }

    {
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - 60,0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(60, 50));
        ImGui::Begin("FPS:");
        ImGui::Text(("%.2f"), floor(1/deltaTime));
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


glm::mat4 CalcFlashlightPosition() {
    Camera c = programState->camera;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, c.Position + 0.35f * c.Front + 0.07f * c.Right - 0.08f * c.Up);

    model = glm::rotate(model, -glm::radians(c.Yaw + 180), c.Up);
    model = glm::rotate(model, glm::radians(c.Pitch), c.Right);

    model = glm::scale(model, glm::vec3(0.025f));

    return model;
}

unsigned int loadCubeMap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "CubeMap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
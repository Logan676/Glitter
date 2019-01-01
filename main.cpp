#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.h"

#include <shader_m.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 8.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(0.7f, -1.0f, 5.0f);

const char *vertexShaderSource ="#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoords;"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"out vec2 TexCoords;\n"

"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"

"void main()\n"
"{\n"
"   FragPos = vec3(model * vec4(aPos, 1.0));\n"
"   Normal = aNormal;\n"
"   TexCoords = aTexCoords;\n"

"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\0";

const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"

"struct Material {"
"   sampler2D diffuse;\n"
"   sampler2D specular;\n"
"   float shininess;\n"
"};\n"


"struct DirLight {"
"   vec3 direction;\n"

"   vec3 ambient;\n"
"   vec3 diffuse;\n"
"   vec3 specular;\n"
"};\n"

"struct PointLight {"
"   vec3 position;\n"

"   float constant;\n"
"   float linear;\n"
"   float quadratic;\n"

"   vec3 ambient;\n"
"   vec3 diffuse;\n"
"   vec3 specular;\n"
"};\n"

"struct SpotLight {\n"
"   vec3 position;\n"
"   vec3 direction;\n"

"   float cutOff;\n"
"   float outerCutOff;\n"

"   float constant;\n"
"   float linear;\n"
"   float quadratic;\n"

"   vec3 ambient;\n"
"   vec3 diffuse;\n"
"   vec3 specular;\n"
"};\n"

"#define NR_POINT_LIGHTS 4\n"

"in vec3 Normal;\n"
"in vec3 FragPos;\n"
"in vec2 TexCoords;\n"

"uniform vec3 viewPos;\n"
"uniform DirLight dirLight;\n"
"uniform SpotLight spotLight;\n"
"uniform PointLight pointLights[NR_POINT_LIGHTS];\n"
"uniform Material material;\n"

"vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);\n"
"vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);\n"
"vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);\n"

"void main()\n"
"{\n"
"   vec3 normal = normalize(Normal);\n"
"   vec3 viewDir = viewPos - FragPos;\n"
"   vec3 result;\n"
"   result = CalcDirLight(dirLight, normal, viewDir);\n"
"   for(int i = 0; i < NR_POINT_LIGHTS; i++) {\n"
"       result += CalcPointLight(pointLights[i], normal, FragPos, viewDir);\n"
"   }\n"
"   result += CalcSpotLight(spotLight, normal, FragPos, viewDir);\n"
"   FragColor = vec4(result, 1.0);\n"
"}\n"

"vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {\n"
"   vec3 lightDir = normalize(-light.direction);\n"

"   float diff = max(dot(normal, lightDir), 0.0f);\n"

"   vec3 reflectDir = reflect(-lightDir, normal);\n"
"   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"

"   vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));\n"

"   vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));\n"

"   vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));\n"

"   return (ambient + diffuse + specular);\n"
"}\n"

"vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {\n"
"   vec3 lightDir = normalize(light.position - fragPos);\n"

"   float diff = max(dot(normal, lightDir), 0.0f);\n"

"   vec3 reflectDir = reflect(-lightDir, normal);\n"
"   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"

"   float distance = length(light.position - fragPos);\n"
"   float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"

"   vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));\n"

"   vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));\n"

"   vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));\n"
"   ambient  *= attenuation;\n"
"   diffuse  *= attenuation;\n"
"   specular *= attenuation;\n"
"   return (ambient + diffuse + specular);\n"
"}\n"

"vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {\n"
"   vec3 lightDir = normalize(light.position - fragPos);\n"

"   float diff = max(dot(normal, lightDir), 0.0f);\n"

"   vec3 reflectDir = reflect(-lightDir, normal);\n"
"   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"

"   float distance = length(light.position - fragPos);\n"
"   float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"


"   float theta = dot(lightDir, normalize(-light.direction));\n"
"   float epsilon = light.cutOff - light.outerCutOff;\n"
"   float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);\n"

"   vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));\n"

"   vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));\n"

"   vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));\n"
"   ambient  *= attenuation * intensity;\n"
"   diffuse  *= attenuation * intensity;\n"
"   specular *= attenuation * intensity;\n"
"   return (ambient + diffuse + specular);\n"
"}\n\0";

const char *lightVertexShaderSource ="#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\0";

const char *lightFragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.1);\n"
"}\n\0";

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif
    
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
//    glfwSetCursorPosCallback(window, mouse_callback);
//    glfwSetScrollCallback(window, scroll_callback);
    
    // tell GLFW to capture our mouse
//    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    
    // build and compile our shader zprogram
    // ------------------------------------
    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // vertex shader
    int lightVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(lightVertexShader, 1, &lightVertexShaderSource, NULL);
    glCompileShader(lightVertexShader);
    // check for shader compile errors
    glGetShaderiv(lightVertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(lightVertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::LIGHT SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // light fragment shader
    int lightFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(lightFragmentShader, 1, &lightFragmentShaderSource, NULL);
    glCompileShader(lightFragmentShader);
    // check for shader compile errors
    glGetShaderiv(lightFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(lightFragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::LIGHT FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // link shaders
    int lightingShaderProgram = glCreateProgram();
    glAttachShader(lightingShaderProgram, vertexShader);
    glAttachShader(lightingShaderProgram, fragmentShader);
    glLinkProgram(lightingShaderProgram);
    // check for linking errors
    glGetProgramiv(lightingShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(lightingShaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // link shaders
    int lampShaderProgram = glCreateProgram();
    glAttachShader(lampShaderProgram, lightVertexShader);
    glAttachShader(lampShaderProgram, lightFragmentShader);
    glLinkProgram(lampShaderProgram);
    // check for linking errors
    glGetProgramiv(lampShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(lampShaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::LAMP SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(lightVertexShader);
    glDeleteShader(lightFragmentShader);
    
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
   
    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };
    
    // positions of the point lights
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };
    
    unsigned int cubeVAO, VBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    
    // 只需要绑定VBO不用再次设置VBO的数据，因为箱子的VBO数据中已经包含了正确的立方体顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // texture attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------
    unsigned int diffuseMap = loadTexture("/Users/Logan/development/opengl2/Glitter/resources/textures/container2.png");
    unsigned int specularMap = loadTexture("/Users/Logan/development/opengl2/Glitter/resources/textures/container2_specular.png");
    
    // shader configuration
    // --------------------
    glUseProgram(lightingShaderProgram);
    glUniform1i(glGetUniformLocation(lightingShaderProgram, "material.diffuse"), 0);
    glUniform1i(glGetUniformLocation(lightingShaderProgram, "material.specular"), 1);
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);
        
        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // render container
        glUseProgram(lightingShaderProgram);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        
        
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "material.ambient"), 0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "material.diffuse"),  0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "material.specular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "material.shininess"),  32.0f);

        glUniform3f(glGetUniformLocation(lightingShaderProgram, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "dirLight.ambient"),  0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "dirLight.diffuse"),  0.4f, 0.4f, 0.4f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "dirLight.specular"),  0.5f, 0.5f, 0.5f);
        
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[0].position"),  pointLightPositions[0].x,pointLightPositions[0].y,pointLightPositions[0].z);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[0].ambient"),  0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[0].diffuse"),  0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[0].specular"),  1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[0].constant"),  1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[0].linear"),  0.09);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[0].quadratic"), 0.032);
        
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[1].position"),  pointLightPositions[1].x,pointLightPositions[1].y,pointLightPositions[1].z);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[1].ambient"),  0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[1].diffuse"),  0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[1].specular"),  1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[1].constant"),  1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[1].linear"),  0.09);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[1].quadratic"), 0.032);
        
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[2].position"),  pointLightPositions[2].x,pointLightPositions[2].y,pointLightPositions[2].z);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[2].ambient"),  0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[2].diffuse"),  0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[2].specular"),  1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[2].constant"),  1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[2].linear"),  0.09);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[2].quadratic"), 0.032);
        
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[3].position"),  pointLightPositions[3].x,pointLightPositions[3].y,pointLightPositions[3].z);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[3].ambient"),  0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[3].diffuse"),  0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "pointLights[3].specular"),  1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[3].constant"),  1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[3].linear"),  0.09);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "pointLights[3].quadratic"), 0.032);
        
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "spotLight.position"),  camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "spotLight.direction"),  camera.Front.x,  camera.Front.y,  camera.Front.z);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "spotLight.ambient"),  0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "spotLight.diffuse"),  1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(lightingShaderProgram, "spotLight.specular"),  1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "spotLight.constant"),  1.0f);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "spotLight.linear"),  0.09);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "spotLight.quadratic"),  0.032);
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
        glUniform1f(glGetUniformLocation(lightingShaderProgram, "spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(lightingShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lightingShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv (glGetUniformLocation(lightingShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // with the uniform matrix set, draw the first container
        glBindVertexArray(cubeVAO);
        
        for(unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
           glUniformMatrix4fv (glGetUniformLocation(lightingShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
           glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        
        glUseProgram(lampShaderProgram);
        
        model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(lampShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lampShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(lightVAO);
        for (unsigned int i = 0; i < 4; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
            glUniformMatrix4fv(glGetUniformLocation(lampShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);
    
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    
    return textureID;
}

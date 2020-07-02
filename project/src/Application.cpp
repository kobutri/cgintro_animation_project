//
// Created by trist on 15/06/2020.
//

#include "Application.h"

float vertices[] = {
        -0.04f, 0.04f, 0.0f, 1.0f,
        -0.04f, -0.04f, 0.0f, 1.0f,
        0.08f, -0.0f, 0.0f, 1.0f,
};

unsigned int indices[] = {
        // note that we start from 0!
        0, 2, 1
};

Application &Application::create_app() {
    static Application app;
    return app;
}

Application::Application() {
    initWindow();
    setupGL();
    loadScene();
    loadShaders();
    createNodes();
}

Application::~Application() {
    terminateWindow(window);
}

void Application::run() {

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput();
        showFPS();
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Application::initWindow() {
    if (!glfwInit())
        throw std::runtime_error("failed to init GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(1200, 1000, "Underwater World", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("failed to create window");
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, this);
    this->window = window;
}

void Application::terminateWindow(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::loadGL() {
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        throw std::runtime_error("failed to initialize GLAD");
    }
}

void Application::processInput() {
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

void Application::setupGL() {
    loadGL();
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(90.0f), static_cast<float>(width) / static_cast<float>(height), 0.001f,
                                  1000.0f);
    camera = Camera(glm::vec3(5.0f, 5.0f, 15.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90);
}

void Application::showFPS() {
    double sample = glfwGetTime();
    double delta = sample - lastSample;
    framesSinceSample++;
    if(delta >= 1.0) {
        std::stringstream ss;
        ss << "FPS: " << std::fixed << std::setprecision(1) << (static_cast<double>(framesSinceSample)/delta);
        std::string s = ss.str();
        const char* s_ptr = s.c_str();
        glfwSetWindowTitle(window, s_ptr);
        lastSample = sample;
        framesSinceSample = 0;
    }
}

void Application::render() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto &node_pair : nodes) {
        node_pair.second->updateUBOs(deltaTime);
        node_pair.second->render();
    }
}

void Application::loadScene() {
    vertexBuffers["main"] = createBufferWithData(GL_ARRAY_BUFFER, vertices, sizeof(vertices), GL_STATIC_DRAW);
    vertexBuffers["boids"] = createBufferWithData(GL_ARRAY_BUFFER, nullptr, BOID_NUM * sizeof(Boid),
                                                  GL_DYNAMIC_DRAW);
    elementBuffers["main"] = createBufferWithData(GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices), GL_STATIC_DRAW);

    auto sphere_cloud = generate_sphere_cloud();
    vertexBuffers["sphere_cloud"] = createBufferWithData(GL_ARRAY_BUFFER, sphere_cloud.data(),
                                                         sizeof(glm::vec4) * sphere_cloud.size(),
                                                         GL_STATIC_DRAW);

    auto cube = generateCube();
    elementBuffers["cube"] = createBufferWithData(GL_ARRAY_BUFFER, cube.first.data(),
                                                  sizeof(GLuint) * cube.first.size(), GL_STATIC_DRAW);
    vertexBuffers["cube"] = createBufferWithData(GL_ARRAY_BUFFER, cube.second.data(),
                                                 sizeof(glm::vec4) * cube.second.size(), GL_STATIC_DRAW);

    //ObjLoader:

    bool res = loadOBJ("assets/models/texturedcube.obj", parsedObjects);

    vertexBuffers["obj"] = createBufferWithData(GL_ARRAY_BUFFER, parsedObjects[0].vertices.data(),
                                                sizeof(glm::vec3) * parsedObjects[0].vertices.size(),
                                                GL_STATIC_DRAW);

    elementBuffers["obj"] = createBufferWithData(GL_ELEMENT_ARRAY_BUFFER, parsedObjects[0].vertex_indices.data(),
                                                sizeof(GLuint) * parsedObjects[0].vertex_indices.size(),
                                                 GL_STATIC_DRAW);

    uvBuffers["obj"] = createBufferWithData(GL_ARRAY_BUFFER, parsedObjects[0].uvs.data(),
                                                sizeof(glm::vec2) * parsedObjects[0].uvs.size(),
                                                GL_STATIC_DRAW);

}

void Application::loadShaders() {
    GLuint boids_vsh = compileShader(loadFile("assets/shaders/boids.vert"), GL_VERTEX_SHADER);
    GLuint fsh = compileShader(loadFile("assets/shaders/main.frag"), GL_FRAGMENT_SHADER);
    std::vector<GLuint> boids = {boids_vsh, fsh};
    GLuint boid_program = linkShader(boids);
    shaders["boids"] = boid_program;

    GLuint sphere_cloud_vsh = compileShader(loadFile("assets/shaders/sphere_cloud.vert"), GL_VERTEX_SHADER);
    std::vector<GLuint> sphere_cloud = {sphere_cloud_vsh, fsh};
    shaders["sphere_cloud"] = linkShader(sphere_cloud);

    GLuint main_vsh = compileShader(loadFile("assets/shaders/main.vert"), GL_VERTEX_SHADER);
    std::vector<GLuint> main = {main_vsh, fsh};
    shaders["main"] = linkShader(main);

    glDeleteShader(main_vsh);
    glDeleteShader(sphere_cloud_vsh);
    glDeleteShader(boids_vsh);
    glDeleteShader(fsh);
}

void Application::createNodes() {
    std::vector<VertexInput> input_boids = {
            {
                    0,                      //location
                    4,                      //size
                    GL_FLOAT,               //type
                    4 * sizeof(GLfloat),    //stride
                    vertexBuffers["main"],  //VBO
                    0,                      //offset
                    false,                  //instanced
            },
            {
                    1,                      //location
                    4,                      //size
                    GL_FLOAT,               //type
                    sizeof(Boid),           //stride
                    vertexBuffers["boids"], //VBO
                    offsetof(Boid, pos),    //offset
                    true,                   //instanced
            },
            {
                    2,                      //location
                    4,                      //size
                    GL_FLOAT,               //type
                    sizeof(Boid),           //stride
                    vertexBuffers["boids"], //VBO
                    offsetof(Boid, dir),    //offset
                    true,                   //instanced
            },
    };

    SceneInfo sceneInfo = {
            elementBuffers["cube"],
            vertexBuffers["cube"],
            12,                     //primitiveCount
    };
    SceneInfo samples = {
            0,
            vertexBuffers["sphere_cloud"],
            1000
    };
    Boids boids(elementBuffers["main"], shaders["boids"], std::vector<void*>{&camera, & projection}, input_boids,
        sceneInfo, samples, 4, 6,
        BOID_NUM);
    nodes.insert(std::make_pair("main", std::make_shared<Boids>(boids)));

    std::vector<VertexInput> input_sphere_cloud = {
            {
                    0,                              //location
                    4,                              //size
                    GL_FLOAT,                       //type
                    sizeof(glm::vec4),              //stride
                    vertexBuffers["sphere_cloud"],  //VBO
                    0,                              //offset
                    false,                          //instanced
            }
    };

    SphereCloud sphere(0, shaders["sphere_cloud"], std::vector<void*>{&camera, & projection}, input_sphere_cloud, 1000,
        100, 1);
    //nodes.insert(std::make_pair("sphere_cloud", std::make_shared<SphereCloud>(sphere)));
    
    std::vector<VertexInput> input_cube = {
            {
                    0,                      //location
                    3,                      //size
                    GL_FLOAT,               //type
                    sizeof(glm::vec3),      //stride
                    vertexBuffers["obj"],   //VBO
                    0,                      //offset
                    false,                  //instanced
            }
    };
    cubeModel = glm::mat4(1.0f);
    cubeModel = glm::scale(cubeModel, glm::vec3(1.0f));
    cubeModel = glm::translate(cubeModel, glm::vec3(-2.5f));
    SimpleMesh cube(elementBuffers["obj"], shaders["main"], std::vector<void*>{&camera, & projection, & cubeModel},
        input_cube, parsedObjects[0].vertices.size(), parsedObjects[0].vertex_indices.size(), 1);
    nodes.insert(std::make_pair("obj", std::make_shared<SimpleMesh>(cube)));


}

void Application::mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app->firstMouse) {
        app->lastX = xpos;
        app->lastY = ypos;
        app->firstMouse = false;
    }

    float xoffset = xpos - app->lastX;
    float yoffset = app->lastY - ypos; // reversed since y-coordinates go from bottom to top

    app->lastX = xpos;
    app->lastY = ypos;

    app->camera.ProcessMouseMovement(xoffset, yoffset);
}

void Application::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    app->camera.ProcessMouseScroll(yoffset);
}

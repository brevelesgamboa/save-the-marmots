/*
 * Lab 9: pitch and yaw camera control in a space scene.
 * Based on the course starter by Z. Wood, S. Sueda, and I. Dunn.
 */

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader/tiny_obj_loader.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "Texture.h"
#include "WindowManager.h"

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{
public:
    WindowManager *windowManager = nullptr;
    shared_ptr<Program> prog, texProg;
    shared_ptr<Program> skyProg;
    shared_ptr<Shape> moonRock, cube;
    vector<shared_ptr<Shape>> dummy, dog;
    mat4 dummyTransform, dogTransform;
    shared_ptr<Texture> groundTexture;
    shared_ptr<Texture> skyTexture;
    GLuint skyVAO;

    enum Material { PAINT, ROCK, PAD, GROUND, RUBBER, METAL, SUIT, DOG };
    struct CarPart {
        shared_ptr<Shape> mesh;
        Material material;
    };
    vector<CarPart> car;
    float carGroundOffset = 0.0f;

    GLuint groundVAO, groundPos, groundNor, groundTex, groundIndices;
    // The eye stays at one spot among the meshes; only the viewing direction changes.
    const vec3 cameraPosition = vec3(0.0f, 2.0f, -3.0f);
    const vec3 worldUp = vec3(0.0f, 1.0f, 0.0f); // Positive Y keeps the camera upright.
    // Store angles in radians because the direction formula uses sin and cos.
    float phi = radians(-20.0f);   // Pitch: look up/down, 
    float theta = radians(110.0f); // Yaw: turn left/right
    const float mouseSensitivity = radians(0.1f); // Turn 0.1 degree per mouse pixel.
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool firstMouse = true;
    bool darkPaint = false;
    // Required initial point light, in world coordinates.
    vec3 lightPosition = vec3(-2.0f, 2.0f, 2.0f);

    void keyCallback(GLFWwindow *window, int key, int, int action, int) override
    {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        switch (key) {
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;
            case GLFW_KEY_Q: lightPosition.x -= 0.25f; break;
            case GLFW_KEY_E: lightPosition.x += 0.25f; break;
            case GLFW_KEY_M:
                // Toggle once per press, rather than repeatedly while held.
                if (action == GLFW_PRESS) darkPaint = !darkPaint;
                break;
        }
    }

    void mouseCallback(GLFWwindow *, int, int, int) override {}

    void cursorPosCallback(GLFWwindow *, double x, double y) override
    {
        // Save the first position: there is no previous position to compare yet.
        if (firstMouse) {
            lastMouseX = x;
            lastMouseY = y;
            firstMouse = false;
            return;
        }

        // Compare positions to find how far the mouse moved since the last event.
        float deltaX = static_cast<float>(x - lastMouseX);
        float deltaY = static_cast<float>(lastMouseY - y); // Reverse Y so mouse up means look up.
        // Remember this position for the next mouse event.
        lastMouseX = x;
        lastMouseY = y;

        // Horizontal movement turns the camera; no yaw limit means full rotations.
        theta += deltaX * mouseSensitivity;
        // Stop pitch at +/-80 degrees so the view never points straight up or down.
        phi = glm::clamp(phi + deltaY * mouseSensitivity,
                         radians(-80.0f), radians(80.0f));
    }

    void resizeCallback(GLFWwindow *, int width, int height) override
    {
        glViewport(0, 0, width, height);
    }

    shared_ptr<Program> makeProgram(const string &vertex, const string &fragment,
                                   bool textured)
    {
        auto shader = make_shared<Program>();
        shader->setVerbose(true);
        shader->setShaderNames(vertex, fragment);
        if (!shader->init()) throw runtime_error("Could not initialize scene shaders.");
        for (const char *name : {"P", "V", "M", "MatAmb", "MatDif", "MatSpec",
                                  "MatShine", "lightPos", "lightColor"}) {
            shader->addUniform(name);
        }
        shader->addAttribute("vertPos");
        shader->addAttribute("vertNor");
        if (textured) {
            shader->addUniform("Texture0");
            shader->addAttribute("vertTex");
        }
        return shader;
    }

    void init(const string &resources)
    {
        GLSL::checkVersion();
        glClearColor(0.025f, 0.03f, 0.055f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        // Keep the car's reversed outline shell from covering its painted faces.
        glEnable(GL_CULL_FACE);
        prog = makeProgram(resources + "/simple_vert.glsl",
                           resources + "/simple_frag.glsl", false);
        texProg = makeProgram(resources + "/tex_vert.glsl",
                              resources + "/tex_frag0.glsl", true);

        groundTexture = make_shared<Texture>();
        groundTexture->setFilename(resources + "/moonDust.png");
        groundTexture->init();
        groundTexture->setUnit(0);
        groundTexture->setWrapModes(GL_REPEAT, GL_REPEAT);

        skyProg = make_shared<Program>();
        skyProg->setShaderNames(resources + "/sky_vert.glsl", resources + "/sky_frag.glsl");
        if (!skyProg->init()) throw runtime_error("Could not initialize sky shaders.");
        skyProg->addUniform("inversePV");
        skyProg->addUniform("Texture0");
        glGenVertexArrays(1, &skyVAO);

        skyTexture = make_shared<Texture>();
        skyTexture->setFilename(resources + "/Blue Nebula 6 - 1024x1024.png");
        skyTexture->init();
        skyTexture->setUnit(0);
        skyTexture->setWrapModes(GL_REPEAT, GL_REPEAT);
    }

    shared_ptr<Shape> makeShape(tinyobj::shape_t &source)
    {
        auto mesh = make_shared<Shape>();
        mesh->createShape(source); // Also generates normals if the OBJ has none.
        mesh->measure();
        mesh->init();
        return mesh;
    }

    vector<tinyobj::shape_t> loadObj(const string &resources, const string &filename,
                                   vector<tinyobj::material_t> &materials)
    {
        vector<tinyobj::shape_t> shapes;
        string error;
        string materialDirectory = resources + "/";
        bool loaded = tinyobj::LoadObj(shapes, materials, error,
                                      (resources + "/" + filename).c_str(),
                                      materialDirectory.c_str());
        if (!loaded || shapes.empty()) {
            throw runtime_error("Could not load " + filename + ": " + error);
        }
        if (!error.empty()) cerr << error << endl;
        return shapes;
    }

    mat4 loadProp(const string &resources, const string &filename,
                  vector<shared_ptr<Shape>> &parts, float height, bool zUp)
    {
        vector<tinyobj::material_t> materials;
        auto shapes = loadObj(resources, filename, materials);
        vec3 minimum(numeric_limits<float>::max());
        vec3 maximum(-numeric_limits<float>::max());
        // Measure all parts together so the dummy's limbs stay in place.
        for (auto &source : shapes) {
            if (source.mesh.indices.empty()) continue;
            source.mesh.texcoords.clear();
            auto mesh = makeShape(source);
            minimum = glm::min(minimum, mesh->min);
            maximum = glm::max(maximum, mesh->max);
            parts.push_back(mesh);
        }
        int upAxis = zUp ? 2 : 1;
        float sourceHeight = maximum[upAxis] - minimum[upAxis];
        if (parts.empty() || sourceHeight <= 0.0f) {
            throw runtime_error("Invalid model dimensions: " + filename);
        }
        // Center at the feet and use a world-space height, regardless of export units.
        vec3 bottomCenter = 0.5f * (minimum + maximum);
        bottomCenter[upAxis] = minimum[upAxis];
        mat4 transform = scale(mat4(1.0f), vec3(height / sourceHeight));
        transform = translate(transform, -bottomCenter);
        // The starter dummy uses Z as up; the scene uses Y as up.
        if (zUp) transform = rotate(mat4(1.0f), radians(-90.0f), vec3(1, 0, 0)) * transform;
        return transform;
    }

    void initGeom(const string &resources)
    {
        vector<tinyobj::material_t> materials;
        auto shapes = loadObj(resources, "moonRockNoNormals.obj", materials);
        moonRock = makeShape(shapes[0]);
        shapes = loadObj(resources, "cube.obj", materials);
        cube = makeShape(shapes[0]);

        shapes = loadObj(resources, "hondafit.obj", materials);
        float lowestPoint = numeric_limits<float>::max();
        // The loader splits this car by object and material; keep every part.
        for (auto &source : shapes) {
            if (source.mesh.indices.empty()) continue;
            // The car uses solid materials; only the ground needs a texture.
            source.mesh.texcoords.clear();
            auto mesh = makeShape(source);
            lowestPoint = std::min(lowestPoint, mesh->min.y);
            Material material = RUBBER;
            int id = source.mesh.material_ids.empty() ? -1 : source.mesh.material_ids[0];
            if (id >= 0 && id < static_cast<int>(materials.size())) {
                const string &name = materials[id].name;
                if (name == "Body_Color" || name == "body.019") material = PAINT;
                else if (name == "blur_rim" || name == "inner_rim") material = METAL;
            }
            car.push_back({mesh, material});
        }
        if (car.empty()) throw runtime_error("The car OBJ contains no triangles.");
        carGroundOffset = -lowestPoint;
        dummyTransform = loadProp(resources, "dummy.obj", dummy, 2.2f, true);
        dogTransform = loadProp(resources, "dogNoNormals.obj", dog, 1.2f, false);
        initGround();
    }

    void initGround()
    {
        // A 40-by-40 ground plane. More repeats keep the dust texture from stretching.
        const float positions[] = {-20, 0, -20, -20, 0, 20, 20, 0, 20, 20, 0, -20};
        const float normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
        const float texcoords[] = {0, 0, 0, 12, 12, 12, 12, 0};
        const unsigned short indices[] = {0, 1, 2, 0, 2, 3};
        glGenVertexArrays(1, &groundVAO);
        glBindVertexArray(groundVAO);

        glGenBuffers(1, &groundPos);
        glBindBuffer(GL_ARRAY_BUFFER, groundPos);
        glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glGenBuffers(1, &groundNor);
        glBindBuffer(GL_ARRAY_BUFFER, groundNor);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glGenBuffers(1, &groundTex);
        glBindBuffer(GL_ARRAY_BUFFER, groundTex);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        glGenBuffers(1, &groundIndices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundIndices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glBindVertexArray(0);
    }

    void setMaterial(const shared_ptr<Program> &shader, Material material)
    {
        vec3 ambient, diffuse, specular;
        float shine;
        switch (material) {
            case PAINT: // Glossy blue paint; M switches its shade.
                ambient = darkPaint ? vec3(0.015f, 0.025f, 0.06f) : vec3(0.035f, 0.065f, 0.12f);
                diffuse = darkPaint ? vec3(0.025f, 0.09f, 0.3f) : vec3(0.08f, 0.32f, 0.8f);
                specular = vec3(0.8f); shine = 64.0f;
                break;
            case ROCK: // Dull rock with a broad, faint highlight.
                ambient = vec3(0.08f, 0.075f, 0.07f); diffuse = vec3(0.38f, 0.35f, 0.32f);
                specular = vec3(0.035f); shine = 4.0f;
                break;
            case PAD: // Orange painted landing pad.
                ambient = vec3(0.13f, 0.065f, 0.018f); diffuse = vec3(0.9f, 0.36f, 0.06f);
                specular = vec3(0.35f, 0.27f, 0.16f); shine = 24.0f;
                break;
            case GROUND:
                ambient = vec3(0.18f); diffuse = vec3(0.7f);
                specular = vec3(0.025f); shine = 4.0f;
                break;
            case RUBBER:
                ambient = vec3(0.028f); diffuse = vec3(0.055f);
                specular = vec3(0.08f); shine = 8.0f;
                break;
            case METAL:
                ambient = vec3(0.12f); diffuse = vec3(0.55f);
                specular = vec3(0.8f); shine = 80.0f;
                break;
            case SUIT: // White astronaut stand-in.
                ambient = vec3(0.16f, 0.17f, 0.19f); diffuse = vec3(0.72f, 0.76f, 0.82f);
                specular = vec3(0.25f); shine = 24.0f;
                break;
            case DOG:
                ambient = vec3(0.14f, 0.085f, 0.04f); diffuse = vec3(0.62f, 0.34f, 0.13f);
                specular = vec3(0.08f); shine = 8.0f;
                break;
        }
        glUniform3fv(shader->getUniform("MatAmb"), 1, value_ptr(ambient));
        glUniform3fv(shader->getUniform("MatDif"), 1, value_ptr(diffuse));
        glUniform3fv(shader->getUniform("MatSpec"), 1, value_ptr(specular));
        glUniform1f(shader->getUniform("MatShine"), shine);
    }

    void setFrame(const shared_ptr<Program> &shader, const mat4 &projection,
                  const mat4 &view)
    {
        shader->bind();
        glUniformMatrix4fv(shader->getUniform("P"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(view));
        glUniform3fv(shader->getUniform("lightPos"), 1, value_ptr(lightPosition));
        glUniform3f(shader->getUniform("lightColor"), 1.0f, 1.0f, 1.0f);
    }

    void drawMesh(const shared_ptr<Shape> &mesh, const mat4 &model, Material material)
    {
        setMaterial(prog, material);
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(model));
        mesh->draw(prog);
    }

    void drawSky(const mat4 &projection, const mat4 &view)
    {
        // Ignore translation so the sky stays distant while the view rotates.
        mat4 inversePV = inverse(projection * mat4(mat3(view)));
        skyProg->bind();
        glUniformMatrix4fv(skyProg->getUniform("inversePV"), 1, GL_FALSE, value_ptr(inversePV));
        skyTexture->bind(skyProg->getUniform("Texture0"));
        // Draw at the far depth without writing into the scene's depth buffer.
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        skyTexture->unbind();
        skyProg->unbind();
    }

    void render()
    {
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        if (width == 0 || height == 0) return;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 projection = perspective(radians(45.0f), width / float(height), 0.1f, 100.0f);
        // Turn pitch and yaw into a direction of length 1 
        // theta is yaw and phi is pitch in radians
        vec3 cameraDirection;
        cameraDirection.x = cos(theta) * cos(phi);
        cameraDirection.y = sin(phi);
        cameraDirection.z = sin(theta) * cos(phi);
        // lookAt needs a point in the world, so add the direction to the fixed eye
        vec3 lookAtPoint = cameraPosition + cameraDirection;
        mat4 view = lookAt(cameraPosition, lookAtPoint, worldUp);
        drawSky(projection, view);
        setFrame(prog, projection, view); // Send the camera view to the mesh shader

        mat4 model = translate(mat4(1.0f), vec3(-1.5f, carGroundOffset, 1.5f));
        for (const auto &part : car) drawMesh(part.mesh, model, part.material);

        model = translate(mat4(1.0f), vec3(-7.0f, 1.1f, -3.0f));
        model = scale(model, vec3(1.4f, 1.1f, 1.2f));
        drawMesh(moonRock, model, ROCK);

        model = translate(mat4(1.0f), vec3(5.0f, 0.25f, -4.0f));
        model = scale(model, vec3(3.6f, 0.5f, 3.6f));
        drawMesh(cube, model, PAD);

        model = translate(mat4(1.0f), vec3(4.0f, 0.0f, 2.0f));
        model = rotate(model, radians(-120.0f), vec3(0, 1, 0)) * dummyTransform;
        for (const auto &part : dummy) drawMesh(part, model, SUIT);

        model = translate(mat4(1.0f), vec3(1.0f, 0.0f, 4.2f));
        model = rotate(model, radians(-35.0f), vec3(0, 1, 0)) * dogTransform;
        for (const auto &part : dog) drawMesh(part, model, DOG);
        prog->unbind();

        // Textured geometry uses its own shader, with the same lighting model.
        setFrame(texProg, projection, view);
        setMaterial(texProg, GROUND);
        model = mat4(1.0f);
        glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(model));
        groundTexture->bind(texProg->getUniform("Texture0"));
        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);
        groundTexture->unbind();
        texProg->unbind();
    }
};

int main(int argc, char *argv[])
{
    // Run from the project root, or pass a different resource directory.
    string resources = argc > 1 ? argv[1] : "resources";
    WindowManager windowManager;
    if (!windowManager.init(1280, 800)) return 1;

    int status = 0;
    {
        Application application;
        application.windowManager = &windowManager;
        windowManager.setEventCallbacks(&application);
        // Hide and capture the cursor so looking around continues past window edges.
        glfwSetInputMode(windowManager.getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        try {
            application.init(resources);
            application.initGeom(resources);
            while (!glfwWindowShouldClose(windowManager.getHandle())) {
                application.render();
                glfwSwapBuffers(windowManager.getHandle());
                glfwPollEvents();
            }
        } catch (const exception &error) {
            cerr << error.what() << endl;
            status = 1;
        }
    }
    windowManager.shutdown();
    return status;
}

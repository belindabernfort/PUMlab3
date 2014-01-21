/**************************************************************************************************
 *                                                                                                *
 * TNM090 Particle System                                                                         *
 *                                                                                                *
 * Copyright (c) 2013 Alexander Bock                                                              *
 *                                                                                                *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software  *
 * and associated documentation files (the "Software"), to deal in the Software without           *
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the  *
 * Software is furnished to do so, subject to the following conditions:                           *
 *                                                                                                *
 * The above copyright notice and this permission notice shall be included in all copies or       *
 * substantial portions of the Software.                                                          *
 *                                                                                                *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING  *
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND     *
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.        *
 *                                                                                                *
 *************************************************************************************************/

#include "renderer.h"

#include <ghoul/filesystem/filesystem>
#include <ghoul/logging/logging>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <QGLFormat>
#include <QMouseEvent>
#include <chrono>

using std::chrono::high_resolution_clock;

using namespace ghoul::opengl;

namespace {
    // Category used for the logging mechanism to print out debug/error messages
    const std::string _loggerCat = "Renderer";

    // Skybox size
    const float _skyboxSize = 5.f;
    
    // Linear scaling factor for the rotational part of the interaction
    const float _rotationalFactor = 60.f;
    // Minimum height for the camera to not pass though the ground texture
    const float _minimumHeight = 0.1f;
    // Minimum distance of the camera from the focus point
    const float _minimumDistance = 0.25f;
    // Maximum distance of the camera to the focus point
    const float _maximumDistance = _skyboxSize - 0.1f;
    // The minimum tilt in [0,pi] that is allowed
    const float _minimumTilt = 0.1f;
    // The maximum tilt in [0,pi] that is allowed
    const float _maximumTilt = glm::pi<float>() - _minimumTilt;

    // Default camera parameters
    const glm::vec3 _defaultPosition = glm::vec3(-1.f, 0.f, 1.f);
    const glm::vec3 _defaultFocus = glm::vec3(0.f);
    const glm::vec3 _defaultUp = glm::vec3(0.f, 0.f, 1.f);
    const float _fieldOfView = 45.f; // in degrees
    const float _nearPlane = 0.1f;
    const float _farPlane = 500.f;
    const glm::vec3 _defaultLightPosition = glm::vec3(0.f, 2.f, 10.f);
}

Renderer::Renderer(const QGLFormat& format, QWidget* parent, Qt::WindowFlags f)
    : QGLWidget(format, parent, nullptr, f)
    , _limitCameraPosition(true)
    , _particleData(nullptr)
    , _renderGround(true)
    , _groundVBO(0)
    , _groundTexture(nullptr)
    , _groundTextureNormal(nullptr)
    , _groundProgram(nullptr)
    , _groundProgramReady(false)
    , _renderSkybox(true)
    , _skyboxVBO(0)
    , _skyboxIBO(0)
    , _skyboxTexture(0)
    , _skyboxProgram(nullptr)
    , _skyboxProgramReady(false)
    , _particleVBO(0)
    , _particleTexture(nullptr)
    , _particleProgram(nullptr)
    , _particleProgramReady(false)
    , _numberOfParticles(0)
{}

Renderer::~Renderer() {
    // we don't own _particleData, so we don't delete it
    _particleData = nullptr;

    glDeleteBuffers(1, &_groundVBO);
    delete _groundTexture;
    delete _groundTextureNormal;
    delete _groundProgram;
    _groundProgramReady = false;

    glDeleteBuffers(1, &_skyboxVBO);
    glDeleteBuffers(1, &_skyboxIBO);
    glDeleteTextures(1, &_skyboxTexture);
    delete _skyboxProgram;
    _skyboxProgramReady = false;

    glDeleteBuffers(1, &_particleVBO);
    delete _particleTexture;
    delete _particleProgram;
    _particleProgramReady = false;
}

void Renderer::initializeGL() {
    QGLWidget::initializeGL();

    GLenum res = glewInit();
    if (res != GLEW_OK) {
        LFATAL("GLEW initialization failed");
        return;
    }

    // Set the OpenGL state as we want it
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize the textures, Vertex Buffer Objects, Index Buffer Objects and ProgramObjects
    initializeGround();
    initializeSkybox();
    initializeParticle();

    // Initialize the default camera and light position
    _position = _defaultPosition;
    _focus = _defaultFocus;
    _upVector = _defaultUp;
    _lightPosition = _defaultLightPosition;

    // Create the new view-projection matrix
    updateViewProjectionMatrix();
}

void Renderer::initializeGround() {
    // Generate the VBO for the ground quad
    generateGroundBuffer();

    // loadTexture will return a nullptr if the texture could not be loaded
    _groundTexture = loadTexture(FileSys.absolutePath("${ASSETS}/dirt.jpg"));
    if (_groundTexture != nullptr) {
        _groundTexture->enable();
        _groundTexture->bind();
        _groundTexture->uploadTexture();
    }

    // loadTexture will return a nullptr if the texture could not be loaded
    _groundTextureNormal = loadTexture(FileSys.absolutePath("${ASSETS}/dirt_n.jpg"));
    if (_groundTextureNormal != nullptr) {
        _groundTextureNormal->enable();
        _groundTextureNormal->bind();
        _groundTextureNormal->uploadTexture();
    }

    // Generate the ProgramObject that holds the ShaderObjects used to render the ground
    // _groundProgramReady is true if both the compiling and linking succeeded; errors that occur
    // during either step will be written to the Logmanager by the ProgramObject and ShaderObject
    _groundProgram = new ProgramObject("Ground");
    _groundProgram->attachObject(new ShaderObject(ShaderObject::ShaderTypeVertex, FileSys.absolutePath("${ASSETS}/ground.vert")));
    _groundProgram->attachObject(new ShaderObject(ShaderObject::ShaderTypeFragment, FileSys.absolutePath("${ASSETS}/ground.frag")));
    bool groundCompileSuccess = _groundProgram->compileShaderObjects();
    if (groundCompileSuccess) {
        bool linkSuccess = _groundProgram->linkProgramObject();
        _groundProgramReady = linkSuccess;
    }
}

void Renderer::initializeSkybox() {
    // Generate the VBO and IBO for the skybox
    generateSkyboxBuffer();

    // Create the ProgramObject that holds the ShaderObjects used to render the skybox
    // _skyboxProgramReady is true if both the compiling and linking succeeded; errors that occur
    // during either step will be written to the Logmanager by the ProgramObject and ShaderObject
    _skyboxProgram = new ProgramObject("Skybox");
    _skyboxProgram->attachObject(new ShaderObject(ShaderObject::ShaderTypeVertex, FileSys.absolutePath("${ASSETS}/skybox.vert")));
    _skyboxProgram->attachObject(new ShaderObject(ShaderObject::ShaderTypeFragment, FileSys.absolutePath("${ASSETS}/skybox.frag")));
    bool skyboxCompileSuccess = _skyboxProgram->compileShaderObjects();
    if (skyboxCompileSuccess) {
        bool linkSuccess = _skyboxProgram->linkProgramObject();
        _skyboxProgramReady = linkSuccess;
    }

    // This is a bit uglier as Ghoul does not support loading cubemaps (yet)
    glEnable(GL_TEXTURE_CUBE_MAP);
    glGenTextures(1, &_skyboxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _skyboxTexture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Take a little detour through Ghoul to use DevIL library to load textures
    // Load the textures and assign the data pointer of each texture to be used as the
    // corresponding face texture
    Texture* xPos = loadTexture(FileSys.absolutePath("${ASSETS}/xpos.png"));
    Texture* xNeg = loadTexture(FileSys.absolutePath("${ASSETS}/xneg.png"));
    Texture* yPos = loadTexture(FileSys.absolutePath("${ASSETS}/ypos.png"));
    Texture* yNeg = loadTexture(FileSys.absolutePath("${ASSETS}/yneg.png"));
    Texture* zPos = loadTexture(FileSys.absolutePath("${ASSETS}/zpos.png"));
    Texture* zNeg = loadTexture(FileSys.absolutePath("${ASSETS}/zneg.png"));
    if ((xPos == nullptr) || (xNeg == nullptr) || (yPos == nullptr) || (yNeg == nullptr) ||
        (zPos == nullptr) || (zNeg == nullptr))
    {
        // If any of the texture is not loadable, delete all (it's safe to delete a nullptr)
        // and abort. 'loadTexture' will write an error message if it fails
        delete xPos;
        delete xNeg;
        delete yPos;
        delete yNeg;
        delete zPos;
        delete zNeg;
        return;
    }

    // create and upload the six faces of the cubemap
    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        0,
        GL_RGBA,
        static_cast<GLsizei>(xPos->width()),
        static_cast<GLsizei>(xPos->height()),
        0,
        xPos->format(),
        GL_UNSIGNED_BYTE,
        xPos->pixelData());

    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        0,
        GL_RGBA,
        static_cast<GLsizei>(xNeg->width()),
        static_cast<GLsizei>(xNeg->height()),
        0,
        xNeg->format(),
        GL_UNSIGNED_BYTE,
        xNeg->pixelData());
    
    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        0,
        GL_RGBA,
        static_cast<GLsizei>(yPos->width()),
        static_cast<GLsizei>(yPos->height()),
        0, yPos->format(),
        GL_UNSIGNED_BYTE,
        yPos->pixelData());

    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        0,
        GL_RGBA,
        static_cast<GLsizei>(yNeg->width()),
        static_cast<GLsizei>(yNeg->height()),
        0, yNeg->format(),
        GL_UNSIGNED_BYTE,
        yNeg->pixelData());

    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        0,
        GL_RGBA,
        static_cast<GLsizei>(zPos->width()),
        static_cast<GLsizei>(zPos->height()),
        0, zPos->format(),
        GL_UNSIGNED_BYTE,
        zPos->pixelData());

    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
        0,
        GL_RGBA,
        static_cast<GLsizei>(zNeg->width()),
        static_cast<GLsizei>(zNeg->height()),
        0, zNeg->format(),
        GL_UNSIGNED_BYTE,
        zNeg->pixelData());

    // Now OpenGL has taken a copy of the data in the textures and we can delete our versions
    delete xPos;
    delete xNeg;
    delete yPos;
    delete yNeg;
    delete zPos;
    delete zNeg;
}

void Renderer::initializeParticle() {
    // loadTexture will return a nullptr if the texture could not be loaded
    _particleTexture = loadTexture(FileSys.absolutePath("${ASSETS}/particle.png"));
    if (_particleTexture != nullptr) {
        _particleTexture->enable();
        _particleTexture->bind();
        _particleTexture->uploadTexture();
    }

    // Create the ProgramObject that holds the ShaderObjects used to render the skybox
    // _particleProgramReady is true if both the compiling and linking succeeded; errors that occur
    // during either step will be written to the Logmanager by the ProgramObject and ShaderObject
    _particleProgram = new ProgramObject("Particle");
    _particleProgram->attachObject(new ShaderObject(ShaderObject::ShaderTypeVertex, FileSys.absolutePath("${ASSETS}/particle.vert")));
    _particleProgram->attachObject(new ShaderObject(ShaderObject::ShaderTypeFragment, FileSys.absolutePath("${ASSETS}/particle.frag")));
    bool particleCompileSuccess = _particleProgram->compileShaderObjects();
    if (particleCompileSuccess) {
        bool linkSuccess = _particleProgram->linkProgramObject();
        _particleProgramReady = linkSuccess;
    }
}

void Renderer::resizeGL(int width, int height) {
    // A resize event is not expected for this program, but just to be sure
    glViewport(0, 0, width, height);
    updateViewProjectionMatrix();
}

bool Renderer::groundIsReady() const {
    return ((_groundVBO != 0) && (_groundProgram != nullptr) &&
        _groundProgramReady && (_groundTexture != nullptr) && (_groundTextureNormal != nullptr));
}

bool Renderer::skyboxIsReady() const {
    return ((_skyboxVBO != 0) && (_skyboxIBO != 0) &&
        (_skyboxProgram != nullptr) && _skyboxProgramReady && (_skyboxTexture != 0));
}

bool Renderer::particlesAreReady() const {
    return ((_particleVBO != 0) && (_particleProgram != nullptr) &&
        _particleProgramReady);
}

void Renderer::paintGL() {
#ifdef PERFORMANCE_MEASUREMENTS
    glFinish();
    high_resolution_clock::time_point t0 = high_resolution_clock::now();
#endif
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (_renderGround && groundIsReady())
        drawGround();
    if (_renderSkybox && skyboxIsReady())
        drawSkybox();

    if (particlesAreReady())
        drawParticles();
#ifdef PERFORMANCE_MEASUREMENTS
    //glFlush();
    glFinish();
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    LINFO(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
#endif
}

void Renderer::drawGround() {
    // Active the ProgramObject
    _groundProgram->activate();

    // Bind the ground texture in the first available texture unit
    TextureUnit groundTextureUnit;
    groundTextureUnit.activate();
    _groundTexture->enable();
    _groundTexture->bind();

    // Bind the normal texture in the next free texture unit
    TextureUnit groundTextureNormalUnit;
    groundTextureNormalUnit.activate();
    _groundTextureNormal->enable();
    _groundTextureNormal->bind();

    // We are using 'fragColor' as the output variable from the FragmentShader
    _groundProgram->bindFragDataLocation("fragColor", 0);

    // Enable and bind the VBO holding the vertices for the ground and assign them a location
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _groundVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    _groundProgram->bindAttributeLocation("in_position", _groundVBO);
    
    // Set the rest of the uniforms
    // It would be faster to cache the uniform location and reuse that, but this is more readable
    _groundProgram->setUniform("_viewProjectionMatrix", _viewProjectionMatrix);
    _groundProgram->setUniform("_cameraPosition", _position);
    _groundProgram->setUniform("_lightPosition", _lightPosition);
    _groundProgram->setUniform("_texture", groundTextureUnit.unitNumber());
    _groundProgram->setUniform("_textureNormal", groundTextureNormalUnit.unitNumber());

    // Draw one quad 
    glDrawArrays(GL_QUADS, 0, 4);

    // And disable everything again to be a good citizen
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    _groundTexture->disable();
    _groundTextureNormal->disable();
    _groundProgram->deactivate();
}

void Renderer::drawSkybox() {
    // Active the ProgramObject
    _skyboxProgram->activate();

    // Bind the cube map texture into the first texture unit
    TextureUnit cubeMapTextureUnit;
    cubeMapTextureUnit.activate();
    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _skyboxTexture);

    // We are using 'fragColor' as the output variable from the FragmentShader
    _skyboxProgram->bindFragDataLocation("fragColor", 0);

    // Enable and bind the VBO holding the vertices for the ground and assign them a location
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _skyboxVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    _skyboxProgram->bindAttributeLocation("in_position", _skyboxVBO);

    // Use _skyboxIBO as our element array buffer to handle the indexed rendering
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _skyboxIBO);

    // Set the rest of the uniforms
    // It would be faster to cache the uniform location and reuse that, but this is more readable
    _skyboxProgram->setUniform("_viewProjectionMatrix", _viewProjectionMatrix);
    _skyboxProgram->setUniform("_texture", cubeMapTextureUnit.unitNumber());

    // Render the 4 quads
    glDrawElements(GL_QUADS, _numSkyboxIndices, GL_UNSIGNED_SHORT, 0);

    // And disable everything again to be a good citizen
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisable(GL_TEXTURE_CUBE_MAP);
    _skyboxProgram->deactivate();
}

void Renderer::drawParticles() {
    // We want to be able to set the point size from the shader
    // and let OpenGL generate texture coordinates for each point
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE); // Deprecated in OpenGL 3.2, but necessary

    // Activate the ProgramObject
    _particleProgram->activate();

    // Bind the only one texture that is used as the color and normal texture
    TextureUnit textureUnit;
    textureUnit.activate();
    _particleTexture->enable();
    _particleTexture->bind();

    // We are using 'fragColor' as the output variable from the FragmentShader
    _particleProgram->bindFragDataLocation("fragColor", 0);

    // Enable and bind the VBO holding the vertices for the ground and assign them a location
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _particleVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, 0);
    _particleProgram->bindAttributeLocation("in_position", _particleVBO);

    // Set the rest of the uniforms
    // It would be faster to cache the uniform location and reuse that, but this is more readable
    _particleProgram->setUniform("_viewProjectionMatrix", _viewProjectionMatrix);
    _particleProgram->setUniform("_cameraPosition", _position);
    _particleProgram->setUniform("_lightPosition", _lightPosition);
    _particleProgram->setUniform("_texture", textureUnit.unitNumber());

    // _particleData holds the xyz coordinates, so it has thrice the amount of data
    // than it has points. And since we test for the correct amount of data elsewhere,
    // it is safe to assume that everything is fine
    glDrawArrays(GL_POINTS, 0, _numberOfParticles);

    // Be a good citizen and disable everything again
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    _particleTexture->disable();
    _particleProgram->deactivate();
    glDisable(GL_POINT_SPRITE);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void Renderer::mousePressEvent(QMouseEvent* event) {
    // Just store the current mouse position
    _oldMousePosition = scaledMouse(glm::ivec2(event->x(), event->y()));

    // Qt should not continue processing this event after we are done with it
    event->ignore();
}

void Renderer::mouseMoveEvent(QMouseEvent* event) {
    // Where are we now?
    glm::vec2 newMouse = scaledMouse(glm::ivec2(event->x(), event->y()));

    // Left mouse button -> rotation/tilt
    // Right mouse button -> zooming
    if (event->buttons() == Qt::LeftButton)
        rotate(newMouse);
    else if (event->buttons() == Qt::RightButton)
        zoom(newMouse);

    // After processing, this will be our new old position
    _oldMousePosition = newMouse;

    // Qt should not continue processing this event after we are done with it
    event->ignore();
}

void Renderer::updateViewProjectionMatrix() {
    // Create a new view matrix out of (position, focus, upVector)
    glm::mat4 viewMatrix = glm::lookAt(_position, _focus, _upVector);

    // Create a new projection matrix. Technically, this part will never change
    // as we have a fixed size, but you can't be too sure (and this is not our bottleneck)
    const QSize canvasSize = size();
    glm::mat4 projectionMatrix = glm::perspectiveFov(_fieldOfView,
        static_cast<float>(canvasSize.width()), static_cast<float>(canvasSize.height()),
        _nearPlane, _farPlane);

    // Multiply them for ready-usage in the shaders
    _viewProjectionMatrix = projectionMatrix * viewMatrix;
}

glm::vec2 Renderer::scaledMouse(const glm::ivec2& mousePos) const {
    // mousePos will be in window coordinates, but we want [-1,1]
    const QSize canvasSize = size();
    return glm::vec2(
        static_cast<float>(mousePos.x*2.f) / static_cast<float>(canvasSize.width()) - 1.f,
        static_cast<float>(mousePos.y*2.f) / static_cast<float>(canvasSize.height() - 1.f)
        );
}

void Renderer::rotate(const glm::vec2& newMouse) {
    // Don't do anything if the mouse position hasn't changed
    if (newMouse == _oldMousePosition)
        return;

    // Get the displacement between the old and the new position, scaled by the rotational factor
    glm::vec2 t = (_oldMousePosition - newMouse) * _rotationalFactor;
    
    // limit the maximum change per frame to |t| = 1 -> |phi| = pi
    t = glm::min(t, glm::vec2(1.f)); 
    t = glm::max(t, glm::vec2(-1.f));
    
    // phi will be in [-pi,pi]
    const glm::vec2 phi = 2.f * glm::asin(t);

    // The up vector is const (0,0,1)
    glm::quat rotationQuat = glm::angleAxis(phi.x, _upVector);

    // We don't want _position to coincide with _upVector, since the _upVector x _position would
    // be numerically unstable in that case
    const float currentTilt = glm::acos(glm::dot(glm::normalize(_upVector), glm::normalize(_position)));
    const bool closeToTop = ((currentTilt < _minimumTilt) && (phi.y < 0.f));
    const bool closeToBottom = ((currentTilt >  _maximumTilt) && (phi.y > 0.f));
    if (closeToBottom || closeToTop)
        return;

    // Using the cross product between the up vector and the current position always results in a
    // vector perpendicular to the view direction to enable the tilting.
    glm::quat tiltQuat = glm::angleAxis(phi.y, glm::normalize(glm::cross(_upVector, _position)));
    
    // The order of the quaternions are not important
    _position = tiltQuat * rotationQuat * _position;

    if (_limitCameraPosition) {
        // We don't want the camera to pass through the ground
        _position.z = glm::max(_position.z, _minimumHeight);
    }

    // Create the new view-projection matrix
    updateViewProjectionMatrix();
}

void Renderer::zoom(const glm::vec2& newMouse) {
    // Don't do anything if the mouse position hasn't changed
    if (newMouse == _oldMousePosition)
        return;

    // Get the displacement between the old and the new position. As opposed to the rotation,
    // no scaling factor is needed here as we only consider a pi/2 tilting on 600 pixels instead
    // of a 2pi rotation on 800 pixels
    const float t = (newMouse.y - _oldMousePosition.y);

    // The vector pointing from the focus to the current position
    const glm::vec3 positionVector = _position - _focus;

    // Displace the current position along this vector by the factor of t
    _position = (1 + t) * positionVector;

    if (_limitCameraPosition) {
        // If the potential new position would be outside the maximum distance, we want to move only
        // up to the maximal allowed distance
        if ((glm::length(_position) > _maximumDistance))
            _position = _maximumDistance * glm::normalize(positionVector);

        // If the potential new position would be outside the minimum distance, we want to move only
        // up to the minimal allowed distance
        if ((glm::length(_position) < _minimumDistance))
            _position = _minimumDistance * glm::normalize(positionVector);

        // We don't want the camera to pass through the ground
        _position.z = glm::max(_position.z, _minimumHeight);
    }
    // Create the new view-projection matrix
    updateViewProjectionMatrix();
}

void Renderer::setData(const std::vector<glm::vec3>* particleData) {
    // Update the data for the particles. We don't own any of the data, so no delete is necessary
    _particleData = particleData;
}

void Renderer::updateData() {
    // Don't do anything if there isn't any data available
    if (_particleData == 0 || _particleData->size() == 0) {
        _numberOfParticles = 0;
        return;
    }

    // If there is no buffer object, create a new one
    if (_particleVBO == 0)
        glGenBuffers(1, &_particleVBO);

    // Assign the data in _particleData to the _particleVBO
    // GL_STREAM_DRAW signals to OpenGL that the data will change a lot
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _particleVBO);
    glBufferData(GL_ARRAY_BUFFER, _particleData->size() * 3 * sizeof(float), &(_particleData->at(0)), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    _numberOfParticles = static_cast<GLsizei>(_particleData->size());
}

void Renderer::generateGroundBuffer() {
    // If there is no buffer object, create a new one
    if (_groundVBO == 0)
        glGenBuffers(1, &_groundVBO);

    // Fill the ground buffer with static vertices that will not change
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _groundVBO);

    //         3-----------2     
    //        /           /         y
    //       /     o     /         /
    //      /           /         /
    //     0-----------1         o----->x
    //
    // grid[i + 0] = x
    // grid[i + 1] = y
    // grid[i + 2] = z

    GLfloat vertices[] = {
        -_skyboxSize, -_skyboxSize, 0.f, // 0
         _skyboxSize, -_skyboxSize, 0.f, // 1
         _skyboxSize,  _skyboxSize, 0.f, // 2
        -_skyboxSize,  _skyboxSize, 0.f, // 3
    };

    glBufferData(GL_ARRAY_BUFFER, 3 * 4 * sizeof(float), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

void Renderer::generateSkyboxBuffer() {
    // If there is no buffer object, create a new one
    if (_skyboxVBO == 0)
        glGenBuffers(1, &_skyboxVBO);

    // Fill the skybox vertex buffer with the 8 vertices of the cube
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _skyboxVBO);
    
    //         0---------3
    //        /|        /|
    //       / |       / |
    //      1--+------2  |              z
    //      |  4------+--7              |  y
    //      | /       | /               | /
    //      |/        |/                |/
    //      5---------6                 o----->x
    //

    GLfloat vertices[] = {
        -_skyboxSize,  _skyboxSize,  _skyboxSize, // 0
        -_skyboxSize, -_skyboxSize,  _skyboxSize, // 1
         _skyboxSize, -_skyboxSize,  _skyboxSize, // 2
         _skyboxSize,  _skyboxSize,  _skyboxSize, // 3
        -_skyboxSize,  _skyboxSize, -_skyboxSize, // 4
        -_skyboxSize, -_skyboxSize, -_skyboxSize, // 5
         _skyboxSize, -_skyboxSize, -_skyboxSize, // 6
         _skyboxSize,  _skyboxSize, -_skyboxSize, // 7
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Create the index buffer object, if it doesn't already exist
    if (_skyboxIBO == 0)
        glGenBuffers(1, &_skyboxIBO);

    // Fill the indices with the 6 face that we want to render
    // Same indices as in the drawing above
    GLushort indices[] = {
        0, 1, 2, 3, // top
        3, 2, 6, 7, // right
        7, 6, 5, 4, // bottom
        4, 5, 1, 0, // left
        0, 3, 7, 4, // back
        1, 2, 6, 5, // front
    };

    _numSkyboxIndices = 24;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _skyboxIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void Renderer::showGroundRendering(bool showRendering) {
    _renderGround = showRendering;
}

void Renderer::showSkyboxRendering(bool showRendering) {
    _renderSkybox = showRendering;
}

void Renderer::limitCameraPosition(bool limitDistance) {
    _limitCameraPosition = limitDistance;
}

unsigned int Renderer::numberOfParticles() const {
    return _numberOfParticles;
}

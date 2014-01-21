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

#ifndef __RENDERER_H__
#define __RENDERER_H__

// Need to include opengl first, as QGLWidget will include gl, but not glew
#include <ghoul/opengl/opengl>

#include <QGLWidget>
#include <glm/glm.hpp>

class Renderer : public QGLWidget {
Q_OBJECT
public:
    // Default destructor. Nothing fancy
    Renderer(const QGLFormat& format, QWidget* parent = 0, Qt::WindowFlags f = 0);

    // Destructor that will delete all the OpenGL objects that are created inside this class
    ~Renderer();

    // Assigns the data in 'particleData' to this renderer to be used as a data source
    // This function will call 'updateData' after setting the new data source
    void setData(const std::vector<glm::vec3>* particleData);

    // Recreate the VertexBufferObjects from the data previously stored in particleData
    // Since 'setData' takes in a pointer, this method should be called if the underlying data
    // has changed.
    void updateData();

    // Returns the number of particles currently in the rendering system
    unsigned int numberOfParticles() const;

public slots:
    // Determines if the ground plane should be rendered or not
    void showGroundRendering(bool showRendering);
    // Determines if the skybox should be rendered or not
    void showSkyboxRendering(bool showRendering);
    // Sets a state if the distance of the camera is limited to the skybox
    void limitCameraPosition(bool limitCamera);

protected:
    // creates all the necessary OpenGL objects (VBOs, IBOs, Textures, Shaders, etc)
    void initializeGL();
    
    // Draws the ground, the skybox and the particles
    void paintGL();

    // Handles resizing by recreating the viewprojection matrix
    void resizeGL(int width, int height);

    // Handler for mouse press events
    void mousePressEvent(QMouseEvent* event);

    // Handler for mouse move events; Will handle rotation, tilting and zooming
    void mouseMoveEvent(QMouseEvent* event);

private:
    // Handle rotation/tilting based on 'newMouse' and '_oldMousePosition'
    void rotate(const glm::vec2& newMouse);
    
    // Handle zooming based on 'newMouse' and '_oldMousePosition'
    void zoom(const glm::vec2& newMouse);

    // Creates all objects necessary to render the ground plane
    void initializeGround();
    // Creates the VBO holding the vertices for the ground plane
    void generateGroundBuffer();
    // Draws the ground plane
    void drawGround();
    // Returns true if all objects for the ground have been created
    bool groundIsReady() const;

    // Creates all the objects necessary to render the skybox
    void initializeSkybox();
    // Creates the VBO and IBO to render the skybox
    void generateSkyboxBuffer();
    // Draws the skybox
    void drawSkybox();
    // Returns true if all objects for the skybox have been created
    bool skyboxIsReady() const;

    // Creates the objects necessary to render the particles
    void initializeParticle();
    // Draws the particles
    void drawParticles();
    // Returns true, if all objects for the particles have been created and particle data exists
    bool particlesAreReady() const;

    // Recreate the view matrix and projection matrix from the current position, focus, upVector
    // and window sizes
    void updateViewProjectionMatrix();

    // Transforms the window-coordinates mouse position into the [-1,1] domain
    glm::vec2 scaledMouse(const glm::ivec2& mousePos) const;

    // Stores the old mouse position to be accessed in the next handle function
    glm::vec2 _oldMousePosition;

    // Is the position of the camera limited by the distance of the ground plane?
    bool _limitCameraPosition;

    // The current camera position
    glm::vec3 _position;

    // The current focus point of the camera
    glm::vec3 _focus;

    // The current upVector of the camera
    glm::vec3 _upVector;

    // The cached premultiplied view-projection matrix
    glm::mat4 _viewProjectionMatrix;

    // The current position of the light
    glm::vec3 _lightPosition;

    // Our local copy of the particle position data. This cannot be changed and we don't own
    // this data
    const std::vector<glm::vec3>* _particleData;

    // Should the ground be rendered or not
    bool _renderGround;
    // The vertex buffer object storing the vertices for the ground plane
    GLuint _groundVBO;
    // The color texture used for the ground plane
    ghoul::opengl::Texture* _groundTexture;
    // The normal texture used for the ground plane
    ghoul::opengl::Texture* _groundTextureNormal;
    // The ProgramObject that is used to render the ground plane
    ghoul::opengl::ProgramObject* _groundProgram;
    // True, if the ground plane subcomponent is ready to render
    bool _groundProgramReady;

    // Should the skybox be rendered or not
    bool _renderSkybox;
    // The vertex buffer object storing the vertices for the skybox
    GLuint _skyboxVBO;
    // The index buffer object storing the faces of the skybox
    GLuint _skyboxIBO;
    // The number of indices (24) for the skybox
    int _numSkyboxIndices;
    // The color texture used for the skybox
    GLuint _skyboxTexture;
    // The ProgramObject that is used to render the skybox
    ghoul::opengl::ProgramObject* _skyboxProgram;
    // True, if the skybox subcomponent is ready to render
    bool _skyboxProgramReady;
    
    // The vertex buffer object storing the vertices for the particles
    GLuint _particleVBO;
    // The color texture used for the particles
    ghoul::opengl::Texture* _particleTexture;
    // The Programobject that is used to render the particles
    ghoul::opengl::ProgramObject* _particleProgram;
    // True, if the particle subcomponent is ready to render
    bool _particleProgramReady;

    // Current number of particles in the rendering system
    GLsizei _numberOfParticles;
};

#endif // __RENDERER_H__
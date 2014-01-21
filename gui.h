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

#ifndef __GUI_H__
#define __GUI_H__

#include <QWidget>
#include <glm/glm.hpp>
#include <functional>

class Renderer;
class QCheckBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QTimer;

// A strongly-typed enumeration of the possible effects that can be sent to the callback function
enum class EffectType { Gravity, Wind };
// A strongly-typed enumeration of the possible sources that can be sent to the callback function
enum class SourceType { Point, Cone };

class GUI : public QWidget {
Q_OBJECT
public:
    // The constructor will create create and layout all of the subwidgets and initialize them, as
    // well as starting the timer that will trigger the update callbacks and the rendering
    GUI(QWidget* parent = 0, Qt::WindowFlags f = 0);

    // Pass a pointer to the data that should be used for rendering the particles. Each element in
    // the vector is one particle at a specific position
    void setData(std::vector<glm::vec3>* particleData);

    // Pass functions into these callbacks that will be called whenever the appropriate action
    // happens. 'sourceAddedCallback' will be called when one of the source buttons has been
    // pressed, 'effectAddedCallback' will be called when one of the effect buttons has been
    // pressed, and 'updateCallback' is called whenever the timer signals an update
    void setCallbacks(
        std::function<void(SourceType, glm::vec3, float)> sourceAddedCallback,
        std::function<void(EffectType, glm::vec3, float)> effectAddedCallback,
        std::function<void(float)> updateCallback,
        std::function<void()> removeAllCallback
        );

private slots:
    // This slot will be called when any of the buttons in the interface is pressed
    void handleButtonPress();
    // This slot is activated by the timer and will call the update callback and trigger a rendering
    void handleUpdate();
    // This slot handles the connection between source slider and source label value
    void handleSourceSlider();
    // This slot handles when the randomize function of the source position is toggled
    void handleSourceRandomize();
    // This slot handles the connection between effect slider and effect label value
    void handleEffectSlider();
    // This slot handles when the randomize function of the effect position is toggled
    void handleEffectRandomize();
    // This slot handles when the removeAll button has been pressed
    void handleRemoveAll();

private:
    // Creates the renderer window with the correct OpenGL parameters
    void createRenderer();
    // Creates the group box that contains all the widgets responsible for the sources
    void createSourceBox();
    // Creates the group box that contains all the widgets responsible for the effects
    void createEffectBox();
    // Creates the group box that contains all the widgets responsible for the rendering
    void createRenderingBox();

    // Returns the current position of the sources in the domain [-1,1]
    glm::vec3 sourcePosition() const;
    // Returns the current value of the source slider in the domain [0,1]
    float sourceValue() const;
    // Returns the current position of the effects in the domain [-1,1]
    glm::vec3 effectPosition() const;
    // Returns the current value of the effect slider in the domain [0,1]
    float effectValue() const;
    
    // The main layout of the whole widget
    QGridLayout* _layout;

    // The renderer
    Renderer* _renderer;

    // Widgets for the sources
    QLineEdit* _sourcePositionXText;
    QLineEdit* _sourcePositionYText;
    QLineEdit* _sourcePositionZText;
    QCheckBox* _sourcePositionRandomize;
    QSlider* _sourceValueSlider;
    QLabel* _sourceLabel;
    QPushButton* _sourcePointButton;
    QPushButton* _sourceConeButton;

    // Widgets for the effects
    QLineEdit* _effectPositionXText;
    QLineEdit* _effectPositionYText;
    QLineEdit* _effectPositionZText;
    QCheckBox* _effectPositionRandomize;
    QSlider* _effectValueSlider;
    QLabel* _effectLabel;
    QPushButton* _effectGravityButton;
    QPushButton* _effectWindButton;

    // Widgets for the rendering feedback
    QLabel* _numParticlesLabel;

    // The timer that will trigger updates and renderings
    QTimer* _timer;

    // Callback functions
    std::function<void(SourceType, glm::vec3, float)> _sourceAddedCallback;
    std::function<void(EffectType, glm::vec3, float)> _effectAddedCallback;
    std::function<void(float)> _updateCallback;
    std::function<void()> _removeAllCallback;
};

#endif // __GUI_H__
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

#include "gui.h"
#include "renderer.h"

#include "ghoul/logging/logmanager.h"
#include <QCheckBox>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QGLFormat>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalMapper>
#include <QSlider>
#include <QTimer>
#include <string>
#include <random>

namespace {
    std::string _loggerCat = "GUI";

    // The size of the render window inside the main widget
    const QSize _rendererSize = QSize(800, 600);

    // Initialize the engine and the distribution of the random number generator
    // Which random number generator is chosen is up to the implementation
    std::default_random_engine _generator;
    std::uniform_real_distribution<float> _distributionPosition(-1.f, 1.f);
}

GUI::GUI(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , _layout(nullptr)
    , _renderer(nullptr)
    , _sourcePositionXText(nullptr)
    , _sourcePositionYText(nullptr)
    , _sourcePositionZText(nullptr)
    , _sourcePositionRandomize(nullptr)
    , _sourceValueSlider(nullptr)
    , _sourceLabel(nullptr)
    , _sourcePointButton(nullptr)
    , _sourceConeButton(nullptr)
    , _effectPositionXText(nullptr)
    , _effectPositionYText(nullptr)
    , _effectPositionZText(nullptr)
    , _effectPositionRandomize(nullptr)
    , _effectValueSlider(nullptr)
    , _effectLabel(nullptr)
    , _effectGravityButton(nullptr)
    , _effectWindButton(nullptr)
    , _timer(nullptr)
    , _sourceAddedCallback([](SourceType, glm::vec3, float){}) // initialize function pointer with empty lambda expressions
    , _effectAddedCallback([](EffectType, glm::vec3, float){}) // initialize function pointer with empty lambda expressions
    , _updateCallback([](float){}) // initialize function pointer with empty lambda expressions
    , _removeAllCallback([](){}) // initialize function pointer with empty lambda expressions
{
    //   -----------------------------------------------------------
    //   |                                         |               |
    //   |                                         |    Source     | row 0
    //   |                                         |               |
    //   |                                         |---------------|
    //   |                                         |               |
    //   |                                         |    Effect     | row 1
    //   |                                         |               |
    //   |                Renderer                 |---------------|
    //   |                                         |               |
    //   |                                         |   Removeall   | row 2
    //   |                                         |               |
    //   |                                         |---------------|
    //   |                                         |               |
    //   |                                         | Renderoptions | row 3
    //   |                                         |               |
    //   -----------------------------------------------------------
    //                   column 0                       column 1

    _layout = new QGridLayout;
    // Setting the size contraint to QLayout::SetFixedSize, we prohibit resizing
    _layout->setSizeConstraint(QLayout::SetFixedSize);
    // Allow the first column to take 5x as much space as the others
    _layout->setColumnStretch(0, 5);
    setLayout(_layout);

    createRenderer();
    createSourceBox();
    createEffectBox();

    QPushButton* removeAll = new QPushButton("Remove all");
    connect(removeAll, SIGNAL(clicked(bool)), this, SLOT(handleRemoveAll()));
    _layout->addWidget(removeAll, 2, 1, 1, 1);

    createRenderingBox();

    // Create the timer that will drive the rendering and update rate of 60Hz
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(handleUpdate()));
    _timer->start(16); // 16ms = 60Hz refresh rate
}

void GUI::createRenderer() {
    // Determines the default framebuffer object that we will receive from Qt
    QGLFormat format(QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba);
    // Core profile ftw
    format.setProfile(QGLFormat::CoreProfile);

    // Check if the computer supports at least OpenGL 4.0
    // The renderer is written for > 4.0, so this is a hard requirement
    QGLFormat::OpenGLVersionFlags flags = QGLFormat::openGLVersionFlags();
    const bool hasOpenGL4_0 = flags & QGLFormat::OpenGL_Version_4_0;
    if (!hasOpenGL4_0)
        LFATAL("Graphics Driver do not support OpenGL version 4.0. Renderer will not work.");

    // Initialize the renderer with the format that we determined above
    _renderer = new Renderer(format);
    // We have to set the size of the renderer to determine the overall size of this widget
    _renderer->setFixedSize(_rendererSize);
    _layout->addWidget(_renderer, 0, 0, 4, 1);
}

void GUI::createSourceBox() {
    //   -------------------------------------------------
    //   | Position  |  PosX  |  PosY  |  PosZ  | Random | row 0
    //   |   Value   |  Slider-Slider-Slider    | Value  | row 1
    //   |  SourcePointBtn    |        |   SourceConeBtn | row 2
    //   -------------------------------------------------
    //      col0        col1     col2     col 3    col4

    // A Groupbox that will group all of the Source-related widgets in one box
    QGroupBox* sourceBox = new QGroupBox("Source");
    QGridLayout* boxLayout = new QGridLayout;

    // Setting the position of the new Source
    QLabel* positionText = new QLabel("Position");
    boxLayout->addWidget(positionText, 0, 0, 1, 1);

    // A validator that allows only floating point numbers in [-1,1] to be entered
    QValidator* validator = new QDoubleValidator(-1.0, 1.0, 6);

    // The textbox that holds the x component of the position
    _sourcePositionXText = new QLineEdit;
    _sourcePositionXText->setValidator(validator);
    _sourcePositionXText->setFixedWidth(65);
    boxLayout->addWidget(_sourcePositionXText, 0, 1, 1, 1);

    // The textbox that holds the y component of the position
    _sourcePositionYText = new QLineEdit;
    _sourcePositionYText->setValidator(validator);
    _sourcePositionYText->setFixedWidth(65);
    boxLayout->addWidget(_sourcePositionYText, 0, 2, 1, 1);

    // The textbox that holds the z component of the position
    _sourcePositionZText = new QLineEdit;
    _sourcePositionZText->setValidator(validator);
    _sourcePositionZText->setFixedWidth(65);
    boxLayout->addWidget(_sourcePositionZText, 0, 3, 1, 1);

    // A checkbox that will disable the manual entries and generate random positions
    _sourcePositionRandomize = new QCheckBox("Randomize");
    _sourcePositionRandomize->setChecked(true);
    connect(_sourcePositionRandomize, SIGNAL(toggled(bool)), this, SLOT(handleSourceRandomize()));
    boxLayout->addWidget(_sourcePositionRandomize, 0, 4, 1, 1);

    // A slider that is used to choose a value in [0,1] that can be used for whatever effect
    QLabel* valueText = new QLabel("Value");
    boxLayout->addWidget(valueText, 1, 0, 1, 1);

    _sourceValueSlider = new QSlider;
    _sourceValueSlider->setOrientation(Qt::Horizontal);
    _sourceValueSlider->setMaximum(2000);
    _sourceValueSlider->setValue(2000/2);
    connect(_sourceValueSlider, SIGNAL(valueChanged(int)), this, SLOT(handleSourceSlider()));
    boxLayout->addWidget(_sourceValueSlider, 1, 1, 1, 3);

    // This label will display the value that was chosen with the slider
    _sourceLabel = new QLabel;
    _sourceLabel->setFixedWidth(65);
    _sourceLabel->setAlignment(Qt::AlignHCenter);
    boxLayout->addWidget(_sourceLabel, 1, 4, 1, 1);

    // The button that will call the callback '_sourceAddedCallback' with Source::Point when pressed
    _sourcePointButton = new QPushButton("Point Source");
    connect(_sourcePointButton, SIGNAL(pressed()), this, SLOT(handleButtonPress()));
    boxLayout->addWidget(_sourcePointButton, 2, 0, 1, 2);

    // The button that will call the callback '_sourceAddedCallback' with Source::Cone when pressed
    _sourceConeButton = new QPushButton("Cone Source");
    //_sourceConeButton->setFixedWidth(3 * 65);
    connect(_sourceConeButton, SIGNAL(pressed()), this, SLOT(handleButtonPress()));
    boxLayout->addWidget(_sourceConeButton, 2, 3, 1, 2);

    sourceBox->setLayout(boxLayout);
    _layout->addWidget(sourceBox, 0, 1, 1, 1);

    // Update the text fields according to the randomize default value
    handleSourceRandomize();
    // Update the label value to the default slider value
    handleSourceSlider();
}

void GUI::createEffectBox() {
    //   -------------------------------------------------
    //   | Position  |  PosX  |  PosY  |  PosZ  | Random | row 0
    //   |   Value   |  Slider-Slider-Slider    | Value  | row 1
    //   |  EffectGravity     |        |    EffectWind   | row 2
    //   -------------------------------------------------
    //      col0        col1     col2     col 3    col4

    // A Groupbox that will group all of the Source-related widgets in one box
    QGroupBox* effectBox = new QGroupBox("Effect");
    QGridLayout* boxLayout = new QGridLayout;

    // Setting the position of the new Source
    QLabel* positionText = new QLabel("Position");
    boxLayout->addWidget(positionText, 0, 0, 1, 1);

    // A validator that allows only floating point numbers in [-1,1] to be entered
    QValidator* validator = new QDoubleValidator(-1.0, 1.0, 6);

    // The textbox that holds the x component of the position
    _effectPositionXText = new QLineEdit;
    _effectPositionXText->setValidator(validator);
    _effectPositionXText->setFixedWidth(65);
    boxLayout->addWidget(_effectPositionXText, 0, 1, 1, 1);

    // The textbox that holds the y component of the position
    _effectPositionYText = new QLineEdit;
    _effectPositionYText->setValidator(validator);
    _effectPositionYText->setFixedWidth(65);
    boxLayout->addWidget(_effectPositionYText, 0, 2, 1, 1);

    // The textbox that holds the z component of the position
    _effectPositionZText = new QLineEdit;
    _effectPositionZText->setValidator(validator);
    _effectPositionZText->setFixedWidth(65);
    boxLayout->addWidget(_effectPositionZText, 0, 3, 1, 1);

    // A checkbox that will disable the manual entries and generate random positions
    _effectPositionRandomize = new QCheckBox("Randomize");
    _effectPositionRandomize->setChecked(true);
    connect(_effectPositionRandomize, SIGNAL(toggled(bool)), this, SLOT(handleEffectRandomize()));
    boxLayout->addWidget(_effectPositionRandomize, 0, 4, 1, 1);

    // A slider that is used to choose a value in [0,1] that can be used for whatever effect
    QLabel* valueText = new QLabel("Value");
    boxLayout->addWidget(valueText, 1, 0, 1, 1);

    // A slider that is used to choose a value in [0,1] that can be used for whatever effect
    _effectValueSlider = new QSlider;
    _effectValueSlider->setOrientation(Qt::Horizontal);
    _effectValueSlider->setMaximum(2000);
    _effectValueSlider->setValue(2000/2);
    connect(_effectValueSlider, SIGNAL(valueChanged(int)), this, SLOT(handleEffectSlider()));
    boxLayout->addWidget(_effectValueSlider, 1, 1, 1, 3);

    // This label will display the value that was chosen with the slider
    _effectLabel = new QLabel;
    _effectLabel->setFixedWidth(65);
    _effectLabel->setAlignment(Qt::AlignHCenter);
    boxLayout->addWidget(_effectLabel, 1, 4, 1, 1);

    // The button that will call the callback '_effectAddedCallback' with Effect::Gravity when pressed
    _effectGravityButton = new QPushButton("Effects Gravity");
    connect(_effectGravityButton, SIGNAL(pressed()), this, SLOT(handleButtonPress()));
    boxLayout->addWidget(_effectGravityButton, 2, 0, 1, 2);

    // The button that will call the callback '_effectAddedCallback' with Effect::Wind when pressed
    _effectWindButton = new QPushButton("Effects Wind");
    connect(_effectWindButton, SIGNAL(pressed()), this, SLOT(handleButtonPress()));
    boxLayout->addWidget(_effectWindButton, 2, 3, 1, 2);

    effectBox->setLayout(boxLayout);
    _layout->addWidget(effectBox, 1, 1, 1, 1);

    // Update the text fields according to the randomize default value
    handleEffectRandomize();
    // Update the label value to the default slider value
    handleEffectSlider();
}

void GUI::createRenderingBox() {
    QGroupBox* renderingBox = new QGroupBox("Rendering");
    QVBoxLayout* boxLayout = new QVBoxLayout;

    QCheckBox* enableRenderingGround = new QCheckBox("Show ground plane");
    enableRenderingGround->setChecked(true);
    connect(enableRenderingGround, SIGNAL(toggled(bool)), _renderer, SLOT(showGroundRendering(bool)));
    boxLayout->addWidget(enableRenderingGround);
    
    QCheckBox* enableRenderingSkybox = new QCheckBox("Show Skybox");
    enableRenderingSkybox->setChecked(true);
    connect(enableRenderingSkybox, SIGNAL(toggled(bool)), _renderer, SLOT(showSkyboxRendering(bool)));
    boxLayout->addWidget(enableRenderingSkybox);

    QCheckBox* limitCameraPosition = new QCheckBox("Limit Camera Position");
    limitCameraPosition->setChecked(true);
    connect(limitCameraPosition, SIGNAL(toggled(bool)), _renderer, SLOT(limitCameraPosition(bool)));
    boxLayout->addWidget(limitCameraPosition);

    _numParticlesLabel = new QLabel("Number of Particles:\n");
    boxLayout->addWidget(_numParticlesLabel);

    renderingBox->setLayout(boxLayout);
    _layout->addWidget(renderingBox, 3, 1, 1, 1, Qt::AlignBottom);
}

void GUI::handleButtonPress() {
    // Get the QObject that triggered this signal
    QObject* sender = QObject::sender();

    // Determine which callback to call with which parameters
    if (sender == _sourcePointButton)
        _sourceAddedCallback(SourceType::Point, sourcePosition(), sourceValue());
    else if (sender == _sourceConeButton)
        _sourceAddedCallback(SourceType::Cone, sourcePosition(), sourceValue());
    else if (sender == _effectGravityButton)
        _effectAddedCallback(EffectType::Gravity, effectPosition(), effectValue());
    else if (sender == _effectWindButton)
        _effectAddedCallback(EffectType::Wind, effectPosition(), effectValue());
    else
        LFATAL("Missing handler for button press");
}
void GUI::handleUpdate() {
    const int interval = _timer->interval(); // in ms
    _updateCallback(interval / 1000.f); // in s

    // Update the data of the renderer after the update callback has returned
    _renderer->updateData();
    // Update the label showing the amount of particles
    _numParticlesLabel->setText(QString("Number of Particles:\n%1").arg(_renderer->numberOfParticles()));
    // Trigger a new rendering
    _renderer->updateGL();
}

void GUI::handleSourceSlider() {
    // Get the source slider value and set it as the text label
    const float t = sourceValue();
    QString labelText = QString::number(t);
    _sourceLabel->setText(labelText);
}

void GUI::handleSourceRandomize() {
    const bool randomize = _sourcePositionRandomize->isChecked();
    // If the randomize function is selected, we don't allow editing of the text fields
    _sourcePositionXText->setEnabled(!randomize);
    _sourcePositionYText->setEnabled(!randomize);
    _sourcePositionZText->setEnabled(!randomize);
}

void GUI::handleEffectSlider() {
    // Get the effect slider value and set it as the text label
    const float t = effectValue();
    QString labelText = QString::number(t);
    _effectLabel->setText(labelText);
}

void GUI::handleEffectRandomize() {
    const bool randomize = _effectPositionRandomize->isChecked();
    // If the randomize function is selected, we don't allow editing of the text fields
    _effectPositionXText->setEnabled(!randomize);
    _effectPositionYText->setEnabled(!randomize);
    _effectPositionZText->setEnabled(!randomize);
}

void GUI::handleRemoveAll() {
    _removeAllCallback();
}

glm::vec3 GUI::sourcePosition() const {
    const bool randomize = _sourcePositionRandomize->isChecked();
    if (randomize) {
        // Generate three random number in the domain determined by _distribution
        const float x = _distributionPosition(_generator);
        const float y = _distributionPosition(_generator);
        const float z = _distributionPosition(_generator);
        return glm::vec3(x, y, z);
    }
    else {
        const float x = _sourcePositionXText->text().toFloat();
        const float y = _sourcePositionYText->text().toFloat();
        const float z = _sourcePositionZText->text().toFloat();
        return glm::vec3(x, y, z);
    }
}

float GUI::sourceValue() const {
    // Normalize the value from [minValue,maxValue] to [0,1]
    const int minValue = _sourceValueSlider->minimum();
    const int maxValue = _sourceValueSlider->maximum();
    const int currentValue = _sourceValueSlider->value();
    // At least one of the arguments has to be of type float, otherwise the result
    // would always be 0 because of int/int division
    return (static_cast<float>(currentValue) - minValue) / (maxValue - minValue);
}

glm::vec3 GUI::effectPosition() const {
    const bool randomize = _effectPositionRandomize->isChecked();
    if (randomize) {
        // Generate three random number in the domain determined by _distribution
        const float x = _distributionPosition(_generator);
        const float y = _distributionPosition(_generator);
        const float z = _distributionPosition(_generator);
        return glm::vec3(x, y, z);
    }
    else {
        const float x = _effectPositionXText->text().toFloat();
        const float y = _effectPositionYText->text().toFloat();
        const float z = _effectPositionZText->text().toFloat();
        return glm::vec3(x, y, z);
    }
}

float GUI::effectValue() const {
    // Normalize the value from [minValue,maxValue] to [0,1]
    const int minValue = _effectValueSlider->minimum();
    const int maxValue = _effectValueSlider->maximum();
    const int currentValue = _effectValueSlider->value();
    // At least one of the arguments has to be of type float, otherwise the result
    // would always be 0 because of int/int division
    return (static_cast<float>(currentValue) - minValue) / (maxValue - minValue);
}

void GUI::setData(std::vector<glm::vec3>* particleData) {
    // just forwarding the data to the renderer
    _renderer->setData(particleData);
}

void GUI::setCallbacks(
    std::function<void(SourceType, glm::vec3, float)> sourceAddedCallback,
    std::function<void(EffectType, glm::vec3, float)> effectAddedCallback,
    std::function<void(float)> updateCallback,
    std::function<void()> removeAllCallback)
{
    _sourceAddedCallback = sourceAddedCallback;
    _effectAddedCallback = effectAddedCallback;
    _updateCallback = updateCallback;
    _removeAllCallback = removeAllCallback;
}

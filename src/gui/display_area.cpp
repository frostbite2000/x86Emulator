/*
 * x86Emulator - A portable x86 PC emulator written in C++
 * 
 * Copyright (C) 2024 frostbite2000
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/display_area.h"
#include <QCursor>
#include <QApplication>
#include <QScreen>

DisplayArea::DisplayArea(QWidget* parent)
    : QOpenGLWidget(parent),
      m_emulator(nullptr),
      m_program(nullptr),
      m_texture(nullptr),
      m_fullscreen(false),
      m_mouseCapture(false),
      m_displayWidth(640),
      m_displayHeight(480),
      m_maintainAspectRatio(true),
      m_aspectRatio(4.0f / 3.0f)
{
    // Set widget attributes
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // Allocate frame buffer for initial size
    m_frameBuffer = QImage(m_displayWidth, m_displayHeight, QImage::Format_RGB32);
    m_frameBuffer.fill(Qt::black);
}

DisplayArea::~DisplayArea()
{
    // Clean up OpenGL resources
    makeCurrent();
    
    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }
    
    if (m_program) {
        delete m_program;
        m_program = nullptr;
    }
    
    doneCurrent();
}

void DisplayArea::initialize(Emulator* emulator)
{
    m_emulator = emulator;
    
    // Get initial display dimensions from emulator
    m_displayWidth = m_emulator->getDisplayWidth();
    m_displayHeight = m_emulator->getDisplayHeight();
    m_aspectRatio = float(m_displayWidth) / float(m_displayHeight);
    
    // Reallocate frame buffer if needed
    if (m_frameBuffer.width() != m_displayWidth || m_frameBuffer.height() != m_displayHeight) {
        m_frameBuffer = QImage(m_displayWidth, m_displayHeight, QImage::Format_RGB32);
        m_frameBuffer.fill(Qt::black);
    }
    
    // Recreate texture if already initialized
    if (m_texture) {
        makeCurrent();
        delete m_texture;
        setupTexture();
        doneCurrent();
    }
    
    // Update display area
    update();
}

void DisplayArea::setFullScreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
    
    // Release mouse capture when exiting fullscreen
    if (!fullscreen && m_mouseCapture) {
        m_mouseCapture = false;
        QApplication::restoreOverrideCursor();
        emit mouseStateChanged(false);
    }
    
    update();
}

QImage DisplayArea::grabFramebuffer() const
{
    // Return a copy of the current frame buffer
    return m_frameBuffer.copy();
}

void DisplayArea::adjustSize()
{
    // Calculate target rectangle for rendering
    int w = width();
    int h = height();
    
    if (m_maintainAspectRatio) {
        float widgetRatio = float(w) / float(h);
        
        if (widgetRatio > m_aspectRatio) {
            // Widget is too wide, center horizontally
            int newWidth = int(h * m_aspectRatio);
            m_targetRect = QRect((w - newWidth) / 2, 0, newWidth, h);
        } else {
            // Widget is too tall, center vertically
            int newHeight = int(w / m_aspectRatio);
            m_targetRect = QRect(0, (h - newHeight) / 2, w, newHeight);
        }
    } else {
        // Fill the entire widget
        m_targetRect = QRect(0, 0, w, h);
    }
    
    update();
}

void DisplayArea::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    setupShaders();
    setupTexture();
}

void DisplayArea::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    adjustSize();
}

void DisplayArea::paintGL()
{
    // Update texture from emulator frame buffer
    updateTexture();
    
    // Clear background
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render textured quad
    renderQuad();
}

void DisplayArea::mousePressEvent(QMouseEvent* event)
{
    if (m_mouseCapture) {
        // Forward mouse press to emulator
        if (m_emulator) {
            m_emulator->mouseButtonPressed(event->button());
        }
    } else if (event->button() == Qt::LeftButton && m_targetRect.contains(event->pos())) {
        // Capture mouse when clicking inside display area
        m_mouseCapture = true;
        QApplication::setOverrideCursor(Qt::BlankCursor);
        m_lastMousePos = QCursor::pos();
        emit mouseStateChanged(true);
    }
    
    event->accept();
}

void DisplayArea::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_mouseCapture) {
        // Forward mouse release to emulator
        if (m_emulator) {
            m_emulator->mouseButtonReleased(event->button());
        }
    }
    
    event->accept();
}

void DisplayArea::mouseMoveEvent(QMouseEvent* event)
{
    if (m_mouseCapture) {
        // Get screen center
        QPoint center = mapToGlobal(QPoint(width() / 2, height() / 2));
        
        // Calculate relative movement
        QPoint currentPos = QCursor::pos();
        QPoint delta = currentPos - m_lastMousePos;
        
        // Forward mouse movement to emulator
        if (m_emulator) {
            m_emulator->mouseMove(delta.x(), delta.y());
        }
        
        // Reset cursor to center to prevent hitting screen borders
        QCursor::setPos(center);
        m_lastMousePos = center;
    }
    
    event->accept();
}

void DisplayArea::wheelEvent(QWheelEvent* event)
{
    if (m_mouseCapture) {
        // Forward wheel movement to emulator
        if (m_emulator) {
            m_emulator->mouseWheelMove(event->angleDelta().y() / 120);
        }
        
        event->accept();
    } else {
        QOpenGLWidget::wheelEvent(event);
    }
}

void DisplayArea::keyPressEvent(QKeyEvent* event)
{
    // Check for escape key to release mouse capture
    if (event->key() == Qt::Key_Escape && m_mouseCapture) {
        m_mouseCapture = false;
        QApplication::restoreOverrideCursor();
        emit mouseStateChanged(false);
        event->accept();
        return;
    }
    
    // Forward other key events
    QOpenGLWidget::keyPressEvent(event);
}

void DisplayArea::keyReleaseEvent(QKeyEvent* event)
{
    // Forward key events
    QOpenGLWidget::keyReleaseEvent(event);
}

void DisplayArea::setupShaders()
{
    m_program = new QOpenGLShaderProgram;
    
    // Vertex shader
    const char* vertexShaderSource =
        "attribute vec2 position;\n"
        "attribute vec2 texCoord;\n"
        "varying vec2 texCoordOut;\n"
        "void main() {\n"
        "    gl_Position = vec4(position, 0.0, 1.0);\n"
        "    texCoordOut = texCoord;\n"
        "}\n";
    
    // Fragment shader
    const char* fragmentShaderSource =
        "uniform sampler2D texture;\n"
        "varying vec2 texCoordOut;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(texture, texCoordOut);\n"
        "}\n";
    
    // Compile and link shaders
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
}

void DisplayArea::setupTexture()
{
    if (m_texture) {
        delete m_texture;
    }
    
    // Create texture for display
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->setSize(m_displayWidth, m_displayHeight);
    m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_texture->allocateStorage();
    
    // Set texture filtering
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void DisplayArea::updateTexture()
{
    if (!m_emulator) {
        return;
    }
    
    // Get current frame from emulator
    if (m_emulator->getFrameBuffer(m_frameBuffer)) {
        // Update texture
        m_texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, m_frameBuffer.bits());
    }
}

void DisplayArea::renderQuad()
{
    if (!m_program || !m_texture) {
        return;
    }
    
    m_program->bind();
    m_texture->bind();
    
    // Calculate texture coordinates based on target rectangle
    GLfloat vertices[] = {
        // Positions     // Texture Coords
        -1.0f, -1.0f,    0.0f, 1.0f,  // Bottom left
         1.0f, -1.0f,    1.0f, 1.0f,  // Bottom right
         1.0f,  1.0f,    1.0f, 0.0f,  // Top right
        -1.0f,  1.0f,    0.0f, 0.0f   // Top left
    };
    
    // Set vertex attributes
    int posAttr = m_program->attributeLocation("position");
    int texAttr = m_program->attributeLocation("texCoord");
    
    m_program->enableAttributeArray(posAttr);
    m_program->enableAttributeArray(texAttr);
    
    m_program->setAttributeArray(posAttr, (const GLfloat*)vertices, 2, sizeof(GLfloat) * 4);
    m_program->setAttributeArray(texAttr, (const GLfloat*)(vertices + 2), 2, sizeof(GLfloat) * 4);
    
    // Draw quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Clean up
    m_program->disableAttributeArray(posAttr);
    m_program->disableAttributeArray(texAttr);
    
    m_texture->release();
    m_program->release();
}

void DisplayArea::updateCursorState()
{
    if (m_mouseCapture) {
        QApplication::setOverrideCursor(Qt::BlankCursor);
    } else {
        QApplication::restoreOverrideCursor();
    }
}
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

#ifndef DISPLAY_AREA_H
#define DISPLAY_AREA_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QImage>

#include "emulator.h"

/**
 * @brief The DisplayArea widget handles rendering of the emulated screen
 */
class DisplayArea : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit DisplayArea(QWidget* parent = nullptr);
    ~DisplayArea() override;
    
    void initialize(Emulator* emulator);
    void setFullScreen(bool fullscreen);
    QImage grabFramebuffer() const;
    void adjustSize();

signals:
    void mouseStateChanged(bool captured);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void setupShaders();
    void setupTexture();
    void updateTexture();
    void renderQuad();
    void updateCursorState();
    
    Emulator* m_emulator;
    
    // OpenGL resources
    QOpenGLShaderProgram* m_program;
    QOpenGLTexture* m_texture;
    
    // Display state
    bool m_fullscreen;
    bool m_mouseCapture;
    QPoint m_lastMousePos;
    
    // Emulated display properties
    int m_displayWidth;
    int m_displayHeight;
    QImage m_frameBuffer;
    
    // Aspect ratio correction
    bool m_maintainAspectRatio;
    float m_aspectRatio;
    QRect m_targetRect;
};

#endif // DISPLAY_AREA_H
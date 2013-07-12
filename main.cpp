/**
 *
 * Qt5 OpenGL ES 2.0 Test Application
 *
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Thomas Perl <thomas.perl@jollamobile.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **/

#include <QGuiApplication>
#include <QWindow>
#include <QTime>
#include <QTouchEvent>
#include <QMap>

#include <QOpenGLContext>
#include <QSurfaceFormat>

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

class OpenGLES2Test : public QWindow {
    public:
        OpenGLES2Test();

    protected:
        virtual void timerEvent(QTimerEvent *event);
	virtual void touchEvent(QTouchEvent *event);
        virtual bool event(QEvent *event);

    private:
        QOpenGLContext *context;
        QOpenGLShaderProgram *prog;
        QTime time;
        QMap<int,QPointF> touches;
        bool focused;
        int timerId;
};

OpenGLES2Test::OpenGLES2Test()
    : QWindow()
    , context(NULL)
    , time()
    , touches()
    , focused(false)
    , timerId(-1)
{
    setSurfaceType(QSurface::OpenGLSurface);
    reportContentOrientationChange(Qt::LandscapeOrientation);

    // Start measuring time since initialization (for animation)
    time.start();
}

static const char *vertex_shader_src =
"precision mediump float;\n"
"\n"
"attribute vec2 vtxcoord;\n"
"uniform mat4 projection;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = projection * vec4(vtxcoord, 0.0, 1.0);\n"
"}\n"
;

static const char *fragment_shader_src =
"precision mediump float;\n"
"\n"
"uniform vec4 color;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = color;\n"
"}\n"
;

void
OpenGLES2Test::timerEvent(QTimerEvent *event)
{
    if (context == NULL) {
        context = new QOpenGLContext(this);
        context->create();
        context->makeCurrent(this);

        glViewport(0, 0, width(), height());
        glClearColor(1.0, 0.0, 0.0, 1.0);

        prog = new QOpenGLShaderProgram(this);
        prog->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_src);
        prog->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_src);
        prog->link();

        qDebug() << "Shader program log:" << prog->log();
    } else {
        context->makeCurrent(this);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    prog->bind();

    int vtxcoord_loc = prog->attributeLocation("vtxcoord");
    glEnableVertexAttribArray(vtxcoord_loc);

    // Set orthographic projection on the whole window
    QMatrix4x4 projection;
    projection.ortho(0, width(), height(), 0, -1, 1);
    prog->setUniformValue("projection", projection);

    // offset width (in pixels) = half side of squares
    int offset = 40;

    // Draw a yellow rotating square (animated) in the screen center
    QMatrix m;
    m.rotate(time.elapsed() * 0.1);

    // Center of square should be in top left corner (with some spacing)
    QPointF center(offset * 2, offset * 2);

    QPointF a = center + (QPointF(-offset, -offset) * m);
    QPointF b = center + (QPointF(offset, -offset) * m);
    QPointF c = center + (QPointF(-offset, offset) * m);
    QPointF d = center + (QPointF(offset, offset) * m);
    float vertices[] = {
        a.x(), a.y(),
        b.x(), b.y(),
        c.x(), c.y(),
        d.x(), d.y(),
    };

    glVertexAttribPointer(vtxcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    if (focused) {
        prog->setUniformValue("color", 1.0, 1.0, 0.0, 1.0);
    } else {
        prog->setUniformValue("color", 1.0, 0.0, 1.0, 1.0);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Draw each active touch point in green
    prog->setUniformValue("color", 0.0, 1.0, 0.0, 1.0);
    foreach (QPointF point, touches.values()) {
        float vtx[] = {
            point.x()-offset, point.y()-offset,
            point.x()+offset, point.y()-offset,
            point.x()-offset, point.y()+offset,
            point.x()+offset, point.y()+offset,
        };
        glVertexAttribPointer(vtxcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, vtx);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glDisableVertexAttribArray(vtxcoord_loc);
    prog->release();

    context->swapBuffers(this);
}

void
OpenGLES2Test::touchEvent(QTouchEvent *event)
{
    foreach (QTouchEvent::TouchPoint point, event->touchPoints()) {
        int id = point.id();
        QPointF pos = point.pos();
        Qt::TouchPointState state = point.state();

        switch (state) {
            case Qt::TouchPointPressed:
                qDebug() << "pressed" << id << "at" << pos;
                touches[id] = pos;
                break;
            case Qt::TouchPointMoved:
                qDebug() << "moved" << id << "at" << pos;
                touches[id] = pos;
                break;
            case Qt::TouchPointStationary:
                qDebug() << "stationary" << id << "at" << pos;
                //touches[id] = pos;
                break;
            case Qt::TouchPointReleased:
                qDebug() << "released" << id << "at" << pos;
                touches.remove(id);
                break;
            default:
                qDebug() << "!!";
                break;
        }
    }
}

bool
OpenGLES2Test::event(QEvent *event)
{
    switch (event->type()) {
        case QEvent::FocusIn:
            qDebug() << "Got Focus";
            timerId = startTimer(1000. / 60.); // target 60 FPS
            focused = true;
            break;
        case QEvent::FocusOut:
            qDebug() << "Lost Focus";
            killTimer(timerId);
            focused = false;
            timerEvent(NULL); // render "backgrounded" state once
            break;
        default:
            /* do nothing */
            break;
    }

    return QWindow::event(event);
}

int
main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    OpenGLES2Test window;
    window.resize(640, 480);
    window.showFullScreen();
    qDebug() << "size:" << window.size();

    return app.exec();
}


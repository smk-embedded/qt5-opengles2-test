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
#include <QScreen>
#include <QTime>
#include <QTouchEvent>
#include <QMap>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

#include <QAccelerometer>

class OpenGLES2Test : public QWindow, protected QOpenGLFunctions {
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
        bool initial;
        QAccelerometer accelerometer;
};

OpenGLES2Test::OpenGLES2Test()
    : QWindow()
    , context(NULL)
    , time()
    , touches()
    , focused(false)
    , timerId(-1)
    , initial(true)
    , accelerometer()
{
    setSurfaceType(QSurface::OpenGLSurface);

    // Start reading accelerometer values
    accelerometer.start();

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
    Q_UNUSED(event);

    if (!isExposed()) {
        qDebug() << "Not exposed yet";
        return;
    }

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

    // Visualize accelerometer readings
    QAccelerometerReading *reading = accelerometer.reading();
    if (reading) {
        QList<float> readings;
        readings << reading->x() << reading->y() << reading->z();
        float height = 40.f;
        float width = size().width() / 3.f;
        float spacing = height * 3.f / 4.f;
        float border = 4.f;

        float x = size().width() / 2.f;
        float y = size().height() / 2.f - (3 * height) - (2 * spacing);
        foreach (float reading, readings) {
            float bgvtx[] = {
                x - width, y,
                x + width, y,
                x - width, y + height,
                x + width, y + height,
            };

            prog->setUniformValue("color", 0.0, 0.0, 0.0, 1.0);
            glVertexAttribPointer(vtxcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, bgvtx);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            float fgvtx[] = {
                x + border, y + border,
                x + border + (width - 2 * border) * reading / 9.81f, y + border,
                x + border, y + height - border,
                x + border + (width - 2 * border) * reading / 9.81f, y + height - border,
            };

            prog->setUniformValue("color", 1.0, 1.0, 1.0, 1.0);
            glVertexAttribPointer(vtxcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, fgvtx);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            y += height + spacing;
        }
    }

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
        (float)a.x(), (float)a.y(),
        (float)b.x(), (float)b.y(),
        (float)c.x(), (float)c.y(),
        (float)d.x(), (float)d.y(),
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
            (float)point.x()-offset, (float)point.y()-offset,
            (float)point.x()+offset, (float)point.y()-offset,
            (float)point.x()-offset, (float)point.y()+offset,
            (float)point.x()+offset, (float)point.y()+offset,
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
        case QEvent::Show:
            if (initial) {
                qDebug() << "Initial Focus";
                if (timerId == -1) {
                    timerId = startTimer(1000. / 60.); // target 60 FPS
                }
                initial = false;
            }
            break;
        case QEvent::FocusIn:
            qDebug() << "Got Focus";
            if (timerId == -1) {
                timerId = startTimer(1000. / 60.); // target 60 FPS
            }
            focused = true;
            break;
        case QEvent::FocusOut:
            qDebug() << "Lost Focus";
            if (timerId != -1) {
                killTimer(timerId);
                timerId = -1;
            }
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
    window.showFullScreen();

    // This is how you would switch to landscape mode
    //window.reportContentOrientationChange(Qt::LandscapeOrientation);
    //window.screen()->setOrientationUpdateMask(Qt::LandscapeOrientation);

    qDebug() << "size:" << window.size();

    return app.exec();
}

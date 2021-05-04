#include "mainwindow.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QOpenGLWindow>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QScreen>
#include <QDebug>

//解析字符串中的opengl版本号
bool parseOpenGLVersion(const QByteArray &versionString, int &major, int &minor)
{
    bool majorOk = false;
    bool minorOk = false;
    QList<QByteArray> parts = versionString.split(' ');
    if (versionString.startsWith(QByteArrayLiteral("OpenGL ES"))) {
        if (parts.size() >= 3) {
            QList<QByteArray> versionParts = parts.at(2).split('.');
            if (versionParts.size() >= 2) {
                major = versionParts.at(0).toInt(&majorOk);
                minor = versionParts.at(1).toInt(&minorOk);
                // Nexus 6 has "OpenGL ES 3.0V@95.0 (GIT@I86da836d38)"
                if (!minorOk)
                    if (int idx = versionParts.at(1).indexOf('V'))
                        minor = versionParts.at(1).left(idx).toInt(&minorOk);
            } else {
                qWarning("Unrecognized OpenGL ES version");
            }
        } else {
            // If < 3 parts to the name, it is an unrecognised OpenGL ES
            qWarning("Unrecognised OpenGL ES version");
        }
    } else {
        // Not OpenGL ES, but regular OpenGL, the version numbers are first in the string
        QList<QByteArray> versionParts = parts.at(0).split('.');
        if (versionParts.size() >= 2) {
            major = versionParts.at(0).toInt(&majorOk);
            minor = versionParts.at(1).toInt(&minorOk);
        } else {
            qWarning("Unrecognized OpenGL version");
        }
    }

    if (!majorOk || !minorOk)
        qWarning("Unrecognized OpenGL version");
    return (majorOk && minorOk);
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);
    int major=0;
    int minor=0;
    //一般需要先构造一个QGuiApplication有些函数才能调用
    QScreen *screen=app.primaryScreen();
    if(screen){
        QOffscreenSurface surface(screen);
        surface.create();
        QOpenGLContext context;
        context.create();
        context.makeCurrent(&surface);

        const GLubyte *glstr=context.functions()->glGetString(GL_VERSION);
        if(glstr){
            QByteArray bytestr= QByteArray(reinterpret_cast<const char*>(glstr));
            qDebug()<<"glGetString"<<bytestr;
            if(!parseOpenGLVersion(bytestr,major,minor)){
                major=0;
                minor=0;
            }
        }
        context.doneCurrent();
        surface.destroy();
    }
    qDebug() << "version:" << major << minor;

    //默认指定为3.3版本
    if(major*10+minor>=33){
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        //qDebug() <<"deault format:"<< format;
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setMajorVersion(3);
        format.setMinorVersion(3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
        //qDebug() <<"current format"<< format;
    }else{
        qDebug()<<"The version of OpenGL is too old";
    }

    MainWindow w;
    w.show();
    return app.exec();
}

#ifndef CIRCLEPROGRESSBAR_H
#define CIRCLEPROGRESSBAR_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

#include <QPropertyAnimation>

class CircleProgressBar : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
    Q_PROPERTY(double drawValue READ getDrawValue WRITE setDrawValue)
public:
    explicit CircleProgressBar(QWidget *parent = nullptr);
    ~CircleProgressBar();

    void setRange(double min,double max);
    void setValue(double value);

    double getDrawValue() const;
    void setDrawValue(double value);

protected:
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

private:
    //着色器程序
    QOpenGLShaderProgram _shaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject _vao;
    //顶点缓冲
    QOpenGLBuffer _vbo;
    //属性动画
    QPropertyAnimation *_animation;
    //进度值
    double _progressMin=0;
    double _progressMax=100;
    double _progressValue=0; //设置的值
    double _progressDraw=0; //绘制临时值
};

#endif // CIRCLEPROGRESSBAR_H

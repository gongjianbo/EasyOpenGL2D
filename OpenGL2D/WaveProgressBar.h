#ifndef WAVEPROGRESSBAR_H
#define WAVEPROGRESSBAR_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

#include <QPropertyAnimation>
#include <QTimer>

//龚建波：进度球
class WaveProgressBar : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
    Q_PROPERTY(double drawValue READ getDrawValue WRITE setDrawValue)
public:
    explicit WaveProgressBar(QWidget *parent = nullptr);
    ~WaveProgressBar();

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
    QOpenGLShaderProgram shaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject vao;
    //顶点缓冲
    QOpenGLBuffer vbo;
    //属性动画
    QPropertyAnimation *animation;
    //进度值
    double progressMin=0;
    double progressMax=100;
    double progressValue=0; //设置的值
    double progressDraw=0; //绘制临时值
    //定时动画
    QTimer *timer;
    int timeValue=0;
};
#endif // WAVEPROGRESSBAR_H

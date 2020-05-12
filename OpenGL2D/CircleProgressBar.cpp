#include "CircleProgressBar.h"

#include <QPainter>

CircleProgressBar::CircleProgressBar(QWidget *parent)
    : QOpenGLWidget(parent)
{
    _animation=new QPropertyAnimation(this,"drawValue");
    _animation->setDuration(2000); //动画持续时间
    _animation->setEasingCurve(QEasingCurve::OutQuart); //先快后慢
}

CircleProgressBar::~CircleProgressBar()
{
    makeCurrent();
    _vbo.destroy();
    _vao.destroy();
    doneCurrent();
}

void CircleProgressBar::setRange(double min, double max)
{
    if(_progressMax<=_progressMin)
        return;
    _progressMin=min;
    _progressMax=max;
}

void CircleProgressBar::setValue(double value)
{
    if(value<_progressMin||value>_progressMax)
        return;
    _progressDraw=_progressValue;
    _progressValue=value;

    _animation->setStartValue(_progressDraw);
    _animation->setEndValue(_progressValue);
    _animation->start();
}

double CircleProgressBar::getDrawValue() const
{
    return _progressDraw;
}

void CircleProgressBar::setDrawValue(double value)
{
    _progressDraw=value;
    update();
}

void CircleProgressBar::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str=R"(#version 330 core
                           layout (location = 0) in vec2 aPos;
                           out vec2 thePos;
                           void main()
                           {
                           gl_Position = vec4(aPos, 0.0, 1.0);
                           thePos = aPos;
                           })";
    //GLSL没有atan2函数么？自己写了一个，不过最终结果为0-360度的归一化值[0,1]
    //gl_FragColor在3移除了，自己声明一个
    //aSmoothWidth用来计算平滑所需宽度，根据不同的大小来计算
    const char *fragment_str=R"(#version 330 core
                             #define PI 3.14159265
                             #define OFFSET 0.01
                             uniform float aValue;
                             uniform float aSmoothWidth;
                             in vec2 thePos;
                             out vec4 FragColor;

                             float myatan2(float y,float x)
                             {
                             float ret_val=0;

                             if(x>0){
                             ret_val=atan(y/x);
                             }

                             else if(x<0){
                             if(y>=0){
                             ret_val=PI+atan(y/x);
                             }else{
                             ret_val=-PI+atan(y/x);
                             }
                             }

                             else{
                             if(y>0){
                             ret_val =PI/2;
                             }else if(y<0){
                             ret_val=-PI/2;
                             }
                             }

                             if(ret_val<0){
                             ret_val=2*PI+ret_val;
                             }
                             return ret_val/(2*PI);
                             }

                             void main()
                             {
                             float len = abs(sqrt(pow(thePos.x,2)+pow(thePos.y,2)));
                             float alpha = 1.0-smoothstep(0.15,0.15+aSmoothWidth,abs(len-0.75));
                             float angle = myatan2(thePos.y,thePos.x);

                             if(angle<aValue){
                             FragColor = vec4(1.0,0.1,(1.0-angle),alpha);
                             }else{
                             FragColor = vec4(0.4,0.1,0.6,alpha);
                             }
                             })";


    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!_shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,vertex_str)){
        qDebug()<<"compiler vertex error"<<_shaderProgram.log();
    }
    if(!_shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,fragment_str)){
        qDebug()<<"compiler fragment error"<<_shaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!_shaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<_shaderProgram.log();
    }

    //两个三角拼接的一个矩形
    const float vertices[] = {
        -1.0f, -1.0f, //左下角
        1.0f, -1.0f, //右下角
        1.0f,  1.0f, //右上角

        1.0f,  1.0f, //右上角
        -1.0f,  1.0f, //左上角
        -1.0f, -1.0f, //左下角
    };
    _vao.create();
    _vao.bind();
    _vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    _vbo.create();
    _vbo.bind();
    _vbo.allocate(vertices,sizeof(vertices));

    // position attribute
    int attr = -1;
    attr = _shaderProgram.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    _shaderProgram.setAttributeBuffer(attr, GL_FLOAT, 0, 2, sizeof(GLfloat) * 2);
    _shaderProgram.enableAttributeArray(attr);
}

void CircleProgressBar::paintGL()
{
    //以短边为边长，保持比例，Qt这里有个问题，在resize里设置的没用
    const int item_w=width()>height()?height():width();
    glViewport((width()-item_w)/2,
               (height()-item_w)/2,
               item_w,
               item_w);

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    //基于源像素Alpha通道值的半透明混合函数
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //开启多重采样抗锯齿，貌似没啥效果
    //glEnable(GL_MULTISAMPLE);

    _shaderProgram.bind();
    //把进度[min,max]归一化[0,1]
    const float progress=(_progressDraw-_progressMin)/(_progressMax-_progressMin);
    _shaderProgram.setUniformValue("aValue", progress);
    //aSmoothWidth用来计算平滑所需宽度，根据不同的大小来计算，这里用N px的宽度
    _shaderProgram.setUniformValue("aSmoothWidth", float(3.0/item_w));
    _vao.bind();

    glDrawArrays(GL_TRIANGLES, 0, 6);

    _vao.release();
    _shaderProgram.release();

    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei",16));
    const QString text_val=QString::number(progress*100,'f',2)+" %";
    const int text_x=width()/2-painter.fontMetrics().width(text_val)/2;
    const int text_y=height()/2+painter.fontMetrics().height()/2;
    painter.drawText(text_x,text_y,text_val);
}

void CircleProgressBar::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

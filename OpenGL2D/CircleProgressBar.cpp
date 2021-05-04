#include "CircleProgressBar.h"

#include <QPainter>
#include <QDebug>

CircleProgressBar::CircleProgressBar(QWidget *parent)
    : QOpenGLWidget(parent)
{
    animation=new QPropertyAnimation(this,"drawValue");
    animation->setDuration(2000); //动画持续时间
    animation->setEasingCurve(QEasingCurve::OutQuart); //先快后慢
}

CircleProgressBar::~CircleProgressBar()
{
    makeCurrent();
    vbo.destroy();
    vao.destroy();
    doneCurrent();
}

void CircleProgressBar::setRange(double min, double max)
{
    if(progressMax<=progressMin)
        return;
    progressMin=min;
    progressMax=max;
}

void CircleProgressBar::setValue(double value)
{
    if(value<progressMin||value>progressMax)
        return;
    progressDraw=progressValue;
    progressValue=value;

    animation->setStartValue(progressDraw);
    animation->setEndValue(progressValue);
    animation->start();
}

double CircleProgressBar::getDrawValue() const
{
    return progressDraw;
}

void CircleProgressBar::setDrawValue(double value)
{
    progressDraw=value;
    update();
}

void CircleProgressBar::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //[aPos]两个三角的顶点数据
    //[thePos]表示当前像素点
    const char *vertex_str=R"(#version 330 core
                           layout (location = 0) in vec2 aPos;
                           out vec2 thePos;
                           void main()
                           {
                             gl_Position = vec4(aPos, 0.0, 1.0);
                             thePos = aPos;
                           })";
    //GLSL的atan2也叫atan，不过参数不同，我们封装一个0-360度的归一化值[0,1]的版本
    //[FragColor]该点输出颜色，gl_FragColor在3移除了，自己声明一个
    //[aValue]进度值
    //[aSmoothWidth]用来计算平滑所需宽度，根据绘制区域大小来计算
    //[len]坐标点距离圆心的距离,[0,1]，勾股定理
    //[alpha]使用smoothstep平滑函数取0.75±0.15的圆圈透明度为1
    //[angle]thePos像素点对应的角度值，用于调节渐变，归一化到[0，1]
    //[angle_smooth]进度值那条斜线取平滑
    //[ret smoothstep(a,b,x)]可以用来生成0-1的平滑过渡，达到抗锯齿效果
    //返回0: x<a<b 或者 x>a>b
    //返回1: x<b<a 或者 x>b>a
    //返回n: 根据x在ab间位置，返回[0,1]过度值
    const char *fragment_str=R"(#version 330 core
                             #define PI 3.14159265
                             uniform float aValue;
                             uniform float aSmoothWidth;
                             in vec2 thePos;
                             out vec4 FragColor;

                             float myatan2(float y,float x)
                             {
                               float ret_val = 0.0;
                               if(x != 0.0){
                                 ret_val = atan(y,x);
                                 if(ret_val < 0.0){
                                   ret_val += 2.0*PI;
                                 }
                               }else{
                                 ret_val = y>0 ? PI*0.5 : PI*1.5;
                               }
                               return ret_val/(2.0*PI);
                             }

                             void main()
                             {
                             float len = abs(sqrt(pow(thePos.x,2.0)+pow(thePos.y,2.0)));
                             float alpha = smoothstep(0.15+aSmoothWidth,0.15,abs(len-0.75));
                             float angle = myatan2(thePos.y,thePos.x);
                             float angle_smooth = smoothstep(aValue+aSmoothWidth/3.0,aValue,angle);

                             if(angle_smooth>0.0 && aValue>0.0){
                               if(angle_smooth>=1.0){
                                 FragColor = vec4(1.0,0.1,(1.0-angle),alpha);
                               }else{
                                 FragColor = vec4(mix(vec3(1.0,0.1,(1.0-angle)),vec3(0.4,0.1,0.6),1.0-angle_smooth),alpha);
                               }
                             }else{
                               FragColor = vec4(0.4,0.1,0.6,alpha);
                             }
                             })";


    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,vertex_str)){
        qDebug()<<"compiler vertex error"<<shaderProgram.log();
    }
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,fragment_str)){
        qDebug()<<"compiler fragment error"<<shaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!shaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<shaderProgram.log();
    }

    //两个三角拼接的一个矩形
    const float vertices[] = {
        -1.0f, -1.0f, //左下角
        +1.0f, -1.0f, //右下角
        +1.0f, +1.0f, //右上角

        +1.0f, +1.0f, //右上角
        -1.0f, +1.0f, //左上角
        -1.0f, -1.0f, //左下角
    };
    vao.create();
    vao.bind();
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));

    // position attribute
    int attr = -1;
    attr = shaderProgram.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    shaderProgram.setAttributeBuffer(attr, GL_FLOAT, 0, 2, sizeof(GLfloat) * 2);
    shaderProgram.enableAttributeArray(attr);
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

    shaderProgram.bind();
    //把进度[min,max]归一化[0,1]
    const float progress=(progressDraw-progressMin)/(progressMax-progressMin);
    //qDebug()<<"draw progress"<<progress;
    shaderProgram.setUniformValue("aValue", progress);
    //aSmoothWidth用来计算平滑所需宽度，根据不同的大小来计算，这里用N px的宽度
    shaderProgram.setUniformValue("aSmoothWidth", float(3.0/item_w));
    vao.bind();

    glDrawArrays(GL_TRIANGLES, 0, 6);

    vao.release();
    shaderProgram.release();

    //目前文字用QPainter绘制
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
    //以短边为边长，保持比例，Qt这里有个问题，在resize里设置的没用
    const int item_w=width>height?height:width;
    glViewport((width-item_w)/2,
               (height-item_w)/2,
               item_w,
               item_w);
}

#include "WaveProgressBar.h"

#include <QPainter>
#include <QDebug>

WaveProgressBar::WaveProgressBar(QWidget *parent)
    : QOpenGLWidget(parent)
{
    animation=new QPropertyAnimation(this,"drawValue");
    animation->setDuration(2000); //动画持续时间
    animation->setEasingCurve(QEasingCurve::OutQuart); //先快后慢

    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,[this]{
        if(isHidden())
            return;
        //暂时没有考虑周期
        timeValue+=2;
        if(timeValue>=360)
            timeValue=0;
        update();
    });
    timer->start(30);
}

WaveProgressBar::~WaveProgressBar()
{
    //显示后才会执行初始化
    if(!isValid())
        return;
    makeCurrent();
    vbo.destroy();
    vao.destroy();
    doneCurrent();
}

void WaveProgressBar::setRange(double min, double max)
{
    if(progressMax<=progressMin)
        return;
    progressMin=min;
    progressMax=max;
}

void WaveProgressBar::setValue(double value)
{
    if(value<progressMin||value>progressMax)
        return;
    progressDraw=progressValue;
    progressValue=value;

    animation->setStartValue(progressDraw);
    animation->setEndValue(progressValue);
    animation->start();
}

double WaveProgressBar::getDrawValue() const
{
    return progressDraw;
}

void WaveProgressBar::setDrawValue(double value)
{
    progressDraw=value;
    update();
}

void WaveProgressBar::initializeGL()
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
    //[aTime]时间偏移
    //[aSmoothWidth]平滑过渡宽度
    const char *fragment_str=R"(#version 330 core
                             #define PI 3.14159265
                             uniform float aValue;
                             uniform float aTime;
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
                             float alpha = smoothstep(0.6+aSmoothWidth,0.6,len);

                             FragColor = vec4(aValue,0.2,1.0,alpha);
                             if(alpha<=0){
                               FragColor.a = smoothstep(0.05+aSmoothWidth,0.05,abs(len-0.75));
                               float angle = myatan2(thePos.y,thePos.x);
                               float angle_diff=abs(angle-aTime);
                               if(angle_diff>0.5) angle_diff=1.0-angle_diff;
                               float g_smooth = smoothstep(0,aSmoothWidth/2.0,abs(angle_diff-0.25));
                               if(angle_diff<=0.25){
                                 FragColor.g = 0.2+0.4*g_smooth;
                               }
                             }else{
                               float posY = (thePos.y+0.6)/1.2;
                               float yTop1 = posY+0.03*sin(-aTime*10*PI+thePos.x*10)-0.02;
                               float ySmooth1 = smoothstep(yTop1,yTop1+aSmoothWidth,aValue);
                               float yTop2 = posY+0.03*sin(aTime*10*PI+thePos.x*10);
                               float ySmooth2 = smoothstep(yTop2,yTop2+aSmoothWidth,aValue);

                               if(ySmooth1>0&&ySmooth2<0.8){
                                 FragColor.g += 0.4*ySmooth1;
                                 if(ySmooth2>0) FragColor.g += 0.2*ySmooth2;
                               }else if(ySmooth2>0){
                                 FragColor.g += 0.6*ySmooth2;
                               }
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

void WaveProgressBar::paintGL()
{
    //以短边为边长，保持比例，Qt这里有个问题，在resize里设置的没用
    const int item_w=width()>height()?height():width();
    glViewport((width()-item_w)/2,
               (height()-item_w)/2,
               item_w,
               item_w);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
    //时间偏移移动
    shaderProgram.setUniformValue("aTime", float(timeValue/360.0));
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

void WaveProgressBar::resizeGL(int width, int height)
{
    //以短边为边长，保持比例，Qt这里有个问题，在resize里设置的没用
    const int item_w=width>height?height:width;
    glViewport((width-item_w)/2,
               (height-item_w)/2,
               item_w,
               item_w);
}

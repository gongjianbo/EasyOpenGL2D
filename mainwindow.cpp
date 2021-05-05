#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initTabA();
    initTabB();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initTabA()
{
    //默认百分之45
    ui->boxCircleValue->setValue(45);
    ui->glCirlcleProgress->setValue(45);
    //调节进度
    connect(ui->btnCircleSet,&QPushButton::clicked,this,[=]{
        ui->glCirlcleProgress->setValue(ui->boxCircleValue->value());
    });
}

void MainWindow::initTabB()
{
    //默认百分之45
    ui->boxWaveValue->setValue(45);
    ui->glWaveProgress->setValue(45);
    //调节进度
    connect(ui->btnWaveSet,&QPushButton::clicked,this,[=]{
        ui->glWaveProgress->setValue(ui->boxWaveValue->value());
    });
}


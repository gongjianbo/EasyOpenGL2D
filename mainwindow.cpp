#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initTabA();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initTabA()
{
    ui->boxProgressValue->setValue(45);
    ui->glCirlcleProgress->setValue(45);
    connect(ui->btnProgressSet,&QPushButton::clicked,this,[=]{
        ui->glCirlcleProgress->setValue(ui->boxProgressValue->value());
    });
}


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //a 环形进度条
    void initTabA();
    //b 波浪进度球
    void initTabB();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H

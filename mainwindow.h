#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Declare slot functions for Menu Bar actions

    // View
    void on_actionBasic_triggered();
    void on_actionScientific_triggered();
    void on_actionProgrammer_triggered();

    // Converter
    void openConverter();

    // Settings
    void openSettings();

    // Help
    void on_actionHelp_triggered();
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H

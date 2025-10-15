#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDebug> // for outputting debug messages

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create Interactable QAction from QMenu placeholders
    QMenu* converterMenuPlaceholder = ui->menuConverter;
    QMenu* settingsMenuPlaceholder = ui->menuSettings;

    if (converterMenuPlaceholder) {
        QAction* converterAction = new QAction(converterMenuPlaceholder->title(), this);
        menuBar()->insertAction(converterMenuPlaceholder->menuAction(), converterAction);
        connect(converterAction, &QAction::triggered, this, &MainWindow::openConverter);
        menuBar()->removeAction(converterMenuPlaceholder->menuAction());
        converterMenuPlaceholder->deleteLater(); // Use deleteLater for safety
    }
    if (settingsMenuPlaceholder) {
        QAction* settingsAction = new QAction(settingsMenuPlaceholder->title(), this);
        menuBar()->insertAction(settingsMenuPlaceholder->menuAction(), settingsAction);
        connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
        menuBar()->removeAction(settingsMenuPlaceholder->menuAction());
        settingsMenuPlaceholder->deleteLater(); // Use deleteLater for safety
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Function Implementation for when Menu Bar actions are triggered

// View
void MainWindow::on_actionBasic_triggered()
{
    qDebug() << "Switching to Basic View...";
    // Add your custom logic here. For example:
    // ui->label->setText("Button clicked!");
}
void MainWindow::on_actionScientific_triggered()
{
    qDebug() << "Switching to Scientific View...";
}
void MainWindow::on_actionProgrammer_triggered()
{
    qDebug() << "Switching to Programmer View...";
}

// Converter
void MainWindow::openConverter()
{
    qDebug() << "Opening Converter...";
}

// Settings
void MainWindow::openSettings()
{
    qDebug() << "Opening Settings...";
}

// Help
void MainWindow::on_actionHelp_triggered()
{
    qDebug() << "Opening Help...";
}
void MainWindow::on_actionAbout_triggered()
{
    qDebug() << "Opening About Section...";
}

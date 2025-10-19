#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug> // for outputting debug messages

#include "converter.h" // Include header for Converter form
#include "settings.h" // Include header for Settings form
#include "help.h" // Include header for Help form
#include "about.h" // Include header for About form

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

    // Connect the calculator's buttons to on-click functions
    QObject::connect(ui->ac, &QPushButton::clicked, this, &MainWindow::on_button_ac_clicked);

    // TODO: Copy and Paste the above function for all other buttons (and interactable elements)
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Function Implementation for when Menu Bar actions are triggered

// View
void MainWindow::on_actionBasic_triggered()
{
    // Switch to Basic View
    ui->calculator_views->setCurrentIndex(0);
}
void MainWindow::on_actionScientific_triggered()
{
    // Switch to Scientific View
    ui->calculator_views->setCurrentIndex(1);
}
void MainWindow::on_actionProgrammer_triggered()
{
    // Switch to Programmer View
    ui->calculator_views->setCurrentIndex(2);
}

// Converter
void MainWindow::openConverter()
{
    // Create an instance of the Converter form
    converter* converter_form = new converter(this); 
    // ... and show it on screen
    converter_form->show();
}

// Settings
void MainWindow::openSettings()
{
    // Create an instance of the Settings form
    settings* settings_form = new settings(this);
    // ... and show it on screen
    settings_form->show();
}

// Help
void MainWindow::on_actionHelp_triggered()
{
    // Create an instance of the Help form
    help* help_form = new help(this);
    // ... and show it on screen
    help_form->show();
}
void MainWindow::on_actionAbout_triggered()
{
    // Create an instance of the About form
    about* about_form = new about(this);
    // ... and show it on screen
    about_form->show();
}

// Functions to handle the Calculator Buttons' input

// Buttons in Scientific View
void MainWindow::on_button_ac_clicked() {
    // DEBUG
    qDebug() << "Button AC clicked!";
}
void MainWindow::on_button_add_clicked() {
}
void MainWindow::on_button_ans_clicked() {
}
void MainWindow::on_button_backspace_clicked() {
}
void MainWindow::on_button_decimal_point_clicked() {
}
void MainWindow::on_button_divide_clicked() {
}
void MainWindow::on_button_equals_clicked() {
}
void MainWindow::on_button_multiply_clicked() {
}
void MainWindow::on_button_n0_clicked() {
}
void MainWindow::on_button_n1_clicked() {
}
void MainWindow::on_button_n2_clicked() {
}
void MainWindow::on_button_n3_clicked() {
}
void MainWindow::on_button_n4_clicked() {
}
void MainWindow::on_button_n5_clicked() {
}
void MainWindow::on_button_n6_clicked() {
}
void MainWindow::on_button_n7_clicked() {
}
void MainWindow::on_button_n8_clicked() {
}
void MainWindow::on_button_n9_clicked() {
}
void MainWindow::on_button_negate_clicked() {
}
void MainWindow::on_button_subtract_clicked() {
}

void MainWindow::on_button_absolute_value_clicked() {
}
void MainWindow::on_button_constant_e_clicked() {
}
void MainWindow::on_button_constant_pi_clicked() {
}
void MainWindow::on_button_cosine_clicked() {
}
void MainWindow::on_button_exponent_scientific_clicked() {
}
void MainWindow::on_button_exponential_clicked() {
}
void MainWindow::on_button_exponential_base10_clicked() {
}
void MainWindow::on_button_exponential_natural_clicked() {
}
void MainWindow::on_button_factorial_clicked() {
}
void MainWindow::on_button_hyp_cosine_clicked() {
}
void MainWindow::on_button_hyp_sine_clicked() {
}
void MainWindow::on_button_hyp_tangent_clicked() {
}
void MainWindow::on_button_inverse_cosine_clicked() {
}
void MainWindow::on_button_inverse_hyp_cosine_clicked() {
}
void MainWindow::on_button_inverse_hyp_sine_clicked() {
}
void MainWindow::on_button_inverse_hyp_tangent_clicked() {
}
void MainWindow::on_button_inverse_sine_clicked() {
}
void MainWindow::on_button_inverse_tangent_clicked() {
}
void MainWindow::on_button_logarithm_common_clicked() {
}
void MainWindow::on_button_logarithm_natural_clicked() {
}
void MainWindow::on_button_memory_add_clicked() {
}
void MainWindow::on_button_memory_clear_clicked() {
}
void MainWindow::on_button_memory_recall_clicked() {
}
void MainWindow::on_button_memory_subtract_clicked() {
}
void MainWindow::on_button_modulus_clicked() {
}
void MainWindow::on_button_parentheses_left_clicked() {
}
void MainWindow::on_button_parentheses_right_clicked() {
}
void MainWindow::on_button_percent_clicked() {
}
void MainWindow::on_button_random_number_clicked() {
}
void MainWindow::on_button_reciprocal_clicked() {
}
void MainWindow::on_button_sine_clicked() {
}
void MainWindow::on_button_square_clicked() {
}
void MainWindow::on_button_square_root_clicked() {
}
void MainWindow::on_button_tangent_clicked() {
}
void MainWindow::on_button_x_th_root_clicked() {
}

// TODO: Add Basic and Programmer View buttons here
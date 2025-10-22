#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug> // for outputting debug messages

#include "converter.h" // Include header for Converter form
#include "settings.h" // Include header for Settings form
#include "help.h" // Include header for Help form
#include "about.h" // Include header for About form

#include <gmp.h> // to handle the Arithmetic

#include <string> // Some other helpful dependencies
#include <vector>

// Initialize important variables globally
std::vector<std::string> equation_buffer;
std::vector<std::string> equation_display;
std::vector<std::string> numeric_input_buffer;
bool dp_used = false;
bool number_is_negative = false;
bool new_number = true;

// Define important functions
std::string concat_numeric_input_buffer_content() {
    std::string output = "";
    for (std::string c : numeric_input_buffer) {
        output += c;
    }
    return output;
}

std::string concat_equation_buffer_content() {
    std::string output = "";
    for (std::string c : equation_buffer) {
        output += c;
    }
    return output;
}

// DEBUG: output numeric input buffer and equation buffer
void input_dbg() {
    qDebug() << "Numeric Input Buffer: ";
    qDebug() << concat_numeric_input_buffer_content();
    qDebug() << "Equation Buffer: ";
    qDebug() << concat_equation_buffer_content();
    qDebug() << "\n";
}

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

    // Buttons in Scientific View

    QObject::connect(ui->ac, &QPushButton::clicked, this, &MainWindow::on_button_ac_clicked);
    QObject::connect(ui->add, &QPushButton::clicked, this, &MainWindow::on_button_add_clicked);
    QObject::connect(ui->ans, &QPushButton::clicked, this, &MainWindow::on_button_ans_clicked);
    QObject::connect(ui->backspace, &QPushButton::clicked, this, &MainWindow::on_button_backspace_clicked);
    QObject::connect(ui->decimal_point, &QPushButton::clicked, this, &MainWindow::on_button_decimal_point_clicked);
    QObject::connect(ui->divide, &QPushButton::clicked, this, &MainWindow::on_button_divide_clicked);
    QObject::connect(ui->equals, &QPushButton::clicked, this, &MainWindow::on_button_equals_clicked);
    QObject::connect(ui->multiply, &QPushButton::clicked, this, &MainWindow::on_button_multiply_clicked);
    QObject::connect(ui->n0, &QPushButton::clicked, this, &MainWindow::on_button_n0_clicked);
    QObject::connect(ui->n1, &QPushButton::clicked, this, &MainWindow::on_button_n1_clicked);
    QObject::connect(ui->n2, &QPushButton::clicked, this, &MainWindow::on_button_n2_clicked);
    QObject::connect(ui->n3, &QPushButton::clicked, this, &MainWindow::on_button_n3_clicked);
    QObject::connect(ui->n4, &QPushButton::clicked, this, &MainWindow::on_button_n4_clicked);
    QObject::connect(ui->n5, &QPushButton::clicked, this, &MainWindow::on_button_n5_clicked);
    QObject::connect(ui->n6, &QPushButton::clicked, this, &MainWindow::on_button_n6_clicked);
    QObject::connect(ui->n7, &QPushButton::clicked, this, &MainWindow::on_button_n7_clicked);
    QObject::connect(ui->n8, &QPushButton::clicked, this, &MainWindow::on_button_n8_clicked);
    QObject::connect(ui->n9, &QPushButton::clicked, this, &MainWindow::on_button_n9_clicked);
    QObject::connect(ui->negate, &QPushButton::clicked, this, &MainWindow::on_button_negate_clicked);
    QObject::connect(ui->subtract, &QPushButton::clicked, this, &MainWindow::on_button_subtract_clicked);

    QObject::connect(ui->memory_add, &QPushButton::clicked, this, &MainWindow::on_button_memory_add_clicked);
    QObject::connect(ui->memory_clear, &QPushButton::clicked, this, &MainWindow::on_button_memory_clear_clicked);
    QObject::connect(ui->memory_recall, &QPushButton::clicked, this, &MainWindow::on_button_memory_recall_clicked);
    QObject::connect(ui->memory_subtract, &QPushButton::clicked, this, &MainWindow::on_button_memory_subtract_clicked);
    QObject::connect(ui->parentheses_left, &QPushButton::clicked, this, &MainWindow::on_button_parentheses_left_clicked);
    QObject::connect(ui->parentheses_right, &QPushButton::clicked, this, &MainWindow::on_button_parentheses_right_clicked);

    QObject::connect(ui->angleUnitSelection, &QComboBox::activated, this, &MainWindow::on_angleUnitSelection_activated);

    QObject::connect(ui->absolute_value, &QPushButton::clicked, this, &MainWindow::on_button_absolute_value_clicked);
    QObject::connect(ui->constant_e, &QPushButton::clicked, this, &MainWindow::on_button_constant_e_clicked);
    QObject::connect(ui->constant_pi, &QPushButton::clicked, this, &MainWindow::on_button_constant_pi_clicked);
    QObject::connect(ui->cosine, &QPushButton::clicked, this, &MainWindow::on_button_cosine_clicked);
    QObject::connect(ui->exponent_scientific, &QPushButton::clicked, this, &MainWindow::on_button_exponent_scientific_clicked);
    QObject::connect(ui->exponential, &QPushButton::clicked, this, &MainWindow::on_button_exponential_clicked);
    QObject::connect(ui->exponential_base10, &QPushButton::clicked, this, &MainWindow::on_button_exponential_base10_clicked);
    QObject::connect(ui->exponential_natural, &QPushButton::clicked, this, &MainWindow::on_button_exponential_natural_clicked);
    QObject::connect(ui->factorial, &QPushButton::clicked, this, &MainWindow::on_button_factorial_clicked);
    QObject::connect(ui->hyp_cosine, &QPushButton::clicked, this, &MainWindow::on_button_hyp_cosine_clicked);
    QObject::connect(ui->hyp_sine, &QPushButton::clicked, this, &MainWindow::on_button_hyp_sine_clicked);
    QObject::connect(ui->hyp_tangent, &QPushButton::clicked, this, &MainWindow::on_button_hyp_tangent_clicked);
    QObject::connect(ui->inverse_cosine, &QPushButton::clicked, this, &MainWindow::on_button_inverse_cosine_clicked);
    QObject::connect(ui->inverse_hyp_cosine, &QPushButton::clicked, this, &MainWindow::on_button_inverse_hyp_cosine_clicked);
    QObject::connect(ui->inverse_hyp_sine, &QPushButton::clicked, this, &MainWindow::on_button_inverse_hyp_sine_clicked);
    QObject::connect(ui->inverse_hyp_tangent, &QPushButton::clicked, this, &MainWindow::on_button_inverse_hyp_tangent_clicked);
    QObject::connect(ui->inverse_sine, &QPushButton::clicked, this, &MainWindow::on_button_inverse_sine_clicked);
    QObject::connect(ui->inverse_tangent, &QPushButton::clicked, this, &MainWindow::on_button_inverse_tangent_clicked);
    QObject::connect(ui->logarithm_common, &QPushButton::clicked, this, &MainWindow::on_button_logarithm_common_clicked);
    QObject::connect(ui->logarithm_natural, &QPushButton::clicked, this, &MainWindow::on_button_logarithm_natural_clicked);
    QObject::connect(ui->modulus, &QPushButton::clicked, this, &MainWindow::on_button_modulus_clicked);
    QObject::connect(ui->percent, &QPushButton::clicked, this, &MainWindow::on_button_percent_clicked);
    QObject::connect(ui->random_number, &QPushButton::clicked, this, &MainWindow::on_button_random_number_clicked);
    QObject::connect(ui->reciprocal, &QPushButton::clicked, this, &MainWindow::on_button_reciprocal_clicked);
    QObject::connect(ui->sine, &QPushButton::clicked, this, &MainWindow::on_button_sine_clicked);
    QObject::connect(ui->square, &QPushButton::clicked, this, &MainWindow::on_button_square_clicked);
    QObject::connect(ui->square_root, &QPushButton::clicked, this, &MainWindow::on_button_square_root_clicked);
    QObject::connect(ui->tangent, &QPushButton::clicked, this, &MainWindow::on_button_tangent_clicked);
    QObject::connect(ui->x_th_root, &QPushButton::clicked, this, &MainWindow::on_button_x_th_root_clicked);

    // Basic View

    QObject::connect(ui->ac_2, &QPushButton::clicked, this, &MainWindow::on_button_ac_clicked);
    QObject::connect(ui->add_2, &QPushButton::clicked, this, &MainWindow::on_button_add_clicked);
    QObject::connect(ui->ans_2, &QPushButton::clicked, this, &MainWindow::on_button_ans_clicked);
    QObject::connect(ui->backspace_2, &QPushButton::clicked, this, &MainWindow::on_button_backspace_clicked);
    QObject::connect(ui->decimal_point_2, &QPushButton::clicked, this, &MainWindow::on_button_decimal_point_clicked);
    QObject::connect(ui->divide_2, &QPushButton::clicked, this, &MainWindow::on_button_divide_clicked);
    QObject::connect(ui->equals_2, &QPushButton::clicked, this, &MainWindow::on_button_equals_clicked);
    QObject::connect(ui->multiply_2, &QPushButton::clicked, this, &MainWindow::on_button_multiply_clicked);
    QObject::connect(ui->n0_2, &QPushButton::clicked, this, &MainWindow::on_button_n0_clicked);
    QObject::connect(ui->n1_2, &QPushButton::clicked, this, &MainWindow::on_button_n1_clicked);
    QObject::connect(ui->n2_2, &QPushButton::clicked, this, &MainWindow::on_button_n2_clicked);
    QObject::connect(ui->n3_2, &QPushButton::clicked, this, &MainWindow::on_button_n3_clicked);
    QObject::connect(ui->n4_2, &QPushButton::clicked, this, &MainWindow::on_button_n4_clicked);
    QObject::connect(ui->n5_2, &QPushButton::clicked, this, &MainWindow::on_button_n5_clicked);
    QObject::connect(ui->n6_2, &QPushButton::clicked, this, &MainWindow::on_button_n6_clicked);
    QObject::connect(ui->n7_2, &QPushButton::clicked, this, &MainWindow::on_button_n7_clicked);
    QObject::connect(ui->n8_2, &QPushButton::clicked, this, &MainWindow::on_button_n8_clicked);
    QObject::connect(ui->n9_2, &QPushButton::clicked, this, &MainWindow::on_button_n9_clicked);
    QObject::connect(ui->negate_2, &QPushButton::clicked, this, &MainWindow::on_button_negate_clicked);
    QObject::connect(ui->subtract_2, &QPushButton::clicked, this, &MainWindow::on_button_subtract_clicked);

    QObject::connect(ui->memory_add_2, &QPushButton::clicked, this, &MainWindow::on_button_memory_add_clicked);
    QObject::connect(ui->memory_clear_2, &QPushButton::clicked, this, &MainWindow::on_button_memory_clear_clicked);
    QObject::connect(ui->memory_recall_2, &QPushButton::clicked, this, &MainWindow::on_button_memory_recall_clicked);
    QObject::connect(ui->memory_subtract_2, &QPushButton::clicked, this, &MainWindow::on_button_memory_subtract_clicked);
    QObject::connect(ui->parentheses_left_2, &QPushButton::clicked, this, &MainWindow::on_button_parentheses_left_clicked);
    QObject::connect(ui->parentheses_right_2, &QPushButton::clicked, this, &MainWindow::on_button_parentheses_right_clicked);

    QObject::connect(ui->absolute_value_2, & QPushButton::clicked, this, &MainWindow::on_button_absolute_value_clicked);
    QObject::connect(ui->exponential_2, &QPushButton::clicked, this, &MainWindow::on_button_exponential_clicked);
    QObject::connect(ui->reciprocal_2, &QPushButton::clicked, this, &MainWindow::on_button_reciprocal_clicked);
    QObject::connect(ui->square_2, &QPushButton::clicked, this, &MainWindow::on_button_square_clicked);
    QObject::connect(ui->square_root_2, &QPushButton::clicked, this, &MainWindow::on_button_square_root_clicked);
    QObject::connect(ui->x_th_root_2, &QPushButton::clicked, this, &MainWindow::on_button_x_th_root_clicked);

    // Programmer View
    
    QObject::connect(ui->ac_3, &QPushButton::clicked, this, &MainWindow::on_button_ac_clicked);
    QObject::connect(ui->add_3, &QPushButton::clicked, this, &MainWindow::on_button_add_clicked);
    QObject::connect(ui->ans_3, &QPushButton::clicked, this, &MainWindow::on_button_ans_clicked);
    QObject::connect(ui->backspace_3, &QPushButton::clicked, this, &MainWindow::on_button_backspace_clicked);
    QObject::connect(ui->decimal_point_3, &QPushButton::clicked, this, &MainWindow::on_button_decimal_point_clicked);
    QObject::connect(ui->divide_3, &QPushButton::clicked, this, &MainWindow::on_button_divide_clicked);
    QObject::connect(ui->equals_3, &QPushButton::clicked, this, &MainWindow::on_button_equals_clicked);
    QObject::connect(ui->multiply_3, &QPushButton::clicked, this, &MainWindow::on_button_multiply_clicked);
    QObject::connect(ui->n0_3, &QPushButton::clicked, this, &MainWindow::on_button_n0_clicked);
    QObject::connect(ui->n1_3, &QPushButton::clicked, this, &MainWindow::on_button_n1_clicked);
    QObject::connect(ui->n2_3, &QPushButton::clicked, this, &MainWindow::on_button_n2_clicked);
    QObject::connect(ui->n3_3, &QPushButton::clicked, this, &MainWindow::on_button_n3_clicked);
    QObject::connect(ui->n4_3, &QPushButton::clicked, this, &MainWindow::on_button_n4_clicked);
    QObject::connect(ui->n5_3, &QPushButton::clicked, this, &MainWindow::on_button_n5_clicked);
    QObject::connect(ui->n6_3, &QPushButton::clicked, this, &MainWindow::on_button_n6_clicked);
    QObject::connect(ui->n7_3, &QPushButton::clicked, this, &MainWindow::on_button_n7_clicked);
    QObject::connect(ui->n8_3, &QPushButton::clicked, this, &MainWindow::on_button_n8_clicked);
    QObject::connect(ui->n9_3, &QPushButton::clicked, this, &MainWindow::on_button_n9_clicked);
    QObject::connect(ui->negate_3, &QPushButton::clicked, this, &MainWindow::on_button_negate_clicked);
    QObject::connect(ui->subtract_3, &QPushButton::clicked, this, &MainWindow::on_button_subtract_clicked);

    QObject::connect(ui->memory_add_3, &QPushButton::clicked, this, &MainWindow::on_button_memory_add_clicked);
    QObject::connect(ui->memory_clear_3, &QPushButton::clicked, this, &MainWindow::on_button_memory_clear_clicked);
    QObject::connect(ui->memory_recall_3, &QPushButton::clicked, this, &MainWindow::on_button_memory_recall_clicked);
    QObject::connect(ui->memory_subtract_3, &QPushButton::clicked, this, &MainWindow::on_button_memory_subtract_clicked);
    QObject::connect(ui->parentheses_left_3, &QPushButton::clicked, this, &MainWindow::on_button_parentheses_left_clicked);
    QObject::connect(ui->parentheses_right_3, &QPushButton::clicked, this, &MainWindow::on_button_parentheses_right_clicked);

    QObject::connect(ui->ascii, &QPushButton::clicked, this, &MainWindow::on_button_ascii_clicked);
    QObject::connect(ui->base_bin, &QPushButton::clicked, this, &MainWindow::on_button_base_bin_clicked);
    QObject::connect(ui->base_dec, &QPushButton::clicked, this, &MainWindow::on_button_base_dec_clicked);
    QObject::connect(ui->base_hex, &QPushButton::clicked, this, &MainWindow::on_button_base_hex_clicked);
    QObject::connect(ui->base_oct, &QPushButton::clicked, this, &MainWindow::on_button_base_oct_clicked);
    QObject::connect(ui->bit_rotate_left, &QPushButton::clicked, this, &MainWindow::on_button_bit_rotate_left_clicked);
    QObject::connect(ui->bit_rotate_right, &QPushButton::clicked, this, &MainWindow::on_button_bit_rotate_right_clicked);
    QObject::connect(ui->bit_shift_left, &QPushButton::clicked, this, &MainWindow::on_button_bit_shift_left_clicked);
    QObject::connect(ui->bit_shift_right, &QPushButton::clicked, this, &MainWindow::on_button_bit_shift_right_clicked);
    QObject::connect(ui->bitwise_and, &QPushButton::clicked, this, &MainWindow::on_button_bitwise_and_clicked);
    QObject::connect(ui->bitwise_not, &QPushButton::clicked, this, &MainWindow::on_button_bitwise_not_clicked);
    QObject::connect(ui->bitwise_or, &QPushButton::clicked, this, &MainWindow::on_button_bitwise_or_clicked);
    QObject::connect(ui->bitwise_xor, &QPushButton::clicked, this, &MainWindow::on_button_bitwise_xor_clicked);
    QObject::connect(ui->conv, &QPushButton::clicked, this, &MainWindow::on_button_conv_clicked);
    QObject::connect(ui->modulus_2, &QPushButton::clicked, this, &MainWindow::on_button_modulus_2_clicked);
    QObject::connect(ui->unicode, &QPushButton::clicked, this, &MainWindow::on_button_unicode_clicked);
    QObject::connect(ui->i_a, &QPushButton::clicked, this, &MainWindow::on_button_i_a_clicked);
    QObject::connect(ui->i_b, &QPushButton::clicked, this, &MainWindow::on_button_i_b_clicked);
    QObject::connect(ui->i_c, &QPushButton::clicked, this, &MainWindow::on_button_i_c_clicked);
    QObject::connect(ui->i_d, &QPushButton::clicked, this, &MainWindow::on_button_i_d_clicked);
    QObject::connect(ui->i_e, &QPushButton::clicked, this, &MainWindow::on_button_i_e_clicked);
    QObject::connect(ui->i_f, &QPushButton::clicked, this, &MainWindow::on_button_i_f_clicked);

    QObject::connect(ui->wordlen_word, &QRadioButton::toggled, this, &MainWindow::on_wordlen_toggled);
    QObject::connect(ui->wordlen_dword, &QRadioButton::toggled, this, &MainWindow::on_wordlen_toggled);
    QObject::connect(ui->wordlen_qword, &QRadioButton::toggled, this, &MainWindow::on_wordlen_toggled);
    QObject::connect(ui->wordlen_byte, &QRadioButton::toggled, this, &MainWindow::on_wordlen_toggled);

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

// Buttons in all Views

void MainWindow::on_button_ac_clicked() {
    numeric_input_buffer = {"0"};
    equation_buffer = {};
    dp_used = false;
    new_number = true;
    input_dbg();
}
void MainWindow::on_button_add_clicked() {
    // logic for appending numeric_input_buffer contents to equation
    // and clearing numeric_input_buffer
    for (std::string c : numeric_input_buffer) {
        equation_buffer.push_back(c);
    }
    equation_buffer.push_back("+");
    dp_used = false;
    new_number = true;
    input_dbg();
}
void MainWindow::on_button_ans_clicked() {
}
void MainWindow::on_button_backspace_clicked() {
    std::string last = numeric_input_buffer.back();
    numeric_input_buffer.pop_back();
    if (last == ".") {
        dp_used = false;
    }
    if (numeric_input_buffer.empty()) {
        numeric_input_buffer = {"0"};
        new_number = true;
    }
    input_dbg();
}
void MainWindow::on_button_decimal_point_clicked() {
    if (dp_used == false) {
        if (new_number) {
            numeric_input_buffer = {};
            numeric_input_buffer.push_back("0");
            new_number = false;
        }
        numeric_input_buffer.push_back(".");
        dp_used = true;
    }
    input_dbg();
}
void MainWindow::on_button_divide_clicked() {
    // logic for appending numeric_input_buffer contents to equation
    // and clearing numeric_input_buffer
    for (std::string c : numeric_input_buffer) {
        equation_buffer.push_back(c);
    }
    equation_buffer.push_back("/");
    dp_used = false;
    new_number = true;
    input_dbg();
}
void MainWindow::on_button_equals_clicked() {
    // TODO: Execute the equation
}
void MainWindow::on_button_multiply_clicked() {
    // logic for appending numeric_input_buffer contents to equation
    // and clearing numeric_input_buffer
    for (std::string c : numeric_input_buffer) {
        equation_buffer.push_back(c);
    }
    equation_buffer.push_back("*");
    dp_used = false;
    new_number = true;
    input_dbg();
}
void MainWindow::on_button_n0_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("0");
    input_dbg();
}
void MainWindow::on_button_n1_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("1");
    input_dbg();
}
void MainWindow::on_button_n2_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("2");
    input_dbg();
}
void MainWindow::on_button_n3_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("3");
    input_dbg();
}
void MainWindow::on_button_n4_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("4");
    input_dbg();
}
void MainWindow::on_button_n5_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("5");
    input_dbg();
}
void MainWindow::on_button_n6_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("6");
    input_dbg();
}
void MainWindow::on_button_n7_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("7");
    input_dbg();
}
void MainWindow::on_button_n8_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("8");
    input_dbg();
}
void MainWindow::on_button_n9_clicked() {
    if (new_number) {
        numeric_input_buffer = {};
        new_number = false;
    }
    numeric_input_buffer.push_back("9");
    input_dbg();
}
void MainWindow::on_button_negate_clicked() {
    if (number_is_negative == false) {
        numeric_input_buffer.insert(numeric_input_buffer.begin(), "-");
        number_is_negative = true;
        input_dbg();
    } else {
        numeric_input_buffer.erase(numeric_input_buffer.begin());
        number_is_negative = false;
        input_dbg();
    }
}
void MainWindow::on_button_subtract_clicked() {
    // logic for appending numeric_input_buffer contents to equation
    // and clearing numeric_input_buffer
    for (std::string c : numeric_input_buffer) {
        equation_buffer.push_back(c);
    }
    equation_buffer.push_back("-");
    dp_used = false;
    new_number = true;
    input_dbg();
}

void MainWindow::on_button_memory_add_clicked() {
}
void MainWindow::on_button_memory_clear_clicked() {
}
void MainWindow::on_button_memory_recall_clicked() {
}
void MainWindow::on_button_memory_subtract_clicked() {
}
void MainWindow::on_button_parentheses_left_clicked() {
}
void MainWindow::on_button_parentheses_right_clicked() {
}

// Buttons only in Scientific View

void MainWindow::on_angleUnitSelection_activated(int i) {
    // TODO: add angle unit selection logic here
    /*
        i=0 is Degrees  (Deg)
        i=1 is Radians  (Rad)
        i=2 is Gradians (Grad)
    */
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
void MainWindow::on_button_modulus_clicked() {
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

// Buttons only in Programmer View

void MainWindow::on_button_ascii_clicked() {
}
void MainWindow::on_button_base_bin_clicked() {
}
void MainWindow::on_button_base_dec_clicked() {
}
void MainWindow::on_button_base_hex_clicked() {
}
void MainWindow::on_button_base_oct_clicked() {
}
void MainWindow::on_button_bit_rotate_left_clicked() {
}
void MainWindow::on_button_bit_rotate_right_clicked() {
}
void MainWindow::on_button_bit_shift_left_clicked() {
}
void MainWindow::on_button_bit_shift_right_clicked() {
}
void MainWindow::on_button_bitwise_and_clicked() {
}
void MainWindow::on_button_bitwise_not_clicked() {
}
void MainWindow::on_button_bitwise_or_clicked() {
}
void MainWindow::on_button_bitwise_xor_clicked() {
}
void MainWindow::on_button_conv_clicked() {
}
void MainWindow::on_button_modulus_2_clicked() {
}
void MainWindow::on_button_unicode_clicked() {
}
void MainWindow::on_button_i_a_clicked() {
}
void MainWindow::on_button_i_b_clicked() {
}
void MainWindow::on_button_i_c_clicked() {
}
void MainWindow::on_button_i_d_clicked() {
}
void MainWindow::on_button_i_e_clicked() {
}
void MainWindow::on_button_i_f_clicked() {
}

void MainWindow::on_wordlen_toggled() {
    // TODO: add Word length selection logic here
    /*
        if:
        wordlen_word->isChecked(): Word length is WORD, 16 bits
        wordlen_dword->isChecked(): Word length is DWORD, 32 bits
        wordlen_qword->isChecked(): Word length is QWORD, 64 bits
        wordlen_byte->isChecked(): Word length is BYTE, 8 bits
    */
}

// THIS IS THE ACTUAL ARITHMETIC HANDLING PART OF THE NUMERIC ENGINE
// or maybe not...

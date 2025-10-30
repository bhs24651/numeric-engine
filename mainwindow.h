#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <gmp.h>
#include <gmpxx.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
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

    // Declare slot functions for Calculator Buttons

    // Buttons in all Views

    void on_button_ac_clicked();
    void on_button_add_clicked();
    void on_button_ans_clicked();
    void on_button_backspace_clicked();
    void on_button_decimal_point_clicked();
    void on_button_divide_clicked();
    void on_button_equals_clicked();
    void on_button_multiply_clicked();
    void on_button_n0_clicked();
    void on_button_n1_clicked();
    void on_button_n2_clicked();
    void on_button_n3_clicked();
    void on_button_n4_clicked();
    void on_button_n5_clicked();
    void on_button_n6_clicked();
    void on_button_n7_clicked();
    void on_button_n8_clicked();
    void on_button_n9_clicked();
    void on_button_negate_clicked();
    void on_button_subtract_clicked();

    void on_button_memory_add_clicked();
    void on_button_memory_clear_clicked();
    void on_button_memory_recall_clicked();
    void on_button_memory_subtract_clicked();
    void on_button_parentheses_left_clicked();
    void on_button_parentheses_right_clicked();

    // Buttons in Scientific View (some of which are in Basic View)

    void on_angleUnitSelection_activated(int i);

    void on_button_absolute_value_clicked();
    void on_button_constant_e_clicked();
    void on_button_constant_pi_clicked();
    void on_button_cosine_clicked();
    void on_button_exponent_scientific_clicked();
    void on_button_exponential_clicked();
    void on_button_exponential_base10_clicked();
    void on_button_exponential_natural_clicked();
    void on_button_factorial_clicked();
    void on_button_hyp_cosine_clicked();
    void on_button_hyp_sine_clicked();
    void on_button_hyp_tangent_clicked();
    void on_button_inverse_cosine_clicked();
    void on_button_inverse_hyp_cosine_clicked();
    void on_button_inverse_hyp_sine_clicked();
    void on_button_inverse_hyp_tangent_clicked();
    void on_button_inverse_sine_clicked();
    void on_button_inverse_tangent_clicked();
    void on_button_logarithm_common_clicked();
    void on_button_logarithm_natural_clicked();
    void on_button_modulus_clicked();
    void on_button_percent_clicked();
    void on_button_random_number_clicked();
    void on_button_reciprocal_clicked();
    void on_button_sine_clicked();
    void on_button_square_clicked();
    void on_button_square_root_clicked();
    void on_button_tangent_clicked();
    void on_button_x_th_root_clicked();

    // Buttons in Programmer View

    void on_button_ascii_clicked();
    void on_button_base_bin_clicked();
    void on_button_base_dec_clicked();
    void on_button_base_hex_clicked();
    void on_button_base_oct_clicked();
    void on_button_bit_rotate_left_clicked();
    void on_button_bit_rotate_right_clicked();
    void on_button_bit_shift_left_clicked();
    void on_button_bit_shift_right_clicked();
    void on_button_bitwise_and_clicked();
    void on_button_bitwise_not_clicked();
    void on_button_bitwise_or_clicked();
    void on_button_bitwise_xor_clicked();
    void on_button_conv_clicked();
    void on_button_modulus_2_clicked();
    void on_button_unicode_clicked();
    void on_button_i_a_clicked();
    void on_button_i_b_clicked();
    void on_button_i_c_clicked();
    void on_button_i_d_clicked();
    void on_button_i_e_clicked();
    void on_button_i_f_clicked();

    void on_wordlen_toggled();

    // Slots for Other functions

    void updateDisplay();
    void appendDigit(const std::string& digit);
    void appendOperator(const std::string& op);
    void load_entry_from_big(const mpf_class& x);

private:
    Ui::MainWindow* ui;
};
#endif // MAINWINDOW_H

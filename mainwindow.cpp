#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug> // for outputting debug messages

#include "converter.h" // Include header for Converter form
#include "settings.h" // Include header for Settings form
#include "help.h" // Include header for Help form
#include "about.h" // Include header for About form

#include <gmp.h> // to handle the Arithmetic
#include <gmpxx.h>   // <-- add (C++ API; keep <gmp.h> or remove if unused)

#include <string> // Some other helpful dependencies
#include <vector>

// Initialize important variables globally
std::vector<std::string> equation_buffer;
std::vector<std::string> equation_display;
std::vector<std::string> numeric_input_buffer = { "0" };
std::string equation = "";
bool dp_used = false;
bool number_is_negative = false;
bool new_number = true;
bool minus_zero = false;
int open_parens = 0;

// Define important functions
std::string concat_numeric_input_buffer_content() {
    std::string output = "";
    output += number_is_negative ? "-" : "";
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
    qDebug() << QString::fromStdString(concat_numeric_input_buffer_content());  // <-- convert
    qDebug() << "Equation Buffer: ";
    qDebug() << QString::fromStdString(concat_equation_buffer_content());       // <-- convert
    qDebug() << "\n";
}

// ===== Postfix (RPN) evaluator with GMP =====
typedef mpf_class BigFloat;

static bool g_eval_div0 = false;

// Tokenize into numbers, operators, parentheses, with unary minus as "u-"
static std::vector<std::string> tokenizeInfix(const std::string& s) {
    std::vector<std::string> out;
    auto isop = [](char c) { return c == '+' || c == '-' || c == '*' || c == '/'; };
    const size_t n = s.size();
    for (size_t i = 0; i < n; ++i) {
        char c = s[i];
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            std::string num;
            while (i < n && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) num += s[i++];
            --i; out.push_back(num);
        }
        else if (c == '(' || c == ')') {
            out.emplace_back(1, c);
        }
        else if (isop(c)) {
            bool unary = false;
            if (c == '-') {
                if (out.empty()) unary = true;
                else {
                    const std::string& prev = out.back();
                    if (prev == "(" || prev == "+" || prev == "-" || prev == "*" || prev == "/") unary = true;
                }
            }
            if (unary) out.push_back("u-"); else out.emplace_back(1, c);
        }
    }
    return out;
}

static int prec(const std::string& op) {
    if (op == "u-") return 3; // unary minus highest
    if (op == "*" || op == "/") return 2;
    if (op == "+" || op == "-") return 1;
    return 0;
}
static bool isOp(const std::string& t) { return t == "+" || t == "-" || t == "*" || t == "/" || t == "u-"; }

static std::vector<std::string> infixToPostfix(const std::vector<std::string>& toks) {
    std::vector<std::string> out; out.reserve(toks.size());
    std::vector<std::string> st;
    for (const auto& t : toks) {
        if (isOp(t)) {
            while (!st.empty() && isOp(st.back()) && ((prec(st.back()) > prec(t)) || (prec(st.back()) == prec(t) && t != "u-"))) {
                out.push_back(st.back()); st.pop_back();
            }
            st.push_back(t);
        }
        else if (t == "(") {
            st.push_back(t);
        }
        else if (t == ")") {
            while (!st.empty() && st.back() != "(") { out.push_back(st.back()); st.pop_back(); }
            if (!st.empty() && st.back() == "(") st.pop_back();
        }
        else {
            out.push_back(t);
        }
    }
    while (!st.empty()) { if (st.back() != "(") out.push_back(st.back()); st.pop_back(); }
    return out;
}

static BigFloat toBig(const std::string& s) { BigFloat v(s); return v; }
static BigFloat evalPostfix(const std::vector<std::string>& rpn) {
    std::vector<BigFloat> st;
    for (const auto& t : rpn) {
        if (!isOp(t)) { st.push_back(toBig(t)); continue; }
        if (t == "u-") {
            if (st.empty()) return BigFloat(0);
            BigFloat a = st.back(); st.pop_back(); st.push_back(-a); continue;
        }
        if (st.size() < 2) return BigFloat(0);
        BigFloat b = st.back(); st.pop_back();
        BigFloat a = st.back(); st.pop_back();
        if (t == "+") st.push_back(a + b);
        else if (t == "-") st.push_back(a - b);
        else if (t == "*") st.push_back(a * b);
        else if (t == "/") {
            if (b == 0) { g_eval_div0 = true; st.push_back(BigFloat(0)); } // do not attempt inf/NaN
            else st.push_back(a / b);
        }
    }
    return st.empty() ? BigFloat(0) : st.back();
}

static BigFloat evaluateFromInfix(const std::string& expr) {
    g_eval_div0 = false;
    auto toks = tokenizeInfix(expr);
    auto rpn = infixToPostfix(toks);
    return evalPostfix(rpn);
}

// GMP string formatter (no iostream precision quirks)
static std::string mpf_to_string(const mpf_class& x, size_t digits = 34) {
    mp_exp_t exp = 0;                          // digits before decimal
    std::string mant = x.get_str(exp, 10, digits);

    bool neg = (!mant.empty() && mant[0] == '-');
    if (neg) mant.erase(mant.begin());
    if (mant.empty()) return "0";

    std::string s;
    if (exp <= 0) {
        if (neg) s.push_back('-');
        s += "0.";
        s.append(static_cast<size_t>(-exp), '0');
        s += mant;
    }
    else if (static_cast<size_t>(exp) >= mant.size()) {
        if (neg) s.push_back('-');
        s += mant;
        s.append(static_cast<size_t>(exp) - mant.size(), '0');
    }
    else {
        if (neg) s.push_back('-');
        s.append(mant, 0, static_cast<size_t>(exp));
        s.push_back('.');
        s.append(mant, static_cast<size_t>(exp), std::string::npos);
    }
    if (auto p = s.find('.'); p != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s.empty() ? "0" : s;
}

static std::string stripTrailingOperator(const std::string& s) {
    if (s.empty()) return s;
    size_t i = s.size();
    while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1]))) --i;
    if (i == 0) return "";
    char c = s[i - 1];
    if (c == '+' || c == '-' || c == '*' || c == '/') return s.substr(0, i - 1);
    return s;
}
// ===== end RPN evaluator =====

// ===== Pretty-printing the equation (display only) =====

// Merge consecutive digit/decimal tokens into a single number token.
static std::vector<std::string> coalesceNumbers(const std::vector<std::string>& raw) {
    std::vector<std::string> out;
    std::string acc;
    auto flush_acc = [&]() { if (!acc.empty()) { out.push_back(acc); acc.clear(); } };

    for (const auto& t : raw) {
        bool isDigit = (t.size() == 1) && (std::isdigit(static_cast<unsigned char>(t[0])) || t == ".");
        if (isDigit) {
            acc += t;
        }
        else {
            flush_acc();
            out.push_back(t);
        }
    }
    flush_acc();
    return out;
}

static inline bool isBinaryOpTok(const std::string& t) {
    return t == "+" || t == "-" || t == "*" || t == "/";
}

static bool isUnaryMinusAt(const std::vector<std::string>& toks, size_t i) {
    if (toks[i] != "-") return false;
    if (i == 0) return true;
    const std::string& prev = toks[i - 1];
    // If previous is an opening paren or an operator, this '-' is unary
    return (prev == "(" || isBinaryOpTok(prev));
}

static QString pretty_equation_from_tokens(const std::vector<std::string>& raw) {
    const auto toks = coalesceNumbers(raw);
    QString out;

    auto lastChar = [&]() -> QChar {
        if (out.isEmpty()) return QChar();
        return out.at(out.size() - 1);
        };

    for (size_t i = 0; i < toks.size(); ++i) {
        const std::string& t = toks[i];

        // Parentheses: no extra spaces next to them
        if (t == "(") {
            // If previous token was a value or ')', you *might* want a space (optional)
            // We keep it tight for a classic calculator look.
            out += "(";
            continue;
        }
        if (t == ")") {
            out += ")";
            continue;
        }

        // Operators
        if (isBinaryOpTok(t)) {
            // Unary minus stays tight (e.g., 1 * -(2) or (-3))
            if (t == "-" && isUnaryMinusAt(toks, i)) {
                out += "-";
                continue;
            }

            // Binary operator: add spaces around
            QString sym;
            if (t == "*")      sym = QStringLiteral("×");
            else if (t == "/") sym = QStringLiteral("÷");
            else               sym = QString::fromStdString(t);

            // no space just after '('
            if (lastChar() != '(' && lastChar() != QChar(' ') && !out.isEmpty())
                out += " ";
            out += sym;
            // no space just before ')'
            if (i + 1 < toks.size() && toks[i + 1] != ")")
                out += " ";
            continue;
        }

        // Number or any other literal token
        out += QString::fromStdString(t);
    }

    return out;
}

void MainWindow::updateDisplay() {
    std::string eq = concat_equation_buffer_content();
    std::string current = concat_numeric_input_buffer_content();

    ui->answerInputLabel->setText(QString::fromStdString(current));
    // OLD:
    // ui->equationLabel->setText(QString::fromStdString(eq));

    // NEW (beautified):
    ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
}

MainWindow::MainWindow(QWidget* parent)
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

    QObject::connect(ui->absolute_value_2, &QPushButton::clicked, this, &MainWindow::on_button_absolute_value_clicked);
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
    numeric_input_buffer = { "0" };
    equation_buffer.clear();
    dp_used = false;
    number_is_negative = false;
    new_number = true;
    open_parens = 0;
    equation.clear();
    updateDisplay();
}

void MainWindow::on_button_backspace_clicked() {
    if (numeric_input_buffer.size() > 1) {
        if (numeric_input_buffer.back() == ".") dp_used = false;
        numeric_input_buffer.pop_back();
    }
    else {
        numeric_input_buffer = { "0" };
        new_number = true;
    }
    updateDisplay();
}

void MainWindow::appendDigit(const std::string& digit) {
    if (new_number) {
        numeric_input_buffer.clear();
        new_number = false;
    }

    // Avoid leading zeros unless there's a decimal point
    if (numeric_input_buffer.size() == 1 && numeric_input_buffer[0] == "0" && digit != ".") {
        numeric_input_buffer.clear();
    }

    numeric_input_buffer.push_back(digit);
    updateDisplay();
}
void MainWindow::on_button_n0_clicked() { appendDigit("0"); }
void MainWindow::on_button_n1_clicked() { appendDigit("1"); }
void MainWindow::on_button_n2_clicked() { appendDigit("2"); }
void MainWindow::on_button_n3_clicked() { appendDigit("3"); }
void MainWindow::on_button_n4_clicked() { appendDigit("4"); }
void MainWindow::on_button_n5_clicked() { appendDigit("5"); }
void MainWindow::on_button_n6_clicked() { appendDigit("6"); }
void MainWindow::on_button_n7_clicked() { appendDigit("7"); }
void MainWindow::on_button_n8_clicked() { appendDigit("8"); }
void MainWindow::on_button_n9_clicked() { appendDigit("9"); }

static inline bool isOpToken(const std::string& t) {
    return t == "+" || t == "-" || t == "*" || t == "/";
}

void MainWindow::appendOperator(const std::string& op) {
    auto isOpToken = [](const std::string& t) { return t == "+" || t == "-" || t == "*" || t == "/"; };


    // If buffer is empty and user presses an operator after AC, treat as 0 <op>
    if (equation_buffer.empty() && new_number) {
        equation_buffer.push_back("0");
    }


    // Commit current number if present
    if (!new_number) {
        if (number_is_negative) equation_buffer.push_back("(");
        if (number_is_negative) equation_buffer.push_back("-");
        for (const std::string& c : numeric_input_buffer) equation_buffer.push_back(c);
        if (number_is_negative) equation_buffer.push_back(")");
    }


    // Collapse any trailing operators; keep only the last one pressed
    while (!equation_buffer.empty() && isOpToken(equation_buffer.back())) {
        equation_buffer.pop_back();
    }


    // Add operator
    equation_buffer.push_back(op);


    // Reset for next input
    dp_used = false;
    new_number = true;
    number_is_negative = false;


    updateDisplay();
}
void MainWindow::on_button_add_clicked() { appendOperator("+"); }
void MainWindow::on_button_subtract_clicked() { appendOperator("-"); }
void MainWindow::on_button_multiply_clicked() { appendOperator("*"); }
void MainWindow::on_button_divide_clicked() { appendOperator("/"); }

void MainWindow::on_button_decimal_point_clicked() {
    if (!dp_used) {
        if (new_number) {
            numeric_input_buffer.clear();
            numeric_input_buffer.push_back("0");
            new_number = false;
        }
        numeric_input_buffer.push_back(".");
        dp_used = true;
        updateDisplay();
    }
}

void MainWindow::on_button_negate_clicked() {
    number_is_negative = !number_is_negative;
    updateDisplay();
}

void MainWindow::on_button_ans_clicked() {
}

void MainWindow::on_button_equals_clicked() {
    // --- 1) Build token list for both display and evaluation ---
    std::vector<std::string> eval_tokens = equation_buffer;

    // Commit current input (wrap negatives the same way you do elsewhere)
    if (!new_number) {
        if (number_is_negative) { eval_tokens.push_back("("); eval_tokens.push_back("-"); }
        for (const auto& c : numeric_input_buffer) eval_tokens.push_back(c);
        if (number_is_negative) eval_tokens.push_back(")");
    }

    // Auto-close open parens
    for (int i = 0; i < open_parens; ++i) eval_tokens.push_back(")");

    // Strip any trailing operator (token-based)
    auto isOpToken = [](const std::string& t) { return t == "+" || t == "-" || t == "*" || t == "/"; };
    if (!eval_tokens.empty() && isOpToken(eval_tokens.back())) {
        eval_tokens.pop_back();
    }

    // If nothing left, just use the current input as the whole expression
    if (eval_tokens.empty()) {
        // reflect current input only
        eval_tokens.clear();
        if (number_is_negative) { eval_tokens.push_back("("); eval_tokens.push_back("-"); }
        for (const auto& c : numeric_input_buffer) eval_tokens.push_back(c);
        if (number_is_negative) eval_tokens.push_back(")");
    }

    // --- 2) Show the BEAUTIFIED equation BEFORE evaluation ---
    // Use your pretty-printer on the token vector
    ui->equationLabel->setText(pretty_equation_from_tokens(eval_tokens) + " =");

    // --- 3) Concatenate ASCII tokens for the evaluator ---
    std::string final_eq;
    final_eq.reserve(eval_tokens.size() * 2);
    for (const auto& t : eval_tokens) final_eq += t;

    // --- 4) Evaluate (RPN + GMP) ---
    BigFloat res = evaluateFromInfix(final_eq);

    std::string result;
    if (g_eval_div0) {
        result = "Error: Divide by 0";
    }
    else {
        result = mpf_to_string(res, 34); // your GMP-safe formatter
    }

    ui->answerInputLabel->setText(QString::fromStdString(result));

    // --- 5) Prepare next state ---
    equation_buffer.clear();
    numeric_input_buffer = { g_eval_div0 ? std::string("0") : result };
    new_number = true; number_is_negative = false; dp_used = false; open_parens = 0;
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
    auto commit_current_number = [&]() {
        if (!new_number) {
            if (number_is_negative) equation_buffer.push_back("(");
            if (number_is_negative) equation_buffer.push_back("-");
            for (const std::string& c : numeric_input_buffer) equation_buffer.push_back(c);
            if (number_is_negative) equation_buffer.push_back(")");
        }
        };

    auto push_open = [&]() {
        equation_buffer.push_back("(");
        ++open_parens;
        new_number = true; dp_used = false; number_is_negative = false;
        };

    auto lastChar = [&]() -> char {
        if (equation_buffer.empty()) return 0;
        const std::string& s = equation_buffer.back();
        return s.empty() ? 0 : s.back();
        };

    // If a number is currently being typed, do implicit multiplication: number * (
    if (!new_number) {
        commit_current_number();
        equation_buffer.push_back("*");   // <- split tokens
        push_open();                      // pushes "(" and updates state
        updateDisplay();
        return;
    }

    char lc = lastChar();
    bool afterOpOrStart = (lc == 0 || lc == '(' || lc == '+' || lc == '-' || lc == '*' || lc == '/');

    if (afterOpOrStart) {
        push_open();
    }
    else {
        // Previous token was a value or ')': implicit multiplication
        equation_buffer.push_back("*");
        push_open();
    }
    updateDisplay();
}

void MainWindow::on_button_parentheses_right_clicked() {
    if (open_parens > 0) {
        // Add current number before closing
        if (!new_number) {
            if (number_is_negative) equation_buffer.push_back("(");
            if (number_is_negative) equation_buffer.push_back("-");
            for (std::string c : numeric_input_buffer)
                equation_buffer.push_back(c);
            if (number_is_negative) equation_buffer.push_back(")");
        }

        equation_buffer.push_back(")");
        open_parens--;
        dp_used = false;
        new_number = true;
        number_is_negative = false;
        updateDisplay();
    }
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

// why did i make this
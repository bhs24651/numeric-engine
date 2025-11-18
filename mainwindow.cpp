#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug> // for outputting debug messages

#include "converter.h" // Include header for Converter form
#include "settings.h" // Include header for Settings form
#include "help.h" // Include header for Help form
#include "about.h" // Include header for About form

#include <gmp.h> // to handle the Arithmetic
#include <gmpxx.h>   // <-- add (C++ API; keep <gmp.h> or remove if unused)
#include <cctype>
#include <cmath>
#include <sstream>    // for std::ostringstream
#include <iomanip>    // for std::fixed and std::setprecision
#include <random>

#include <string> // Some other helpful dependencies
#include <vector>

// Initialize important variables globally
typedef mpf_class BigFloat;

std::vector<std::string> equation_buffer;
std::vector<std::string> equation_display;
std::vector<std::string> numeric_input_buffer = { "0" };
std::string equation = "";
std::string last_eval_error;
bool dp_used = false;
bool number_is_negative = false;
bool new_number = true;
bool minus_zero = false;
int open_parens = 0;
bool just_evaluated = false;
bool just_evaluated_full = false;   // true only when '=' was pressed

BigFloat last_answer = 0;
BigFloat memory = 0;

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
static bool g_eval_div0 = false;

// Tokenize into numbers, operators, parentheses, with unary minus as "u-"
static std::vector<std::string> tokenizeInfix(const std::string& s) {
    std::vector<std::string> out;
    auto isop = [](char c) { return c == '+' || c == '-' || c == '*' || c == '/' || c == '^'; };
    const size_t n = s.size();

    for (size_t i = 0; i < n; ++i) {
        char c = s[i];
        if (std::isspace(static_cast<unsigned char>(c))) continue;

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            std::string num;
            while (i < n && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) num += s[i++];
            --i; out.push_back(num);
        }
        else if (std::isalpha(static_cast<unsigned char>(c))) {
            // read identifier
            std::string id;
            while (i < n && std::isalpha(static_cast<unsigned char>(s[i]))) {
                id.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(s[i]))));
                ++i;
            }
            --i;
            if (id == "mod") out.push_back("mod");
            // (ignore other identifiers here; function calls get expanded earlier)
        }
        else if (c == '(' || c == ')') {
            out.emplace_back(1, c);
        }
        else if (isop(c)) {
            // unary minus detection (allow after any operator or '(')
            bool unary = false;
            if (c == '-') {
                if (out.empty()) unary = true;
                else {
                    const std::string& prev = out.back();
                    if (prev == "(" || prev == "+" || prev == "-" || prev == "*" || prev == "/" || prev == "^" || prev == "mod")
                        unary = true;
                }
            }
            if (unary) out.push_back("u-");
            else out.emplace_back(1, c);
        }
    }
    return out;
}

static int prec(const std::string& op) {
    if (op == "u-") return 4;
    if (op == "^")  return 3;
    if (op == "*" || op == "/" || op == "mod") return 2;   // ← added
    if (op == "+" || op == "-") return 1;
    return 0;
}

static bool isOp(const std::string& t) {
    return t == "+" || t == "-" || t == "*" || t == "/" || t == "^" || t == "u-" || t == "mod"; // ← added
}

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

static BigFloat pow_int(BigFloat base, long long exp) {
    if (exp == 0) return BigFloat(1);
    bool neg = (exp < 0);
    unsigned long long n = neg ? (unsigned long long)(-exp) : (unsigned long long)exp;
    BigFloat res = 1;
    while (n) {
        if (n & 1ULL) res *= base;
        if (n > 1)    base *= base;
        n >>= 1ULL;
    }
    return neg ? (BigFloat(1) / res) : res;
}

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
        else if (t == "^") {
            // Use full-precision integer exponent when possible
            double bd = b.get_d();
            long long bi = static_cast<long long>(bd);
            if (std::fabs(bd - static_cast<double>(bi)) < 1e-12) {
                st.push_back(pow_int(a, bi));
            }
            else {
                // fallback for non-integer exponents
                st.push_back(BigFloat(::pow(a.get_d(), b.get_d())));
            }
        }
        else if (t == "mod") {
            if (b == 0) {              // x mod 0 → “undefined” like division by zero
                g_eval_div0 = true;
                st.push_back(BigFloat(0));
            }
            else {
                // C/C++ fmod semantics: sign follows the dividend (a)
                double r = std::fmod(a.get_d(), b.get_d());
                st.push_back(BigFloat(r));
            }
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

static std::string format_fixed(const mpf_class& x, int max_decimals) {
    // get a long mantissa then round/cut to max_decimals in fixed form
    mp_exp_t exp = 0;
    std::string mant = x.get_str(exp, 10, 80); // 80+ digits headroom
    bool neg = (!mant.empty() && mant[0] == '-');
    if (neg) mant.erase(mant.begin());
    if (mant.empty()) return "0";

    std::string s;
    if (exp <= 0) {
        s += neg ? "-0." : "0.";
        s.append(static_cast<size_t>(-exp), '0');
        s += mant;
    }
    else if (static_cast<size_t>(exp) >= mant.size()) {
        s += neg ? "-" : "";
        s += mant;
        s.append(static_cast<size_t>(exp) - mant.size(), '0');
    }
    else {
        s += neg ? "-" : "";
        s.append(mant, 0, static_cast<size_t>(exp));
        s.push_back('.');
        s.append(mant, static_cast<size_t>(exp), std::string::npos);
    }

    // round/cut to max_decimals
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        size_t want = dot + 1 + static_cast<size_t>(max_decimals);
        if (s.size() > want) {
            // simple cut (optional: implement half-up rounding)
            s.resize(want);
        }
        // trim trailing zeros
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s.empty() ? "0" : s;
}

// Round a digit-only mantissa to `keep` significant digits (half-up), with carry propagation.
static void round_digit_mantissa(std::string& d, int keep) {
    if ((int)d.size() <= keep) return;
    int carry = (d[keep] >= '5') ? 1 : 0;
    d.resize(keep);
    for (int i = keep - 1; i >= 0 && carry; --i) {
        int v = (d[i] - '0') + carry;
        d[i] = char('0' + (v % 10));
        carry = v / 10;
    }
    if (carry) d.insert(d.begin(), '1'); // e.g., 9..9 -> 10..0, bumps exponent
}

static std::string format_scientific_e(const mpf_class& x, int sig_digits) {
    if (x == 0) return "0";

    mp_exp_t exp10 = 0;
    std::string mant = x.get_str(exp10, 10, sig_digits + 2); // a little headroom
    bool neg = (!mant.empty() && mant[0] == '-');
    if (neg) mant.erase(mant.begin());

    // mant like "12345..." with exp10 meaning 1.2345... × 10^(exp10-1)
    std::string m = mant;
    if (m.size() > 1) {
        m.insert(m.begin() + 1, '.');  // 1.xxx
    }

    // trim decimals to sig_digits total significant figures
    if (auto p = m.find('.'); p != std::string::npos) {
        // keep: 1 digit, '.', and (sig_digits - 1) decimal digits
        size_t keep = 1 + 1 + (sig_digits - 1);
        if (m.size() > keep) m.resize(keep);

        // strip trailing zeros and possible trailing '.'
        while (!m.empty() && m.back() == '0') m.pop_back();
        if (!m.empty() && m.back() == '.') m.pop_back();
    }

    long e = static_cast<long>(exp10) - 1;

    std::string out;
    if (neg) out.push_back('-');
    out += m;
    out += 'e';
    if (e >= 0) out.push_back('+');
    out += std::to_string(e);

    return out;
}

static std::string format_scientific(const mpf_class& x, int sig_digits) {
    if (x == 0) return "0";
    mp_exp_t exp10 = 0;
    std::string mant = x.get_str(exp10, 10, sig_digits + 2); // a little headroom
    bool neg = (!mant.empty() && mant[0] == '-');
    if (neg) mant.erase(mant.begin());

    // mant like "12345..." with exp10 meaning 1.2345... × 10^(exp10-1)
    std::string m = mant;
    if (m.size() > 1) m.insert(m.begin() + 1, '.');

    // trim decimals
    if (auto p = m.find('.'); p != std::string::npos) {
        // keep (sig_digits) significant digits total
        size_t keep = 1 + 1 + (sig_digits - 1); // "d" "." + (sig-1)
        if (m.size() > keep) m.resize(keep);
        while (!m.empty() && m.back() == '0') m.pop_back();
        if (!m.empty() && m.back() == '.') m.pop_back();
    }

    long e = static_cast<long>(exp10) - 1;
    return std::string(neg ? "-" : "") + m + " × 10^" + std::to_string(e);
}

static std::string shrink_for_display(const std::string& s, std::size_t max_chars = 24)
{
    if (s.size() <= max_chars) return s;

    // Look for scientific 'e' notation
    std::size_t epos = s.find('e');
    if (epos == std::string::npos) {
        // Not scientific: just hard truncate from the right
        return s.substr(0, max_chars);
    }

    // Split into mantissa (with sign) and exponent
    std::string mant = s.substr(0, epos);   // e.g. "-1.23456789"
    std::string exp = s.substr(epos);      // e.g. "e+12345"

    // While the whole thing is too long, try to drop fractional digits
    while (mant.size() + exp.size() > max_chars) {
        // Find decimal point in mantissa
        std::size_t dot = mant.find('.');
        if (dot == std::string::npos) {
            // No decimal point → nothing fractional to drop
            break;
        }
        if (mant.size() <= dot + 1) {
            // Nothing after "." to drop
            break;
        }

        // Drop one character from the end (a fractional digit)
        mant.pop_back();

        // Clean up trailing zeros and possibly the '.' itself
        while (!mant.empty() && mant.back() == '0') mant.pop_back();
        if (!mant.empty() && mant.back() == '.') mant.pop_back();
    }

    std::string out = mant + exp;

    // As a last resort (huge exponents), just hard truncate
    if (out.size() > max_chars)
        out.resize(max_chars);

    return out;
}

static std::string format_for_display(const mpf_class& x,
    int max_decimals = 21,
    int sci_sig = 21,
    int sci_pos_thresh = 20,   // |x| >= 1e20
    int sci_neg_thresh = -5)   // |x| <= 1e-5
{
    if (x == 0) return "0";
    mp_exp_t e = 0;
    (void)x.get_str(e, 10, 2);
    long exp10 = static_cast<long>(e) - 1;

    std::string s;
    if (exp10 >= sci_pos_thresh || exp10 <= sci_neg_thresh) {
        // e-notation
        s = format_scientific_e(x, sci_sig);
    }
    else {
        // fixed notation
        s = format_fixed(x, max_decimals);
    }

    // Enforce max display width (24 chars)
    return shrink_for_display(s, 24);
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

// --- Display-only function detection ---
static bool isDisplayFunc(const std::string& t) {
    return
        t == "FUNC_ABS" ||
        t == "FUNC_PI" ||
        t == "FUNC_E" ||
        t == "FUNC_SIN" ||
        t == "FUNC_COS" ||
        t == "FUNC_TAN" ||
        t == "FUNC_ASIN" ||
        t == "FUNC_ACOS" ||
        t == "FUNC_ATAN" ||
        t == "FUNC_SINH" ||
        t == "FUNC_COSH" ||
        t == "FUNC_TANH" ||
        t == "FUNC_ASINH" ||
        t == "FUNC_ACOSH" ||
        t == "FUNC_ATANH" ||
        t == "FUNC_LN" ||
        t == "FUNC_LOG10" ||
        t == "FUNC_SQRT" ||
        t == "FUNC_SQR" ||
        t == "FUNC_RECIP" ||
        t == "FUNC_EXP" ||
        t == "FUNC_EXP10" ||
        t == "FUNC_FACT" ||
        t == "FUNC_MOD" ||
        t == "FUNC_PERCENT" ||
        t == "FUNC_XROOT";
}

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

// --- add these helpers just ABOVE pretty_equation_from_tokens() ---

static bool isNumberToken(const std::string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') {
        if (s.size() == 1) return false;
        i = 1;
    }
    bool dp = false;
    for (; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (std::isdigit(c)) continue;
        if (s[i] == '.' && !dp) { dp = true; continue; }
        return false;
    }
    return true;
}

static bool isValueLikeToken(const std::string& t) {
    if (t == ")" || t == "ANS" || t == "FUNC_PI" || t == "FUNC_E")
        return true;
    // Treat a plain numeric token as a value too
    return isNumberToken(t);
}

// Detect the exact token sequence: "(" "-" <number> ")"
static bool isNegativeLiteral(const std::vector<std::string>& toks, size_t i) {
    return (i + 3 < toks.size()
        && toks[i] == "("
        && toks[i + 1] == "-"
        && isNumberToken(toks[i + 2])
        && toks[i + 3] == ")");
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

        // collapse "( - n )" but wrap if it's the base of a power: (-n)^m
        if (isNegativeLiteral(toks, i)) {
            // tokens are: i:"(", i+1:"-", i+2:<num>, i+3:")"
            bool powAfter = (i + 4 < toks.size() && toks[i + 4] == "^");
            QString neg = "-" + QString::fromStdString(toks[i + 2]);
            if (powAfter) {
                out += "(" + neg + ")";    // show (-n) when immediately followed by ^
            }
            else {
                out += neg;                // otherwise just -n
            }
            i += 3; // skip "( - n )"
            continue;
        }

        // If this token is a number (including negative like "-3"), and the NEXT token is '^',
        // we should wrap the base in parentheses to show (-x)^y
        /*if (isNumberToken(t) && (i + 1 < toks.size()) && toks[i + 1] == "^") {
            // wrap negative or positive numbers only if negative or if user prefers clarity:
            if (!t.empty() && t[0] == '-') {
                out += "(" + QString::fromStdString(t) + ")";
            }
            else {
                out += QString::fromStdString(t);
            }
            continue;
        }*/
        // Wrap negative numbers in parentheses when used as base for power (^)
        if ((i + 1 < toks.size()) && toks[i + 1] == "^") {
            // Case 1: plain negative number token "-3"
            if (isNumberToken(t) && !t.empty() && t[0] == '-') {
                out += "(" + QString::fromStdString(t) + ")";
                continue;
            }

            // Case 2: already parenthesized form "( - 3 )"
            if (isNegativeLiteral(toks, i)) {
                QString neg = "-" + QString::fromStdString(toks[i + 2]);
                out += "(" + neg + ")";
                i += 3; // skip "( - n )"
                continue;
            }
        }

        // --- display-only funcs ---
        if (t == "FUNC_ABS") { out += "abs";   continue; }
        if (t == "ANS") { out += "Ans"; continue; }
        if (t == "FUNC_PI") { out += "π";     continue; }
        if (t == "FUNC_E") { out += "e";     continue; }
        if (t == "FUNC_SIN") { out += "sin";   continue; }
        if (t == "FUNC_COS") { out += "cos";   continue; }
        if (t == "FUNC_TAN") { out += "tan";   continue; }
        if (t == "FUNC_ASIN") { out += "sin⁻¹";  continue; }
        if (t == "FUNC_ACOS") { out += "cos⁻¹";  continue; }
        if (t == "FUNC_ATAN") { out += "tan⁻¹";  continue; }
        if (t == "FUNC_SINH") { out += "sinh";  continue; }
        if (t == "FUNC_COSH") { out += "cosh";  continue; }
        if (t == "FUNC_TANH") { out += "tanh";  continue; }
        if (t == "FUNC_ASINH") { out += "sinh⁻¹"; continue; }
        if (t == "FUNC_ACOSH") { out += "cosh⁻¹"; continue; }
        if (t == "FUNC_ATANH") { out += "tanh⁻¹"; continue; }
        if (t == "FUNC_LN") { out += "ln";    continue; }
        if (t == "FUNC_LOG10") { out += "log";   continue; }
        if (t == "FUNC_SQRT") { out += QStringLiteral("√"); continue; }
        if (t == "FUNC_SQR") { out += "sqr";   continue; }
        if (t == "FUNC_RECIP") { out += "1/";    continue; }
        if (t == "FUNC_EXP") { out += "exp";   continue; }
        if (t == "FUNC_EXP10") { out += "10^";   continue; }
        if (t == "FUNC_FACT") { out += "fact";     continue; }
        if (t == "FUNC_MOD") { out += "mod";   continue; }
        if (t == "FUNC_PERCENT") { out += "% of"; continue; }
        // if (t == "FUNC_XROOT") { out += QStringLiteral("√x"); continue; } // we'll refine in a sec
        if (t == "FUNC_XROOT") {
            // FUNC_XROOT(x, y) → √[x](y)
            if (i + 1 < toks.size() && toks[i + 1] == "(") {
                int depth = 0;
                size_t j = i + 1;
                for (; j < toks.size(); ++j) {
                    if (toks[j] == "(") ++depth;
                    else if (toks[j] == ")") {
                        --depth;
                        if (depth == 0) { ++j; break; }
                    }
                }

                // Find comma separating x and y
                size_t commaPos = i + 2;
                int innerDepth = 0;
                for (; commaPos < j; ++commaPos) {
                    if (toks[commaPos] == "(") innerDepth++;
                    else if (toks[commaPos] == ")") innerDepth--;
                    else if (toks[commaPos] == "," && innerDepth == 0)
                        break;
                }

                // Extract root index x
                QString x;
                for (size_t k = i + 2; k < commaPos; ++k)
                    x += QString::fromStdString(toks[k]);

                // Extract radicand y
                QString y;
                for (size_t k = commaPos + 1; k < j - 1; ++k)
                    y += QString::fromStdString(toks[k]);

                // Format √[x](y)
                out += QStringLiteral("√[") + x.trimmed() + QStringLiteral("](") + y.trimmed() + QStringLiteral(")");

                // Skip processed tokens
                i = j - 1;
                continue;
            }
        }

        // ... then your existing code that handles (, ), operators, numbers ...
        // (keep your × / ÷ logic exactly as you had it)
        // ...

        // Parentheses: no extra spaces next to them
        //if (t == "(") {
        //    // If previous token was a value or ')', you *might* want a space (optional)
        //    // We keep it tight for a classic calculator look.
        //    out += "(";
        //    continue;
        //}

        // collapse parenthesized negative literals here
        if (isNegativeLiteral(toks, i)) {
            QString neg = "-" + QString::fromStdString(toks[i + 2]);
            // If next token is "^", parentheses will be added in next step
            out += neg;
            i += 3;
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
            if (t == "+") sym = QStringLiteral("+");
            else if (t == "-") sym = QStringLiteral("−");
            else if (t == "*")      sym = QStringLiteral("×");
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

// Read the current entry (what the user is typing) as BigFloat
static BigFloat current_entry_to_big() {
    return BigFloat(concat_numeric_input_buffer_content());
}

// Load a BigFloat into the entry buffer (respecting sign/decimal flags)
void MainWindow::load_entry_from_big(const mpf_class& x) {
    std::string s = mpf_to_string(x, 34);
    numeric_input_buffer.clear();

    number_is_negative = false;
    if (!s.empty() && s[0] == '-') {
        number_is_negative = true;
        s.erase(s.begin());
    }
    dp_used = (s.find('.') != std::string::npos);
    new_number = false;

    for (char ch : s) {
        // s now has only digits and optionally one '.'
        numeric_input_buffer.push_back(std::string(1, ch));
    }
}

static BigFloat big_pi() {
    // return BigFloat(std::acos(-1.0));
    return BigFloat("3.14159265358979323846");
}

// angle unit helpers
// --- Global angle mode (mirrors the combo box) ---
static AngleUnit g_angle_unit = ANG_DEG;

// --- Conversions ---
static BigFloat rad_to_deg(const BigFloat& r) { return r * 180 / big_pi(); }
static BigFloat rad_to_grad(const BigFloat& r) { return r * 200 / big_pi(); }
static BigFloat deg_to_rad(const BigFloat& d) { return d * big_pi() / 180; }
static BigFloat grad_to_rad(const BigFloat& g) { return g * big_pi() / 200; }

static BigFloat to_radians(const BigFloat& v) {
    switch (g_angle_unit) {
    case ANG_RAD:  return v;
    case ANG_GRAD: return grad_to_rad(v);
    default:       return deg_to_rad(v);
    }
}
static BigFloat from_radians(const BigFloat& r) {
    switch (g_angle_unit) {
    case ANG_RAD:  return r;
    case ANG_GRAD: return rad_to_grad(r);
    default:       return rad_to_deg(r);
    }
}

// Replace your previous expandDisplayFuncs implementation with this:

static std::string joinTokensToInfix(const std::vector<std::string>& toks, size_t start, size_t end) {
    // join tokens[start..end-1] with no separators (same flattening your on_button_equals used)
    std::string s;
    for (size_t i = start; i < end; ++i) s += toks[i];
    return s;
}

static BigFloat evalGroupAsBig(const std::vector<std::string>& toks, size_t start, size_t end) {
    // Flatten the group to infix and evaluate via your existing evaluator
    std::string inf = joinTokensToInfix(toks, start, end);
    return evaluateFromInfix(inf);
}

// Main expand: turn FUNC_NAME ( ... ) into numeric tokens by evaluating the (...) expression
static std::vector<std::string> expandDisplayFuncs(const std::vector<std::string>& toks) {
    std::vector<std::string> out;
    size_t n = toks.size();
    for (size_t i = 0; i < n; ++i) {
        const std::string& t = toks[i];

        // Always expand ANS to its numeric digits for later evaluation.
        if (t == "ANS") {
            std::string s = mpf_to_string(last_answer, 34);
            for (char ch : s) out.push_back(std::string(1, ch));
            continue;
        }

        // Always expand FUNC_PI and FUNC_E to their numeric values
        if (t == "FUNC_PI") {
            std::string s = mpf_to_string(big_pi(), 34);
            for (char ch : s) out.push_back(std::string(1, ch));
            continue;
        }
        if (t == "FUNC_E") {
            BigFloat e(std::exp(1.0));
            std::string s = mpf_to_string(e, 34);
            for (char ch : s) out.push_back(std::string(1, ch));
            continue;
        }

        // --- special-case: FUNC_SQR ( x ) => replace with (x)*(x) to preserve algebraic behavior
        if (t == "FUNC_SQR") {
            // if next is '(' collect group
            if (i + 1 < n && toks[i + 1] == "(") {
                // find matching paren
                int depth = 0;
                size_t j = i + 1;
                for (; j < n; ++j) {
                    if (toks[j] == "(") ++depth;
                    else if (toks[j] == ")") {
                        --depth;
                        if (depth == 0) { ++j; break; }
                    }
                }
                // push group, *, group
                for (size_t k = i + 1; k < j; ++k) out.push_back(toks[k]);
                out.push_back("*");
                for (size_t k = i + 1; k < j; ++k) out.push_back(toks[k]);
                i = j - 1;
                continue;
            }
            // otherwise ignore
            continue;
        }

        // FUNC_RECIP -> 1 / (group)
        if (t == "FUNC_RECIP") {
            if (i + 1 < n && toks[i + 1] == "(") {
                int depth = 0;
                size_t j = i + 1;
                for (; j < n; ++j) {
                    if (toks[j] == "(") ++depth;
                    else if (toks[j] == ")") {
                        --depth;
                        if (depth == 0) { ++j; break; }
                    }
                }
                out.push_back("1");
                out.push_back("/");
                for (size_t k = i + 1; k < j; ++k) out.push_back(toks[k]);
                i = j - 1;
                continue;
            }
            continue;
        }

        // FUNC_PERCENT -> (group) / 100
        if (t == "FUNC_PERCENT") {
            if (i + 1 < n && toks[i + 1] == "(") {
                int depth = 0;
                size_t j = i + 1;
                for (; j < n; ++j) {
                    if (toks[j] == "(") ++depth;
                    else if (toks[j] == ")") {
                        --depth;
                        if (depth == 0) { ++j; break; }
                    }
                }
                for (size_t k = i + 1; k < j; ++k) out.push_back(toks[k]);
                out.push_back("/");
                out.push_back("100");
                i = j - 1;
                continue;
            }
            continue;
        }

        // For unary/functions that should be evaluated to a numeric value at "=":
        if (t.rfind("FUNC_", 0) == 0) {
            // We will handle functions that take one argument: FUNC_SIN, FUNC_COS, FUNC_TAN, FUNC_LN, FUNC_LOG10, FUNC_SQRT, FUNC_ABS, FUNC_EXP, FUNC_EXP10, FUNC_SINH...
            // if next token is "(" collect the group and evaluate
            if (i + 1 < n && toks[i + 1] == "(") {
                int depth = 0;
                size_t j = i + 1;
                for (; j < n; ++j) {
                    if (toks[j] == "(") ++depth;
                    else if (toks[j] == ")") {
                        --depth;
                        if (depth == 0) { ++j; break; }
                    }
                }
                // ... inside your function handler block, before calling evalGroupAsBig:
                auto expanded = expandDisplayFuncs(std::vector<std::string>(toks.begin() + i + 2, toks.begin() + j - 1));
                BigFloat inner = evalGroupAsBig(expanded, 0, expanded.size());
                // evaluate inner expression to BigFloat
                // BigFloat inner = evalGroupAsBig(toks, i + 2, j - 1); // skip the outer '(' and ')'
                BigFloat result = inner;

                // Apply the function
                // circular trig (inputs depend on unit)
                if (t == "FUNC_SIN") {
                    BigFloat vrad = to_radians(inner);
                    result = BigFloat(::sin(vrad.get_d()));
                }
                else if (t == "FUNC_COS") {
                    BigFloat vrad = to_radians(inner);
                    result = BigFloat(::cos(vrad.get_d()));
                }
                else if (t == "FUNC_TAN") {
                    BigFloat vrad = to_radians(inner);
                    result = BigFloat(::tan(vrad.get_d()));
                }
                /*else if (t == "FUNC_ASIN") {
                    result = BigFloat(::asin(inner.get_d()));
                }
                else if (t == "FUNC_ACOS") {
                    result = BigFloat(::acos(inner.get_d()));
                }*/
                // inverse circular trig (outputs shown in selected unit)
                else if (t == "FUNC_ASIN") {
                    double v = inner.get_d();
                    if (v < -1.0 || v > 1.0) {
                        last_eval_error = "Error: asin domain [-1,1]";
                        out.clear(); out.push_back("0"); continue;
                    }
                    BigFloat r = BigFloat(::asin(v));      // radians
                    result = from_radians(r);
                }
                else if (t == "FUNC_ACOS") {
                    double v = inner.get_d();
                    if (v < -1.0 || v > 1.0) {
                        last_eval_error = "Error: acos domain [-1,1]";
                        out.clear(); out.push_back("0"); continue;
                    }
                    BigFloat r = BigFloat(::acos(v));      // radians
                    result = from_radians(r);
                }
                else if (t == "FUNC_ATAN") {
                    BigFloat r = BigFloat(::atan(inner.get_d()));  // radians
                    result = from_radians(r);
                }
                else if (t == "FUNC_SINH") {
                    result = BigFloat(::sinh(inner.get_d()));
                }
                else if (t == "FUNC_COSH") {
                    result = BigFloat(::cosh(inner.get_d()));
                }
                else if (t == "FUNC_TANH") {
                    result = BigFloat(::tanh(inner.get_d()));
                }
                else if (t == "FUNC_ASINH") {
                    // principal value: ℝ → ℝ
                    BigFloat r = BigFloat(std::asinh(inner.get_d()));
                    result = r;
                }
                else if (t == "FUNC_ACOSH") {
                    // domain: x >= 1
                    double v = inner.get_d();
                    if (v < 1.0) {
                        last_eval_error = "Error: acosh domain [1, +inf)";
                        out.clear(); out.push_back("0"); continue;
                    }
                    BigFloat r = BigFloat(std::acosh(v));
                    result = r;
                }
                else if (t == "FUNC_ATANH") {
                    // domain: |x| < 1
                    double v = inner.get_d();
                    if (!(v > -1.0 && v < 1.0)) {
                        last_eval_error = "Error: atanh domain (-1, 1)";
                        out.clear(); out.push_back("0"); continue;
                    }
                    BigFloat r = BigFloat(std::atanh(v));
                    result = r;
                }
                else if (t == "FUNC_LN") {
                    double v = inner.get_d();
                    if (v <= 0.0) {
                        last_eval_error = "Error: ln domain (0,∞)";
                        out.clear();
                        out.push_back("0");
                        continue;
                    }
                    result = BigFloat(::log(v));
                }
                else if (t == "FUNC_LOG10") {
                    double v = inner.get_d();
                    if (v <= 0.0) {
                        last_eval_error = "Error: log domain (0,∞)";
                        out.clear();
                        out.push_back("0");
                        continue;
                    }
                    result = BigFloat(::log10(v));
                }
                else if (t == "FUNC_SQRT") {
                    if (inner < 0) {
                        // leave as is (you may want to handle error earlier); here push "0" to avoid crash
                        result = BigFloat(0);
                    }
                    else {
                        mpf_t tmp; mpf_init(tmp);
                        mpf_sqrt(tmp, inner.get_mpf_t());
                        result = BigFloat(tmp); mpf_clear(tmp);
                    }
                }
                else if (t == "FUNC_ABS") {
                    if (inner < 0) result = -inner; else result = inner;
                }
                else if (t == "FUNC_EXP10") {
                    result = BigFloat(::pow(10.0, inner.get_d()));
                }
                else if (t == "FUNC_EXP") {
                    // treat as e^x
                    result = BigFloat(::exp(inner.get_d()));
                }
                else if (t == "FUNC_FACT") {
                    long long nfact = static_cast<long long>(inner.get_d());
                    if (nfact < 0) {
                        result = BigFloat(0);
                    }
                    else {
                        BigFloat res = 1;
                        for (long long k = 2; k <= nfact; ++k) res *= BigFloat(static_cast<double>(k));
                        result = res;
                    }
                }
                else if (t == "FUNC_XROOT") {
                    // FUNC_XROOT(x, y) => y^(1/x)
                    if (i + 1 < n && toks[i + 1] == "(") {
                        int depth = 0;
                        size_t j = i + 1;
                        for (; j < n; ++j) {
                            if (toks[j] == "(") ++depth;
                            else if (toks[j] == ")") {
                                --depth;
                                if (depth == 0) { ++j; break; }
                            }
                        }

                        // Find comma separating x and y
                        size_t commaPos = i + 2;
                        int innerDepth = 0;
                        for (; commaPos < j; ++commaPos) {
                            if (toks[commaPos] == "(") innerDepth++;
                            else if (toks[commaPos] == ")") innerDepth--;
                            else if (toks[commaPos] == "," && innerDepth == 0)
                                break;
                        }

                        // Evaluate x and y parts
                        auto leftPart = std::vector<std::string>(toks.begin() + i + 2, toks.begin() + commaPos);
                        auto rightPart = std::vector<std::string>(toks.begin() + commaPos + 1, toks.begin() + j - 1);

                        BigFloat x = evalGroupAsBig(leftPart, 0, leftPart.size());
                        BigFloat y = evalGroupAsBig(rightPart, 0, rightPart.size());

                        if (x == 0) {
                            // Root 0 is undefined
                            out.push_back("0");
                        }
                        else {
                            BigFloat result = BigFloat(::pow(y.get_d(), 1.0 / x.get_d()));
                            std::string s = mpf_to_string(result, 34);
                            for (char ch : s)
                                out.push_back(std::string(1, ch));
                        }

                        i = j - 1;
                        continue;
                    }
                }
                else {
                    // unknown FUNC_*, fallback: just append inner tokens (no function application)
                    for (size_t k = i + 1; k < j; ++k) out.push_back(toks[k]);
                    i = j - 1;
                    continue;
                }

                // push numeric result as tokens (digits and '.' as separate tokens to match other code)
                std::string s = mpf_to_string(result, 34);
                for (char ch : s) {
                    out.push_back(std::string(1, ch));
                }

                i = j - 1;
                continue;
            }
            else {
                // unknown no-arg function: skip
                continue;
            }
        }

        // default: copy tokens through
        out.push_back(t);
    }

    return out;
}

static std::vector<std::string> expandDisplayFuncs(const std::vector<std::string>& toks);

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

    // Disable Memory Clear and Memory Recall buttons at startup
    ui->memory_clear->setEnabled(false);
    ui->memory_recall->setEnabled(false);
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

void MainWindow::appendDigit(const std::string& digit)
{
    if (new_number || just_evaluated)
    {
        numeric_input_buffer.clear();

        // Only clear the equation buffer if we just finished evaluating (=),
        // not if we just pressed a function like sin(), cos(), etc.
        if (just_evaluated_full)
        {
            equation_buffer.clear();
            just_evaluated_full = false;
        }

        just_evaluated = false;
        new_number = false;
    }

    //

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
    return t == "+" || t == "-" || t == "*" || t == "/" || t == "mod"; // ← add "mod"
}

void MainWindow::appendOperator(const std::string& op) {
    // Did we *just* finish a full evaluation with '='?
    bool was_full_eval = just_evaluated_full;

    // Reset the flag so subsequent ops behave normally
    if (just_evaluated_full) {
        just_evaluated_full = false;
    }

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

    if (just_evaluated) {
        // Symbolically use last answer as ANS
        equation_buffer.clear();
        equation_buffer.push_back("ANS");
        just_evaluated = false;
    }

    // Add operator
    equation_buffer.push_back(op);

    // Reset for next input
    dp_used = false;
    new_number = true;
    number_is_negative = false;

    if (was_full_eval) {
        // We've already shown the nicely formatted result in answerInputLabel.
        // Only refresh the equation line (e.g. "Ans +").
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        // Normal behavior: update both equation and current-entry display.
        updateDisplay();
    }
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
    // If we just did a full evaluation with '=', start from that result symbolically
    if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    // Commit any currently typed number first
    if (!new_number) {
        if (number_is_negative) equation_buffer.push_back("(");
        if (number_is_negative) equation_buffer.push_back("-");
        for (const auto& c : numeric_input_buffer)
            equation_buffer.push_back(c);
        if (number_is_negative) equation_buffer.push_back(")");
        new_number = true;
    }

    // If previous token is a value (number, ANS, π, e, or ")"), insert implicit "*"
    if (!equation_buffer.empty() && isValueLikeToken(equation_buffer.back())) {
        equation_buffer.push_back("*");
    }

    equation_buffer.push_back("ANS");

    // Treat it as a completed value
    new_number = true;
    number_is_negative = false;
    dp_used = false;

    updateDisplay();
}

void MainWindow::on_button_equals_clicked() {
    std::vector<std::string> eval_tokens = equation_buffer;

    // commit current entry
    if (!new_number) {
        if (number_is_negative) { eval_tokens.push_back("("); eval_tokens.push_back("-"); }
        for (const auto& c : numeric_input_buffer) eval_tokens.push_back(c);
        if (number_is_negative) eval_tokens.push_back(")");
    }

    // auto-close
    for (int i = 0; i < open_parens; ++i) eval_tokens.push_back(")");
    open_parens = 0;

    // strip trailing op
    auto isOpToken = [](const std::string& t) {
        return t == "+" || t == "-" || t == "*" || t == "/" || t == "mod"; // ← add "mod"
        };
    if (!eval_tokens.empty() && isOpToken(eval_tokens.back()))
        eval_tokens.pop_back();

    // fallback to entry only
    if (eval_tokens.empty()) {
        if (number_is_negative) { eval_tokens.push_back("("); eval_tokens.push_back("-"); }
        for (const auto& c : numeric_input_buffer) eval_tokens.push_back(c);
        if (number_is_negative) eval_tokens.push_back(")");
    }

    // show pretty (with sin, √, ×, ÷, etc.)
    ui->equationLabel->setText(pretty_equation_from_tokens(eval_tokens) + " =");

    // make evaluator-safe
    eval_tokens = expandDisplayFuncs(eval_tokens);

    // flatten
    std::string final_eq;
    for (const auto& t : eval_tokens) final_eq += t;

    // evaluate
    BigFloat res = evaluateFromInfix(final_eq);
    last_answer = res;
    just_evaluated = true;

    std::string raw = mpf_to_string(res, 80); // raw, high digits
    std::string disp = g_eval_div0 ? "undefined" : format_for_display(res, 20, 20);

    // show to user
    if (!last_eval_error.empty()) {
        ui->answerInputLabel->setText(QString::fromStdString(last_eval_error));
        last_eval_error.clear();
        return;
    }
    else {
        ui->answerInputLabel->setText(QString::fromStdString(disp));
    }

    //// keep raw for chaining
    //if (!g_eval_div0) {
    //    std::string ans_str = raw; // not disp
    //    equation_buffer.clear();
    //    for (char ch : ans_str) equation_buffer.push_back(std::string(1, ch));
    //}
    //else {
    //    equation_buffer = { "0" };
    //}

    // keep symbolic ANS for chaining (displayed as "Ans"); evaluator will
    // replace ANS with the numeric value when needed.
    if (!g_eval_div0) {
        equation_buffer.clear();
        equation_buffer.push_back("ANS");
    }
    else {
        equation_buffer = { "0" };
    }

    // also keep raw in entry
    numeric_input_buffer = { g_eval_div0 ? std::string("0") : raw };

    number_is_negative = false;
    dp_used = false;

    just_evaluated = true;
    just_evaluated_full = true; // mark that a full evaluation just happened
    new_number = true;
}


void MainWindow::on_button_memory_add_clicked() {
    // M+: memory += current_entry
    memory += current_entry_to_big();
    // (Optional) visual cue could be added here if you have a label
    ui->memory_clear->setEnabled(true);
    ui->memory_recall->setEnabled(true);
}

void MainWindow::on_button_memory_subtract_clicked() {
    // M-: memory -= current_entry
    memory -= current_entry_to_big();
    ui->memory_clear->setEnabled(true);
    ui->memory_recall->setEnabled(true);
}

void MainWindow::on_button_memory_recall_clicked() {
    // MR: recall memory into entry
    load_entry_from_big(memory);
    updateDisplay();
}

void MainWindow::on_button_memory_clear_clicked() {
    // MC: clear memory
    memory = 0;
    ui->memory_clear->setEnabled(false);
    ui->memory_recall->setEnabled(false);
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
    bool afterOpOrStart =
        (lc == 0 || lc == '(' || lc == '+' || lc == '-' ||
            lc == '*' || lc == '/' || lc == '^' || lc == ',');

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
    // 0 = Deg, 1 = Rad, 2 = Grad
    if (i == 1)      g_angle_unit = ANG_RAD;
    else if (i == 2) g_angle_unit = ANG_GRAD;
    else             g_angle_unit = ANG_DEG;

    // Optional: re-render current display so inverse trig answers show in the new unit
    updateDisplay();
}

void MainWindow::on_button_absolute_value_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after abs() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_ABS");
    equation_buffer.push_back("(");
    open_parens++; // track open '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_constant_e_clicked() {
    // If we just finished "=", start a fresh equation
    if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    // If a number is currently being typed, commit it to the equation
    if (!new_number) {
        if (number_is_negative) equation_buffer.push_back("(");
        if (number_is_negative) equation_buffer.push_back("-");
        for (const auto& c : numeric_input_buffer)
            equation_buffer.push_back(c);
        if (number_is_negative) equation_buffer.push_back(")");
        new_number = true;
    }

    // If previous token is a value (number, ANS, π, e, or ")"), insert implicit "*"
    if (!equation_buffer.empty() && isValueLikeToken(equation_buffer.back())) {
        equation_buffer.push_back("*");
    }

    // Append e as a symbolic constant
    equation_buffer.push_back("FUNC_E");

    // Reset entry state
    new_number = true;
    just_evaluated = false;
    dp_used = false;
    number_is_negative = false;

    updateDisplay();
}
void MainWindow::on_button_constant_pi_clicked() {
    // Start a new equation if we just finished one with '='
    if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    // Commit current number if the user was typing one
    if (!new_number) {
        if (number_is_negative) equation_buffer.push_back("(");
        if (number_is_negative) equation_buffer.push_back("-");
        for (const auto& c : numeric_input_buffer)
            equation_buffer.push_back(c);
        if (number_is_negative) equation_buffer.push_back(")");
        new_number = true;
    }

    // Implicit multiplication if previous token is a value
    if (!equation_buffer.empty() && isValueLikeToken(equation_buffer.back())) {
        equation_buffer.push_back("*");
    }

    // Append π as symbolic constant
    equation_buffer.push_back("FUNC_PI");

    // Reset entry state
    new_number = true;
    just_evaluated = false;
    dp_used = false;
    number_is_negative = false;

    updateDisplay();
}

void MainWindow::on_button_cosine_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after sin() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_COS");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_exponent_scientific_clicked() {
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }
    // Commit current number if present
    if (!new_number) {
        if (number_is_negative) { equation_buffer.push_back("("); equation_buffer.push_back("-"); }
        for (const auto& c : numeric_input_buffer) equation_buffer.push_back(c);
        if (number_is_negative) equation_buffer.push_back(")");
        new_number = true;
    }
    else if (equation_buffer.empty()) {
        // If nothing entered yet, start with 1 × 10^
        equation_buffer.push_back("1");
    }

    // Append × 10 ^
    equation_buffer.push_back("*");
    equation_buffer.push_back("10");
    equation_buffer.push_back("^");

    // Prepare to type exponent
    dp_used = false;
    number_is_negative = false;
    new_number = true;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_exponential_clicked()
{
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        just_evaluated_full = false;
    }

    // Prevent buffer clearing after "="
    just_evaluated_full = false;
    just_evaluated = false;

    // Commit current number before inserting ^
    if (!new_number) {
        if (number_is_negative) equation_buffer.push_back("(");
        if (number_is_negative) equation_buffer.push_back("-");
        for (const auto& c : numeric_input_buffer)
            equation_buffer.push_back(c);
        if (number_is_negative) equation_buffer.push_back(")");
        new_number = true;
    }

    // Append caret operator for exponentiation
    equation_buffer.push_back("^");

    // Prepare for next numeric entry
    dp_used = false;
    number_is_negative = false;
    new_number = true;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
//void MainWindow::on_button_exponential_base10_clicked() {
//    std::string x = concat_numeric_input_buffer_content();
//
//    // Prevent clearing the equation after abs() if we just finished "="
//    just_evaluated_full = false;
//
//    equation_buffer.push_back("10");
//    equation_buffer.push_back("^");
//
//    if (!new_number) {
//        equation_buffer.push_back(x);
//    }
//
//    new_number = true;
//    updateDisplay();
//}
void MainWindow::on_button_exponential_base10_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the entire equation if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("10");
    equation_buffer.push_back("^");

    if (!new_number) {
        // If the current entry is negative, wrap it in parentheses
        if (number_is_negative || (!x.empty() && x[0] == '-')) {
            equation_buffer.push_back("(");
            equation_buffer.push_back("-");
            for (size_t i = 1; i < x.size(); ++i) {
                equation_buffer.push_back(std::string(1, x[i]));
            }
            equation_buffer.push_back(")");
        }
        else {
            for (char c : x) {
                equation_buffer.push_back(std::string(1, c));
            }
        }
    }

    new_number = true;
    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_exponential_natural_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the entire equation if we just finished "="
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_E");
    equation_buffer.push_back("^");

    if (!new_number) {
        // If the current entry is negative, wrap it in parentheses
        if (number_is_negative || (!x.empty() && x[0] == '-')) {
            equation_buffer.push_back("(");
            equation_buffer.push_back("-");
            for (size_t i = 1; i < x.size(); ++i) {
                equation_buffer.push_back(std::string(1, x[i]));
            }
            equation_buffer.push_back(")");
        }
        else {
            for (char c : x) {
                equation_buffer.push_back(std::string(1, c));
            }
        }
    }

    new_number = true;
    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_factorial_clicked() {
    /*std::string x = concat_numeric_input_buffer_content();
    equation_buffer.push_back("FUNC_FACT");
    equation_buffer.push_back("(");
    equation_buffer.push_back(x);
    equation_buffer.push_back(")");
    updateDisplay();*/

    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after fact() if we just finished "="
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_FACT");
    equation_buffer.push_back("(");
    open_parens++; // track open '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance
    }

    BigFloat v(x);
    long long n = static_cast<long long>(v.get_d());
    if (n < 0) {
        ui->answerInputLabel->setText("Error: fact(n) (n<0)");
        return;
    }

    BigFloat r = 1;
    for (long long i = 2; i <= n; ++i)
        r *= BigFloat(static_cast<double>(i)); // fixed

    // load_entry_from_big(r);

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_hyp_cosine_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after cosh() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_COSH");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_hyp_sine_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after sinh() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_SINH");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_hyp_tangent_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after tanh() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_TANH");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_inverse_cosine_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after acos() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_ACOS");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_inverse_hyp_sine_clicked() {
    std::string x = concat_numeric_input_buffer_content();
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) { equation_buffer.clear(); just_evaluated_full = false; }

    equation_buffer.push_back("FUNC_ASINH");
    equation_buffer.push_back("(");
    open_parens++;
    if (!new_number) {                 // there is a number ready in the entry buffer
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--;
    }
    new_number = true; number_is_negative = false; dp_used = false;
    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}

void MainWindow::on_button_inverse_hyp_cosine_clicked() {
    std::string x = concat_numeric_input_buffer_content();
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) { equation_buffer.clear(); just_evaluated_full = false; }

    equation_buffer.push_back("FUNC_ACOSH");
    equation_buffer.push_back("(");
    open_parens++;
    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--;
    }
    new_number = true; number_is_negative = false; dp_used = false;
    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}

void MainWindow::on_button_inverse_hyp_tangent_clicked() {
    std::string x = concat_numeric_input_buffer_content();
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) { equation_buffer.clear(); just_evaluated_full = false; }

    equation_buffer.push_back("FUNC_ATANH");
    equation_buffer.push_back("(");
    open_parens++;
    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--;
    }
    new_number = true; number_is_negative = false; dp_used = false;
    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_inverse_sine_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after asin() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_ASIN");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_inverse_tangent_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after atan() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_ATAN");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
/*
void MainWindow::on_button_logarithm_common_clicked() {
    std::string x = concat_numeric_input_buffer_content();
    equation_buffer.push_back("FUNC_LOG10");
    equation_buffer.push_back("(");
    equation_buffer.push_back(x);
    equation_buffer.push_back(")");
    updateDisplay();

    BigFloat v(x);
    if (v <= 0) {
        ui->answerInputLabel->setText("Error: log(<=0)");
        return;
    }
    BigFloat r = ::log10(v.get_d());
    // load_entry_from_big(r);

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    updateDisplay();
}
*/
void MainWindow::on_button_logarithm_common_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after log() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_LOG10");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
/*
void MainWindow::on_button_logarithm_natural_clicked() {
    std::string x = concat_numeric_input_buffer_content();
    equation_buffer.push_back("FUNC_LN");
    equation_buffer.push_back("(");
    equation_buffer.push_back(x);
    equation_buffer.push_back(")");
    updateDisplay();

    BigFloat v(x);
    if (v <= 0) {
        ui->answerInputLabel->setText("Error: ln(<=0)");
        return;
    }
    BigFloat r = ::log(v.get_d());
    // load_entry_from_big(r);

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    updateDisplay();
}
*/
void MainWindow::on_button_logarithm_natural_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after log() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_LN");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_modulus_clicked() {
    appendOperator(" mod ");  // same flow as +, -, *, /
}
//void MainWindow::on_button_percent_clicked() {
//    // basic %: x -> x/100
//    std::string x = concat_numeric_input_buffer_content();
//    equation_buffer.push_back("FUNC_PERCENT");
//    equation_buffer.push_back("(");
//    equation_buffer.push_back(x);
//    equation_buffer.push_back(")");
//    updateDisplay();
//
//    BigFloat v(x);
//    BigFloat r = v / 100;
//    // load_entry_from_big(r);
//
//    new_number = true;
//    number_is_negative = false;
//    dp_used = false;
//
//    updateDisplay();
//}
void MainWindow::on_button_percent_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after %() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_PERCENT");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}


void MainWindow::on_button_random_number_clicked() {
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    // Generate random double in [0,1)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);

    double r = dist(gen);

    // Format to 20 digits after the decimal point
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(20) << r;
    std::string s = oss.str();

    // Load into display / entry
    BigFloat v(s);
    load_entry_from_big(v);

    new_number = false;
    number_is_negative = (r < 0);
    dp_used = (std::floor(r) != r);

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_reciprocal_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after reciprocal() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_RECIP");
    equation_buffer.push_back("(");
    open_parens++; // track open '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_sine_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after sin() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_SIN");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_square_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        just_evaluated_full = false;
    }

    // If user has just typed a number, commit it to the equation buffer
    if (!new_number) {
        if (number_is_negative) equation_buffer.push_back("(");
        if (number_is_negative) equation_buffer.push_back("-");
        for (const auto& c : numeric_input_buffer) equation_buffer.push_back(c);
        if (number_is_negative) equation_buffer.push_back(")");
        new_number = true;
    }

    // Append ^ 2 operator tokens
    equation_buffer.push_back("^");
    equation_buffer.push_back("2");

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_square_root_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after sin() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_SQRT");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_tangent_clicked() {
    std::string x = concat_numeric_input_buffer_content();

    // Prevent clearing the equation after sin() if we just finished "="
    // Prevent previous number from persisting in function 
    bool was_full_eval = just_evaluated_full; if (just_evaluated_full) {
        equation_buffer.clear();
        just_evaluated_full = false;
    }

    equation_buffer.push_back("FUNC_TAN");
    equation_buffer.push_back("(");
    open_parens++; // keep track of unclosed '('

    if (!new_number) {
        equation_buffer.push_back(x);
        equation_buffer.push_back(")");
        open_parens--; // balance parentheses
    }

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    if (was_full_eval) {
        ui->equationLabel->setText(pretty_equation_from_tokens(equation_buffer));
    }
    else {
        updateDisplay();
    }
}
void MainWindow::on_button_x_th_root_clicked() {
    // Commit the current number (the root index x)
    std::string x = concat_numeric_input_buffer_content();

    // Append the function token for evaluation
    equation_buffer.push_back("FUNC_XROOT");
    equation_buffer.push_back("(");
    open_parens++;
    equation_buffer.push_back(x);
    equation_buffer.push_back(","); // comma separates x and y inside parentheses
    // Wait for user to enter the next number (the radicand y)

    BigFloat v(x);
    if (v < 0) {
        ui->answerInputLabel->setText("Error: xthroot(<0)");
        return;
    }
    mpf_t tmp; mpf_init(tmp);
    mpf_sqrt(tmp, v.get_mpf_t());
    BigFloat r(tmp); mpf_clear(tmp);

    new_number = true;
    number_is_negative = false;
    dp_used = false;

    updateDisplay();
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
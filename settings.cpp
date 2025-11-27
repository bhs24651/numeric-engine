#include "settings.h"
#include "ui_settings.h"
#include <QSettings>
#include <QColorDialog>
#include <QMessageBox>
#include <QPushButton>

void settings::loadSettings()
{
    QSettings s("NumericEngine", "NumericEngine");

    primaryButtonColor = s.value("colors/primaryButton", primaryButtonColor).value<QColor>();
    primaryTextColor = s.value("colors/primaryText", primaryTextColor).value<QColor>();
    secondaryButtonColor = s.value("colors/secondaryButton", secondaryButtonColor).value<QColor>();
    secondaryTextColor = s.value("colors/secondaryText", secondaryTextColor).value<QColor>();
    calculatorBackgroundColor = s.value("colors/calcBg", calculatorBackgroundColor).value<QColor>();
    displayTextColor = s.value("colors/displayText", displayTextColor).value<QColor>();
    displayBackgroundColor = s.value("colors/displayBg", displayBackgroundColor).value<QColor>();

    buttonFont = s.value("fonts/buttonFont", buttonFont).value<QFont>();
    displayFont = s.value("fonts/displayFont", displayFont).value<QFont>();

    // Apply immediately to preview buttons (optional)
    ui->choose_button_font_combobox->setCurrentFont(buttonFont);
    ui->choose_display_font_combobox->setCurrentFont(displayFont);

    // You can add a preview on buttons if desired
}

void settings::saveSettings()
{
    QSettings s("NumericEngine", "NumericEngine");

    s.setValue("colors/primaryButton", primaryButtonColor);
    s.setValue("colors/primaryText", primaryTextColor);
    s.setValue("colors/secondaryButton", secondaryButtonColor);
    s.setValue("colors/secondaryText", secondaryTextColor);
    s.setValue("colors/calcBg", calculatorBackgroundColor);
    s.setValue("colors/displayText", displayTextColor);
    s.setValue("colors/displayBg", displayBackgroundColor);

    s.setValue("fonts/buttonFont", buttonFont);
    s.setValue("fonts/displayFont", displayFont);
}

void resetToDefaults() {
    QSettings s("NumericEngine", "NumericEngine");

    s.setValue("colors/primaryButton", QColor("#fbfbfb"));
    s.setValue("colors/primaryText", QColor("#1b1b1b"));
    s.setValue("colors/secondaryButton", QColor("#dbdbdb"));
    s.setValue("colors/secondaryText", QColor("#1b1b1b"));
    s.setValue("colors/calcBg", QColor("#f3f3f3"));
    s.setValue("colors/displayText", QColor("#1b1b1b"));
    s.setValue("colors/displayBg", QColor("#fbfbfb"));

    s.setValue("fonts/buttonFont", QFont("DejaVu Sans"));
    s.setValue("fonts/displayFont", QFont("DejaVu Sans"));
}

settings::settings(QWidget* parent)
    : QMainWindow(parent)
{
    ui = new Ui::settingsClass();
    ui->setupUi(this);

    // load user settings (overwrites defaults if existing)
    loadSettings();

    // QObject::connect(ui->auto_format, &QRadioButton::toggled, this, &settings::on_auto_format_toggled);
    // QObject::connect(ui->round_format, &QRadioButton::toggled, this, &settings::on_round_format_toggled);
    // QObject::connect(ui->scientific_format, &QRadioButton::toggled, this, &settings::on_scientific_format_toggled);

    // QObject::connect(ui->round_spinBox, &QSpinBox::valueChanged, this, &settings::on_round_spinBox_valueChanged);
    // QObject::connect(ui->scientific_spinBox, &QSpinBox::valueChanged, this, &settings::on_scientific_spinBox_valueChanged);

    // Connect QPushButtons
    QObject::connect(ui->choose_color_primary_button, &QPushButton::clicked, this, &settings::handle_primary_button_color);
    QObject::connect(ui->choose_color_primary_button_text, &QPushButton::clicked, this, &settings::handle_primary_button_text_color);
    QObject::connect(ui->choose_color_secondary_button, &QPushButton::clicked, this, &settings::handle_secondary_button_color);
    QObject::connect(ui->choose_color_secondary_button_text, &QPushButton::clicked, this, &settings::handle_secondary_button_text_color);
    QObject::connect(ui->choose_color_calculator_background, &QPushButton::clicked, this, &settings::handle_calculator_background_color);
    QObject::connect(ui->choose_color_display_text, &QPushButton::clicked, this, &settings::handle_display_text_color);
    QObject::connect(ui->choose_color_display_background, &QPushButton::clicked, this, &settings::handle_display_background_color);

    // Connect QFontComboBoxes
    QObject::connect(ui->choose_button_font_combobox, &QFontComboBox::currentFontChanged, this, &settings::on_choose_button_font_combobox_currentFontChanged);
    QObject::connect(ui->choose_display_font_combobox, &QFontComboBox::currentFontChanged, this, &settings::on_choose_display_font_combobox_currentFontChanged);

    QObject::connect(ui->reset_customization_to_defaults, &QPushButton::clicked, this, &settings::handle_reset_customization_to_defaults);

    // Features that aren't visible have been temporarily hidden
    ui->round_format->setVisible(false);
    ui->scientific_format->setVisible(false);
    ui->round_spinBox->setVisible(false);
    ui->scientific_spinBox->setVisible(false);
    ui->dp_label->setVisible(false);
    ui->sf_label->setVisible(false);
}

settings::~settings()
{
    delete ui;
}

// TODO: Add Functionality to Display Format Settings

void settings::on_auto_format_toggled() {}
void settings::on_round_format_toggled() {}
void settings::on_scientific_format_toggled() {}

void settings::on_round_spinBox_valueChanged() {}
void settings::on_scientific_spinBox_valueChanged() {}

void settings::handle_primary_button_color()
{
    QColor c = QColorDialog::getColor(primaryButtonColor, this, "Choose Primary Button Color");
    if (!c.isValid()) return;
    primaryButtonColor = c;
    saveSettings();
    emit customizationChanged();
}

void settings::handle_primary_button_text_color()
{
    QColor c = QColorDialog::getColor(primaryTextColor, this, "Choose Primary Text Color");
    if (!c.isValid()) return;
    primaryTextColor = c;
    saveSettings();
    emit customizationChanged();
}

void settings::handle_secondary_button_color()
{
    QColor c = QColorDialog::getColor(secondaryButtonColor, this, "Choose Secondary Button Color");
    if (!c.isValid()) return;
    secondaryButtonColor = c;
    saveSettings();
    emit customizationChanged();
}

void settings::handle_secondary_button_text_color()
{
    QColor c = QColorDialog::getColor(secondaryTextColor, this, "Choose Secondary Text Color");
    if (!c.isValid()) return;
    secondaryTextColor = c;
    saveSettings();
    emit customizationChanged();
}

void settings::handle_calculator_background_color()
{
    QColor c = QColorDialog::getColor(calculatorBackgroundColor, this, "Choose Calculator Background Color");
    if (!c.isValid()) return;
    calculatorBackgroundColor = c;
    saveSettings();
    emit customizationChanged();
}

void settings::handle_display_text_color()
{
    QColor c = QColorDialog::getColor(displayTextColor, this, "Choose Display Text Color");
    if (!c.isValid()) return;
    displayTextColor = c;
    saveSettings();
    emit customizationChanged();
}

void settings::handle_display_background_color()
{
    QColor c = QColorDialog::getColor(displayBackgroundColor, this, "Choose Display Background Color");
    if (!c.isValid()) return;
    displayBackgroundColor = c;
    saveSettings();
    emit customizationChanged();
}

void settings::on_choose_button_font_combobox_currentFontChanged(const QFont& font)
{
    buttonFont = font;
    saveSettings();
    emit customizationChanged();
}

void settings::on_choose_display_font_combobox_currentFontChanged(const QFont& font)
{
    displayFont = font;
    saveSettings();
    emit customizationChanged();
}

void settings::handle_reset_customization_to_defaults() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::warning(this, "Confirm Reset to Defaults", "Are you sure you want to proceed?\nTHIS ACTION CANNOT BE UNDONE!!!",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        resetToDefaults();
        loadSettings();
    }
}

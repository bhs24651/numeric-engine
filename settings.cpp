#include "settings.h"
#include "ui_settings.h"

settings::settings(QWidget* parent)
    : QMainWindow(parent)
{
    // The compiler can now find the full definition of Ui::settings
    ui = new Ui::settingsClass();
    ui->setupUi(this);

	QObject::connect(ui->auto_format, &QRadioButton::toggled, this, &settings::on_auto_format_toggled);
	QObject::connect(ui->round_format, &QRadioButton::toggled, this, &settings::on_round_format_toggled);
	QObject::connect(ui->scientific_format, &QRadioButton::toggled, this, &settings::on_scientific_format_toggled);

	QObject::connect(ui->round_spinBox, &QSpinBox::valueChanged, this, &settings::on_round_spinBox_valueChanged);
	QObject::connect(ui->scientific_spinBox, &QSpinBox::valueChanged, this, &settings::on_scientific_spinBox_valueChanged);

    // Connect QPushButtons
    QObject::connect(ui->choose_color_primary_button, &QPushButton::clicked, this, &settings::on_choose_color_primary_button_clicked);
    QObject::connect(ui->choose_color_primary_button_text, &QPushButton::clicked,this, &settings::on_choose_color_primary_button_text_clicked);
    QObject::connect(ui->choose_color_secondary_button, &QPushButton::clicked, this, &settings::on_choose_color_secondary_button_clicked);
    QObject::connect(ui->choose_color_secondary_button_text, &QPushButton::clicked, this, &settings::on_choose_color_secondary_button_text_clicked);
    QObject::connect(ui->choose_color_calculator_background, &QPushButton::clicked, this, &settings::on_choose_color_calculator_background_clicked);
    QObject::connect(ui->choose_color_display_text, &QPushButton::clicked, this, &settings::on_choose_color_display_text_clicked);
    QObject::connect(ui->choose_color_display_background, &QPushButton::clicked, this, &settings::on_choose_color_display_background_clicked);

    // Connect QFontComboBoxes
    QObject::connect(ui->choose_button_font_combobox, &QFontComboBox::currentFontChanged, this, &settings::on_choose_button_font_combobox_currentFontChanged);
    QObject::connect(ui->choose_display_font_combobox, &QFontComboBox::currentFontChanged, this, &settings::on_choose_display_font_combobox_currentFontChanged);
}

settings::~settings()
{
    delete ui;
}

void settings::on_auto_format_toggled() {}
void settings::on_round_format_toggled() {}
void settings::on_scientific_format_toggled() {}

void settings::on_round_spinBox_valueChanged() {}
void settings::on_scientific_spinBox_valueChanged() {}

void settings::on_choose_color_primary_button_clicked() {}
void settings::on_choose_color_primary_button_text_clicked() {}
void settings::on_choose_color_secondary_button_clicked() {}
void settings::on_choose_color_secondary_button_text_clicked() {}
void settings::on_choose_color_calculator_background_clicked() {}
void settings::on_choose_color_display_text_clicked() {}
void settings::on_choose_color_display_background_clicked() {}

void settings::on_choose_button_font_combobox_currentFontChanged(const QFont& font) {}
void settings::on_choose_display_font_combobox_currentFontChanged(const QFont& font) {}

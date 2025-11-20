#include "settings.h"

settings::settings(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
}

settings::~settings()
{
	// QObject::connect(ui->wordlen_word, &QRadioButton::toggled, this, &MainWindow::on_wordlen_toggled);
	QObject::connect(ui.auto_format, &QRadioButton::toggled, this, &settings::on_auto_format_toggled);
	QObject::connect(ui.round_format, &QRadioButton::toggled, this, &settings::on_round_format_toggled);
	QObject::connect(ui.scientific_format, &QRadioButton::toggled, this, &settings::on_scientific_format_toggled);

	QObject::connect(ui.round_spinBox, &QSpinBox::valueChanged, this, &settings::on_round_spinBox_valueChanged);
	QObject::connect(ui.scientific_spinBox, &QSpinBox::valueChanged, this, &settings::on_scientific_spinBox_valueChanged);
}

void settings::on_auto_format_toggled() {}
void settings::on_round_format_toggled() {}
void settings::on_scientific_format_toggled() {}

void settings::on_round_spinBox_valueChanged() {}
void settings::on_scientific_spinBox_valueChanged() {}
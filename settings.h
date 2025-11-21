#pragma once

#include <QMainWindow>
#include <QRadioButton>
#include <QPushButton>
#include <QFontComboBox>
// #include "ui_settings.h"

namespace Ui { class settingsClass; }

class settings : public QMainWindow
{
	Q_OBJECT

public:
	settings(QWidget *parent = nullptr);
	~settings();

private slots:
	void on_auto_format_toggled();
	void on_round_format_toggled();
	void on_scientific_format_toggled();

	void on_round_spinBox_valueChanged();
	void on_scientific_spinBox_valueChanged();

	// Color buttons
	void on_choose_color_primary_button_clicked();
	void on_choose_color_primary_button_text_clicked();
	void on_choose_color_secondary_button_clicked();
	void on_choose_color_secondary_button_text_clicked();
	void on_choose_color_calculator_background_clicked();
	void on_choose_color_display_text_clicked();
	void on_choose_color_display_background_clicked();

	// Font selectors
	void on_choose_button_font_combobox_currentFontChanged(const QFont& font);
	void on_choose_display_font_combobox_currentFontChanged(const QFont& font);

private:
	Ui::settingsClass *ui;
};

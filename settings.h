#pragma once

#include <QMainWindow>
#include <QRadioButton>
#include <QPushButton>
#include <QFontComboBox>
#include "ui_settings.h"

namespace Ui { class settingsClass; }

class settings : public QMainWindow
{
	Q_OBJECT

public:
	settings(QWidget* parent = nullptr);
	~settings();

	// functions to allow the calculator to read user settings
	QColor getPrimaryButtonColor() const { return primaryButtonColor; }
	QColor getPrimaryTextColor() const { return primaryTextColor; }
	QColor getSecondaryButtonColor() const { return secondaryButtonColor; }
	QColor getSecondaryTextColor() const { return secondaryTextColor; }
	QColor getCalculatorBackgroundColor() const { return calculatorBackgroundColor; }
	QColor getDisplayTextColor() const { return displayTextColor; }
	QColor getDisplayBackgroundColor() const { return displayBackgroundColor; }

	QFont getButtonFont() const { return buttonFont; }
	QFont getDisplayFont() const { return displayFont; }

private slots:
	void on_auto_format_toggled();
	void on_round_format_toggled();
	void on_scientific_format_toggled();

	void on_round_spinBox_valueChanged();
	void on_scientific_spinBox_valueChanged();

	// Color buttons
	void handle_primary_button_color();
	void handle_primary_button_text_color();
	void handle_secondary_button_color();
	void handle_secondary_button_text_color();
	void handle_calculator_background_color();
	void handle_display_text_color();
	void handle_display_background_color();

	// Font selectors
	void on_choose_button_font_combobox_currentFontChanged(const QFont& font);
	void on_choose_display_font_combobox_currentFontChanged(const QFont& font);

	void handle_reset_customization_to_defaults();

private:
	Ui::settingsClass* ui;

	QColor primaryButtonColor = QColor("#fbfbfb");
	QColor primaryTextColor = QColor("#1b1b1b");
	QColor secondaryButtonColor = QColor("#dbdbdb");
	QColor secondaryTextColor = QColor("#1b1b1b");
	QColor calculatorBackgroundColor = QColor("#f3f3f3");
	QColor displayTextColor = QColor("#1b1b1b");
	QColor displayBackgroundColor = QColor("#fbfbfb");

	QFont buttonFont = QFont("DejaVu Sans");
	QFont displayFont = QFont("DejaVu Sans");

	void loadSettings();
	void saveSettings();

signals:
	void customizationChanged();
};

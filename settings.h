#pragma once

#include <QMainWindow>
#include "ui_settings.h"

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

private:
	Ui::settingsClass ui;
};


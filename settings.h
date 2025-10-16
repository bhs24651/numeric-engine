#pragma once

#include <QMainWindow>
#include "ui_settings.h"

class settings : public QMainWindow
{
	Q_OBJECT

public:
	settings(QWidget *parent = nullptr);
	~settings();

private:
	Ui::settingsClass ui;
};


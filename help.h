#pragma once

#include <QMainWindow>
#include "ui_help.h"

class help : public QMainWindow
{
	Q_OBJECT

public:
	help(QWidget *parent = nullptr);
	~help();

private:
	Ui::helpClass ui;
};


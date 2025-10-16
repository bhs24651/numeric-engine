#pragma once

#include <QMainWindow>
#include "ui_converter.h"

class converter : public QMainWindow
{
	Q_OBJECT

public:
	converter(QWidget *parent = nullptr);
	~converter();

private:
	Ui::converterClass ui;
};


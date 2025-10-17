#pragma once

#include <QMainWindow>
#include "ui_about.h"

class about : public QMainWindow
{
	Q_OBJECT

public:
	about(QWidget *parent = nullptr);
	~about();

private:
	Ui::aboutClass ui;
};


#include "help.h"

help::help(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose);   // ADD THIS LINE
}

help::~help()
{}


#include "converter.h"

converter::converter(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	// Explicitly set the window flags to a regular window
	setWindowFlags(Qt::Window);
}

converter::~converter()
{}

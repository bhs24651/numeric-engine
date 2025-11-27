#include "about.h"

about::about(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose);   // ADD THIS LINE
}

about::~about()
{}


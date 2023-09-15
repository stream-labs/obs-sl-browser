#include "SlBrowserWidget.h"

#include <QDialog>
#include <QCloseEvent>

SlBrowserWidget::SlBrowserWidget() {}

void SlBrowserWidget::closeEvent(QCloseEvent *event) /*override*/
{
	event->ignore();
}

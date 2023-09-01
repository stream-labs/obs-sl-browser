#include "BrowserDockCaster.h"

#include "../../UI/window-dock-browser.hpp"

QCefWidget *BrowserDockCaster::getQCefWidget(QDockWidget *ptr)
{
	return static_cast<BrowserDock *>(ptr)->cefWidget.get();
}

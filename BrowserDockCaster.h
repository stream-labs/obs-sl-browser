#pragma once

class QCefWidget;
class QDockWidget;

// There's a variable called 'cef' in the top of a header in obs that blocks the cef namespace from being included, this is a workaround
class BrowserDockCaster
{
public:
	static QCefWidget *getQCefWidget(QDockWidget *ptr);
};

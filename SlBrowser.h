#pragma once

#include "browser-client.hpp"
#include "browser-app.hpp"

#include <QWidget>

class SlBrowser
{
public:
	void run(int argc, char *argv[]);

	static void CreateCefBrowser(int arg);

public:
	QWidget *m_widget = nullptr;
	CefRefPtr<BrowserApp> app = nullptr;
	CefRefPtr<CefBrowser> m_browser = nullptr;
	CefRefPtr<BrowserClient> browserClient = nullptr;

public:
	static SlBrowser &instance()
	{
		static SlBrowser instance;
		return instance;
	}

private:
	SlBrowser();
	~SlBrowser();

	void browserInit();
	void browserShutdown();
	void browserManagerThread();

public:
	// Disallow copying
	SlBrowser(const SlBrowser &) = delete;
	SlBrowser &operator=(const SlBrowser &) = delete;
};

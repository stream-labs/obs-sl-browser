#include "SlBrowser.h"
#include "SlBrowser.h"

#include <functional>
#include <sstream>
#include <thread>
#include <mutex>
#include <filesystem>

#include <QTimer>
#include <QPointer>
#include <QVBoxLayout>
#include <QDialog>
#include <QMainWindow>
#include <QAbstractButton>
#include <QPushButton>
#include <QApplication>

#include "browser-scheme.hpp"
#include "browser-version.h"
#include "GrpcBrowser.h"

#include "json11/json11.hpp"
#include "cef-headers.hpp"

#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>

#include <include/base/cef_callback.h>
#include <include/wrapper/cef_closure_task.h>

using namespace std;
using namespace json11;

SlBrowser::SlBrowser() {}

SlBrowser::~SlBrowser() {}

void SlBrowser::run(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Not enough args.\n");
		system("pause");
		return;
	}

	int32_t parentListenPort = atoi(argv[1]);
	int32_t myListenPort = atoi(argv[2]);

	if (!GrpcBrowser::instance().startServer(myListenPort))
	{
		printf("sl-proxy: failed to start grpc server, GetLastError = %d\n", GetLastError());
		return;
	}

	if (!GrpcBrowser::instance().connectToClient(parentListenPort))
	{
		printf("sl-proxy: failed to connected to plugin's grpc server, GetLastError = %d\n", GetLastError());
		return;
	}

	printf("parentListenPort = %d\n", parentListenPort);
	printf("myListenPort = %d\n", myListenPort);

	QApplication a(argc, argv);

	// Create CEF Browser
	auto manager_thread = thread(&SlBrowser::browserManagerThread, this);

	// Create Qt Widget
	m_widget = new QWidget{};
	m_widget->resize(800, 600);
	m_widget->show();

	CefPostTask(TID_UI, base::BindOnce(&CreateCefBrowser, 5));

	// Run Qt Application
	int result = a.exec();
}

void SlBrowser::CreateCefBrowser(int arg)
{
	auto &app = SlBrowser::instance();

	CefWindowInfo window_info;
	CefBrowserSettings browser_settings;
	app.browserClient = new BrowserClient(false);

	CefString url = "C:/Users/srogers/Desktop/index.html";

	// Now set the parent of the CEF browser to the QWidget
	window_info.SetAsChild((HWND)app.m_widget->winId(), CefRect(0, 0, app.m_widget->width(), app.m_widget->height()));
	app.m_browser = CefBrowserHost::CreateBrowserSync(window_info, app.browserClient.get(), url, browser_settings, CefRefPtr<CefDictionaryValue>(), nullptr);

	// For the next 3 seconds keep the window on top
	for (int i = 0; i < 30; ++i)
	{

		::Sleep(100);
	}
}

void SlBrowser::browserInit()
{
	TCHAR moduleFileName[MAX_PATH]{};
	GetModuleFileName(NULL, moduleFileName, MAX_PATH);
	std::filesystem::path fsPath(moduleFileName);

	std::string path = fsPath.remove_filename().string();
	path += "sl-browser-page.exe";

	CefMainArgs args;

	CefSettings settings;
	settings.log_severity = LOGSEVERITY_VERBOSE;

	CefString(&settings.user_agent_product) = "Streamlabs";

	//todo
	CefString(&settings.locale) = "en-US";
	CefString(&settings.accept_language_list) = "en-US,en";

	settings.persist_user_preferences = 1;

	//todo
	CefString(&settings.cache_path) = "C:\\Users\\srogers\\AppData\\Roaming\\obs-studio\\plugin_config\\obs-browser - Copy";

	CefString(&settings.browser_subprocess_path) = path.c_str();

	CefString(&settings.log_file) = "C:/Users/srogers/Desktop/cef.log";
	settings.log_severity = LOGSEVERITY_DEBUG;

	// Set the remote debugging port
	settings.remote_debugging_port = 9123;

	app = new BrowserApp();

	CefExecuteProcess(args, app, nullptr);
	CefInitialize(args, settings, app, nullptr);

	/* Register http://absolute/ scheme handler for older
	 * CEF builds which do not support file:// URLs */
	CefRegisterSchemeHandlerFactory("http", "absolute", new BrowserSchemeHandlerFactory());
}

void SlBrowser::browserShutdown()
{
	CefClearSchemeHandlerFactories();
	CefShutdown();
	app = nullptr;
}

void SlBrowser::browserManagerThread()
{
	browserInit();
	CefRunMessageLoop();
	browserShutdown();
}

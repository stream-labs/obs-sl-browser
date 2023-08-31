#include "PluginJsHandler.h"
#include "JavascriptApi.h"
#include "GrpcPlugin.h"

#include <chrono>
#include <obs.h>
#include <obs-frontend-api.h>

#include <QMainWindow>
#include <QDockWidget>

#include "C:\github\obs-studio\UI\window-dock-browser.hpp"

void PluginJsHandler::start()
{
	m_running = true;
	m_workerThread = std::thread(&PluginJsHandler::workerThread, this);
}

void PluginJsHandler::stop()
{
	m_running = false;

	if (m_workerThread.joinable())
		m_workerThread.join();
}

void PluginJsHandler::pushApiRequest(const std::string &funcName, const std::string &params)
{
	std::lock_guard<std::mutex> grd(m_queueMtx);
	m_queudRequests.push_back({funcName, params});
}

void PluginJsHandler::executeApiRequest(const std::string& funcName, const std::string& params)
{
	std::string err;
	json11::Json jsonParams = json11::Json::parse(params, err);

	if (!err.empty())
	{
		blog(LOG_ERROR, "PluginJsHandler::executeApiRequest invalid params %s", params.c_str());
		return;
	}

	const auto &param1Value = jsonParams["param1"];

	if (param1Value.is_null())
	{
		blog(LOG_ERROR, "PluginJsHandler::executeApiRequest Error: 'param1' key not found. %s", params.c_str());
		return;
	}

	std::string jsonReturnStr;

	switch (JavascriptApi::getFunctionId(funcName))
	{
	case JavascriptApi::JS_PANEL_EXECUTEJAVASCRIPT:
	{
		JS_PANEL_EXECUTEJAVASCRIPT(jsonParams, jsonReturnStr);
		break;
	}
	case JavascriptApi::JS_PANEL_SETURL:
	{
		JS_PANEL_SETURL(jsonParams, jsonReturnStr);
		break;
	}
	}

	// We're done, send callback
	if (param1Value.int_value() > 0)
		GrpcPlugin::instance().getClient()->send_executeCallback(param1Value.int_value(), jsonReturnStr);
}

void PluginJsHandler::workerThread()
{
	while (m_running)
	{
		std::vector<std::pair<std::string, std::string>> latestBatch;

		{
			std::lock_guard<std::mutex> grd(m_queueMtx);
			latestBatch.swap(m_queudRequests);
		}

		if (latestBatch.empty())
		{
			using namespace std::chrono;
			std::this_thread::sleep_for(1ms);
		}
		else
		{
			for (auto &itr : latestBatch)
				executeApiRequest(itr.first, itr.second);
		}
	}
}

void PluginJsHandler::JS_PANEL_EXECUTEJAVASCRIPT(const json11::Json &params, std::string &out_jsonReturn)
{
	printf("JS_PANEL_EXECUTEJAVASCRIPT: %s\n", params.dump().c_str());
}

void PluginJsHandler::JS_PANEL_SETURL(const json11::Json &params, std::string &out_jsonReturn)
{
	printf("JS_PANEL_SETURL: %s\n", params.dump().c_str());

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();

	// Execute this lambda in the context of the main GUI thread.
	QMetaObject::invokeMethod(
		mainWindow,
		[mainWindow]() {
			// This code is executed in the context of the QMainWindow's thread.

			// For example:
			QList<QDockWidget *> docks = mainWindow->findChildren<QDockWidget *>();
			foreach(QDockWidget * dock, docks)
			{
				if (dock->property("uuid").isValid())
				{
					printf("Dock with objectName: %s has a uuid property.\n", dock->objectName().toStdString().c_str());
					BrowserDock *ptr = (BrowserDock *)dock;
					ptr->cefWidget->setURL("streamlabs.com");
				}
			}

			// Or add a dock widget
			// QDockWidget *newDock = new QDockWidget();
			// mainWindow->addDockWidget(Qt::LeftDockWidgetArea, newDock);
		},
		Qt::BlockingQueuedConnection); // BlockingQueuedConnection waits until the slot (in this case, the lambda) has been executed.
}

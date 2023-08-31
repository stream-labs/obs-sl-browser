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

	switch (JavascriptApi::getFunctionId(funcName)) {
	case JavascriptApi::JS_PANEL_EXECUTEJAVASCRIPT: {
		JS_PANEL_EXECUTEJAVASCRIPT(jsonParams, jsonReturnStr);
		break;
	}
	case JavascriptApi::JS_PANEL_SETURL: {
		JS_PANEL_SETURL(jsonParams, jsonReturnStr);
		break;
	}
	case JavascriptApi::JS_QUERY_PANEL_UIDS: {
		JS_QUERY_PANEL_UIDS(jsonParams, jsonReturnStr);
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

void PluginJsHandler::JS_QUERY_PANEL_UIDS(const json11::Json &params, std::string &out_jsonReturn)
{
	blog(LOG_WARNING, "JS_QUERY_PANEL_UIDS start: %s\n", params.dump().c_str());

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();

	// This code is executed in the context of the QMainWindow's thread.
	QMetaObject::invokeMethod(
		mainWindow,
		[mainWindow, &out_jsonReturn]() {

			std::vector<json11::Json> panelUids;

			QList<QDockWidget *> docks = mainWindow->findChildren<QDockWidget *>();
			foreach(QDockWidget * dock, docks)
			{
				if (dock->property("uuid").isValid()) {
					BrowserDock *ptr = (BrowserDock *)dock;
					panelUids.push_back(dock->property("uuid").toString().toStdString());
				}
			}

			json11::Json ret = json11::Json::object({{"result", json11::Json::array(panelUids)}});
			out_jsonReturn = ret.dump();
		},
		Qt::BlockingQueuedConnection);

	blog(LOG_WARNING, "JS_QUERY_PANEL_UIDS output: %s\n", out_jsonReturn.c_str());
}

void PluginJsHandler::JS_PANEL_EXECUTEJAVASCRIPT(const json11::Json &params, std::string &out_jsonReturn)
{
	blog(LOG_WARNING, "JS_PANEL_EXECUTEJAVASCRIPT: %s\n", params.dump().c_str());

	const auto &param2Value = params["param2"];
	const auto &param3Value = params["param3"];

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();

	// This code is executed in the context of the QMainWindow's thread.
	QMetaObject::invokeMethod(
		mainWindow,
		[mainWindow, param2Value, param3Value, &out_jsonReturn]() {
			QList<QDockWidget *> docks = mainWindow->findChildren<QDockWidget *>();
			foreach(QDockWidget * dock, docks)
			{
				if (dock->property("uuid").isValid() &&
				    dock->property("uuid").toString().toStdString() == param3Value.string_value()) {
					BrowserDock *ptr = (BrowserDock *)dock;
					ptr->cefWidget->executeJavaScript(param2Value.string_value().c_str());
					return;
				}
			}

			std::string errorMsg = "Dock '" + param2Value.string_value() + "' not found.";
			json11::Json ret = json11::Json::object({{"error", errorMsg}});
			out_jsonReturn = ret.dump();
		},
		Qt::BlockingQueuedConnection);

	blog(LOG_WARNING, "JS_PANEL_EXECUTEJAVASCRIPT output: %s\n", out_jsonReturn.c_str());
}

void PluginJsHandler::JS_PANEL_SETURL(const json11::Json &params, std::string &out_jsonReturn)
{
	blog(LOG_WARNING, "JS_PANEL_SETURL: %s\n", params.dump().c_str());

	const auto &param2Value = params["param2"];
	const auto &param3Value = params["param3"];

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();

	// This code is executed in the context of the QMainWindow's thread.
	QMetaObject::invokeMethod(mainWindow,
		[mainWindow, param2Value, param3Value, &out_jsonReturn]() {
			
			QList<QDockWidget *> docks = mainWindow->findChildren<QDockWidget *>();
			foreach(QDockWidget * dock, docks)
			{
				if (dock->property("uuid").isValid() &&
				    dock->property("uuid").toString().toStdString() == param3Value.string_value())
				{
					BrowserDock *ptr = (BrowserDock *)dock;
					ptr->cefWidget->setURL(param2Value.string_value().c_str());
					return;
				}
			}

			std::string errorMsg = "Dock '" + param2Value.string_value() + "' not found.";
			json11::Json ret = json11::Json::object({{"error", errorMsg}});
			out_jsonReturn = ret.dump();
		},
		Qt::BlockingQueuedConnection);

	blog(LOG_WARNING, "JS_PANEL_SETURL output: %s\n", out_jsonReturn.c_str());
}

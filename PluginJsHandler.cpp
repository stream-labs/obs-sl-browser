#include "PluginJsHandler.h"
#include "JavascriptApi.h"
#include "GrpcPlugin.h"

#include <chrono>
#include <obs.h>

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
	json11::Json jsonParams;
	jsonParams.parse(params, err);

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

	switch (JavascriptApi::getFunctionId(funcName))
	{
	case JavascriptApi::JS_PANEL_EXECUTEJAVASCRIPT:
	{
		JS_PANEL_EXECUTEJAVASCRIPT(jsonParams);
		break;
	}
	case JavascriptApi::JS_PANEL_SETURL:
	{
		JS_PANEL_SETURL(jsonParams);
		break;
	}
	}

	// We're done, send callback
	if (param1Value.int_value() > 0)
		GrpcPlugin::instance().getClient()->send_executeCallback(param1Value.int_value());
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

void PluginJsHandler::JS_PANEL_EXECUTEJAVASCRIPT(const json11::Json &params)
{
	printf("JS_PANEL_EXECUTEJAVASCRIPT: %s\n", params.dump().c_str());
}

void PluginJsHandler::JS_PANEL_SETURL(const json11::Json &params)
{

	printf("JS_PANEL_SETURL: %s\n", params.dump().c_str());
}

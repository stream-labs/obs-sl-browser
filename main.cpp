/******************************************************************************
 Copyright (C) 2014 by John R. Bradley <jrb@turrettech.com>
 Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <functional>
#include <sstream>
#include <thread>
#include <mutex>
#include <filesystem>

#include <QTimer>
#include <QPointer>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QDialog>
#include <QMainWindow>
#include <QAbstractButton>
#include <QPushButton>

#include "browser-client.hpp"
#include "browser-scheme.hpp"
#include "browser-app.hpp"
#include "browser-version.h"
#include "GrpcProxy.h"

#include "json11/json11.hpp"
#include "cef-headers.hpp"

#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>

#include <include/base/cef_callback.h>
#include <include/wrapper/cef_closure_task.h>

using namespace std;
using namespace json11;

static CefRefPtr<BrowserApp> app;

static void BrowserInit()
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

	//printf("%s\n", settings.cache_path.str);

	app = new BrowserApp();

	CefExecuteProcess(args, app, nullptr);
	CefInitialize(args, settings, app, nullptr);


	/* Register http://absolute/ scheme handler for older
	 * CEF builds which do not support file:// URLs */
	CefRegisterSchemeHandlerFactory("http", "absolute", new BrowserSchemeHandlerFactory());
}

static void BrowserShutdown(void)
{
	CefClearSchemeHandlerFactories();
	CefShutdown();
	app = nullptr;
}

static void BrowserManagerThread(void)
{
	// Init
	BrowserInit();
	CefRunMessageLoop();
	BrowserShutdown();
}

QWidget* widget = nullptr;
CefRefPtr<CefBrowser> browser = nullptr;

// Define a function.
void MyFunc(int arg)
{
	// Create CEF Browser
	CefWindowInfo window_info;
	CefBrowserSettings browser_settings;
	CefRefPtr<BrowserClient> browserClient = new BrowserClient(true, false);
	CefString url = "https://codepen.io/liammclennan/pen/boMevM";

	// Now set the parent of the CEF browser to the QWidget
	window_info.SetAsChild((HWND)widget->winId(),
			       CefRect(0, 0, widget->width(), widget->height()));

	browser = CefBrowserHost::CreateBrowserSync(
		window_info, browserClient.get(), url, browser_settings,
		CefRefPtr<CefDictionaryValue>(), nullptr);
}

int main(int argc, char *argv[])
{
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
	printf("Debugging Window:\n");

	if (argc < 3)
	{
		printf("Not enough args.\n");
		system("pause");
		return 0;
	}

	int32_t parentListenPort = atoi(argv[1]);
	int32_t myListenPort = atoi(argv[2]);

	if (!GrpcProxy::instance().startServer(myListenPort))
	{
		printf("sl-proxy: failed to start grpc server, GetLastError = %d\n", GetLastError());
		return 0;
	}

	if (!GrpcProxy::instance().connectToClient(parentListenPort)) {
		printf("sl-proxy: failed to connected to plugin's grpc server, GetLastError = %d\n", GetLastError());
		return 0;
	}

	printf("parentListenPort = %d\n", parentListenPort);
	printf("myListenPort = %d\n", myListenPort);

	system("pause");

	GrpcProxy::instance().getClient()->do_grpc_example_Request(1234567);

	system("pause");

	// Create Qt Application
	// requires
	// - qt dll's
	// - platforms folder
	QApplication a(argc, argv);

	// Create CEF Browser
	auto manager_thread = thread(BrowserManagerThread);

	Sleep(5000);

	// Create Qt Widget
	widget = new QWidget{};
	widget->resize(800, 600);
	widget->show();
	
	CefPostTask(TID_UI, base::BindOnce(&MyFunc, 5));

	// Run Qt Application
	int result = a.exec();

	return true;
}

/*

*/

/*

// On Shutdown
if (manager_thread.joinable()) {
		while (!QueueCEFTask([]() { CefQuitMessageLoop(); }))
			os_sleep_ms(5);

		manager_thread.join();
	}

	os_event_destroy(cef_started_event);

*/

/*
struct obs_source_info info = {};
	info.id = "browser_source";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO |
			    OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_INTERACTION |
			    OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_SRGB;
	info.get_properties = browser_source_get_properties;
	info.get_defaults = browser_source_get_defaults;
	info.icon_type = OBS_ICON_TYPE_BROWSER;

	info.get_name = [](void *) { return obs_module_text("BrowserSource"); };
	info.create = [](obs_data_t *settings, obs_source_t *source) -> void * {
		obs_browser_initialize();
		return new BrowserSource(settings, source);
	};
	info.destroy = [](void *data) {
		static_cast<BrowserSource *>(data)->Destroy();
	};
	info.missing_files = browser_source_missingfiles;
	info.update = [](void *data, obs_data_t *settings) {
		static_cast<BrowserSource *>(data)->Update(settings);
	};
	info.get_width = [](void *data) {
		return (uint32_t) static_cast<BrowserSource *>(data)->width;
	};
	info.get_height = [](void *data) {
		return (uint32_t) static_cast<BrowserSource *>(data)->height;
	};
	info.video_tick = [](void *data, float) {
		static_cast<BrowserSource *>(data)->Tick();
	};
	info.video_render = [](void *data, gs_effect_t *) {
		static_cast<BrowserSource *>(data)->Render();
	};
#if CHROME_VERSION_BUILD < 4103
	info.audio_mix = [](void *data, uint64_t *ts_out,
			    struct audio_output_data *audio_output,
			    size_t channels, size_t sample_rate) {
		return static_cast<BrowserSource *>(data)->AudioMix(
			ts_out, audio_output, channels, sample_rate);
	};
	info.enum_active_sources = [](void *data, obs_source_enum_proc_t cb,
				      void *param) {
		static_cast<BrowserSource *>(data)->EnumAudioStreams(cb, param);
	};
#endif
	info.mouse_click = [](void *data, const struct obs_mouse_event *event,
			      int32_t type, bool mouse_up,
			      uint32_t click_count) {
		static_cast<BrowserSource *>(data)->SendMouseClick(
			event, type, mouse_up, click_count);
	};
	info.mouse_move = [](void *data, const struct obs_mouse_event *event,
			     bool mouse_leave) {
		static_cast<BrowserSource *>(data)->SendMouseMove(event,
								  mouse_leave);
	};
	info.mouse_wheel = [](void *data, const struct obs_mouse_event *event,
			      int x_delta, int y_delta) {
		static_cast<BrowserSource *>(data)->SendMouseWheel(
			event, x_delta, y_delta);
	};
	info.focus = [](void *data, bool focus) {
		static_cast<BrowserSource *>(data)->SendFocus(focus);
	};
	info.key_click = [](void *data, const struct obs_key_event *event,
			    bool key_up) {
		static_cast<BrowserSource *>(data)->SendKeyClick(event, key_up);
	};
	info.show = [](void *data) {
		static_cast<BrowserSource *>(data)->SetShowing(true);
	};
	info.hide = [](void *data) {
		static_cast<BrowserSource *>(data)->SetShowing(false);
	};
	info.activate = [](void *data) {
		BrowserSource *bs = static_cast<BrowserSource *>(data);
		if (bs->restart)
			bs->Refresh();
		bs->SetActive(true);
	};
	info.deactivate = [](void *data) {
		static_cast<BrowserSource *>(data)->SetActive(false);
	};

	obs_register_source(&info);

*/

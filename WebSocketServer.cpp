#include "WebSocketServer.h"
#include <iostream>
#include <libwebsockets.h>

static int callback(lws *wsi, lws_callback_reasons reason, void *user, void *in, size_t len)
{
	printf("callback %d\n", reason);

	switch (reason) {
	case LWS_CALLBACK_ESTABLISHED:
		printf("LWS_CALLBACK_ESTABLISHED\n");
		break;
	case LWS_CALLBACK_RECEIVE: {
		printf("LWS_CALLBACK_RECEIVE\n");
		lws_write(wsi, (unsigned char *)in, len, LWS_WRITE_TEXT);
		break;
	}
	default:
		break;
	}

	return 0;
}

void WebSocketServer::start()
{
	if (m_started)
		return;

	m_started = true;
	m_workerThread = std::thread(&WebSocketServer::workerThread, this);
}

void WebSocketServer::stop()
{
	m_started = false;
}

void WebSocketServer::workerThread()
{
	// Protocol listing for the server
	lws_protocols protocols[] = {{
					     "StreamLabsPluginWS",
					     callback,
					     0,
					     0,
				     },
				     {NULL, NULL, 0, 0}};

	// Server configuration
	lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	info.port = 24231; 
	info.protocols = protocols;

	// Create the server context
	lws_context *context = lws_create_context(&info);
	if (!context) {
		printf("lws_create_context failed\n");
		return;
	}

	printf("WebSocketServer Working\n");

	// Main server loop
	while (m_started) {
		lws_service(context, 1000);
	}

	lws_context_destroy(context);
}

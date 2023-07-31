#pragma once

#include "sl_browser_api.grpc.pb.h"

#include <filesystem>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

class grpc_plugin_objImpl;

class GrpcPlugin
{
public:
	static GrpcPluginServer& instance()
	{
		static GrpcPluginServer a;
		return a;
	}

	bool start(int32_t m_listenPort);
	void stop();

private:
	~GrpcPlugin();

	int32_t m_listenPort{0};

	std::wstring m_modulePath;
	std::unique_ptr<grpc::Server> m_server;
	std::unique_ptr<grpc::ServerBuilder> m_builder;
	std::unique_ptr<grpc_plugin_objImpl> m_service;
};

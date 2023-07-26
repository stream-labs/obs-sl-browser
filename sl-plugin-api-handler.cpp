#include "sl-plugin-api-handler.h"

/*static*/
PluginApiHandler::HandleJavascriptCall(const std::string &functionName,
				       const std::string &jsonStr)
{
	printf("function %s executed with params %s\n", functionName.c_str(),
	       jsonStr.c_str());


}

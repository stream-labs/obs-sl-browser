#pragma once

#include <string>
#include <map>

class JavascriptApi
{
public:
	enum JSFuncs {
		JS_INVALID = 0,
		JS_PANEL_EXECUTEJAVASCRIPT = 1,
		JS_PANEL_SETURL = 2,
	};
public:
	static std::map<std::string, JSFuncs> &getGlobalFunctionNames()
	{
		static std::map<std::string, JSFuncs> names = {
			{ "panel_executeJavascript", JS_PANEL_EXECUTEJAVASCRIPT },
			{ "panel_setURL", JS_PANEL_SETURL }
		};

		return names;
	}

	static bool isValidFunctionName(const std::string& str)
	{
		auto ref = getGlobalFunctionNames();
		return ref.find(str) != ref.end();
	}

	static JSFuncs getFunctionId(const std::string& funcName)
	{
		auto ref = getGlobalFunctionNames();
		auto itr = ref.find(funcName);

		if (itr != ref.end())
			return itr->second;

		return JS_INVALID;
	}
};

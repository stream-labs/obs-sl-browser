#pragma once

#include <string>
#include <map>

class JavascriptApi
{
public:
	enum JSFuncs {
		JS_INVALID = 0,
		JS_PANEL_EXECUTEJAVASCRIPT ,
		JS_PANEL_SETURL,
		JS_QUERY_PANEL_UIDS,
	};
public:
	static std::map<std::string, JSFuncs> &getGlobalFunctionNames()
	{
		// arg1 for any function can be a function that will notify when its done, if arg1 is not a function then it blocks
		static std::map<std::string, JSFuncs> names =
		{
			// .(@function(arg1))
			//	Example arg1 = {"result": ["1f67699e49444d3c9914b9c037aac3c7"]}
			{"query_panel_uids", JS_QUERY_PANEL_UIDS},

			// .(@paneluid, @url)
			{"panel_setURL", JS_PANEL_SETURL},

			// .(@paneluid, @jsString)
			{"panel_executeJavascript", JS_PANEL_EXECUTEJAVASCRIPT},
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

#pragma

#include <string>
#include <set>

class JavascriptApi
{
public:
	static std::set<std::string>& getGlobalFunctionNames()
	{
		static std::set<std::string> names = {
			"helloWorld",
		};

		return names;
	}

	static bool isValidFunctionName(const std::string& str)
	{
		auto ref = getGlobalFunctionNames();
		return ref.find(str) != ref.end();
	}
};

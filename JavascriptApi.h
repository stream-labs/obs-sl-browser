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
		JS_QUERY_PANELS,
		JS_DOWNLOAD_ZIP,
		JS_READ_FILE,
		JS_DELETE_FILES,
		JS_DROP_FOLDER,
		JS_QUERY_DOWNLOADS_FOLDER,
		JS_OBS_SOURCE_CREATE,
		JS_OBS_SOURCE_DESTROY,
	};
public:
	static std::map<std::string, JSFuncs> &getGlobalFunctionNames()
	{
		// arg1 for any function can be a function that will notify when its done, if arg1 is not a function then it blocks
		// Json strings that are sent back to you will either contain the expected data or {"error": "message"}
		static std::map<std::string, JSFuncs> names =
		{
			/***
			* Panels
			*/

			// .(@function(arg1))
			//	Example arg1 = [{ "path": "..." }, { "path": "..." }]
			{"query_panels", JS_QUERY_PANELS},

			// .(@paneluid, @url)
			{"panel_setURL", JS_PANEL_SETURL},

			// .(@paneluid, @jsString)
			{"panel_executeJavascript", JS_PANEL_EXECUTEJAVASCRIPT},

			/***
			* Filesystem
			*/

			// .(@function(arg1), @url)
			// Downloads and unpacks the zip, returning a list of full file paths to the files that were in it
			//	Example arg1 = [{ "path": "..." },]
			{"downloadZip", JS_DOWNLOAD_ZIP},

			// .(@function(arg1), @filepath)
			// Returns the contents of a file as a string. If the filesize is over 1mb this will return an error
			//	Example arg1 = { "contents": "..." }
			{"readFile", JS_READ_FILE},

			// .(@filepaths_jsonStr)
			// Json string, array, [{ path: "..." },] paths must be relative to the streamlabs download folder, ie "/download1234/file.png"
			{"deleteFiles", JS_DELETE_FILES},

			// .(@path)
			// Path must be relative to the streamlabs download folder, ie "/download1234/"
			{"dropFolder", JS_DROP_FOLDER},

			// .(@function(arg1))
			// Returns comprehensive list of everything in our downloads folder
			//	Example arg1 = [{ "path": "..." },]
			{"queryDownloadsFolder", JS_QUERY_DOWNLOADS_FOLDER},

			/***
			* obs_source
			*/

			// .(@function(arg1), id, name, settings_jsonStr, hotkey_data_jsonStr)
			// Creates an obs source, also returns back some information about the source you just created if you want it
			// Note that 'name' is also the guid of it, duplicates can't exist
			//	Example arg1 = { "settings_jsonStr": "obs_data_get_full_json()", "audio_mixers": "obs_source_get_audio_mixers()", "deinterlace_mode": "obs_source_get_deinterlace_mode()", "deinterlace_field_order": "obs_source_get_deinterlace_field_order()" }
			{"obs_source_create", JS_OBS_SOURCE_CREATE},

			// .(@function(arg1), name)
			// Destroys an obs source with the name provided if it exists
			{"obs_source_destroy", JS_OBS_SOURCE_DESTROY},

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

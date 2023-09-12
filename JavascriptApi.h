#pragma once

#include <string>
#include <map>

class JavascriptApi
{
public:
	enum JSFuncs {
		JS_INVALID = 0,
		JS_DOCK_EXECUTEJAVASCRIPT ,
		JS_DOCK_SETURL,
		JS_QUERY_DOCKS,
		JS_DOWNLOAD_ZIP,
		JS_READ_FILE,
		JS_DELETE_FILES,
		JS_DROP_FOLDER,
		JS_QUERY_DOWNLOADS_FOLDER,
		JS_OBS_SOURCE_CREATE,
		JS_OBS_SOURCE_DESTROY,
		JS_DOCK_SETAREA,
		JS_DOCK_SWAP,
		JS_DOCK_NEW_BROWSER_DOCK,
	};
public:
	static std::map<std::string, JSFuncs> &getGlobalFunctionNames()
	{
		// arg1 for any function can be a function that will notify when its done, if arg1 is not a function then it blocks
		// Json strings that are sent back to you will either contain the expected data or {"error": "message"}
		static std::map<std::string, JSFuncs> names =
		{
			/***
			* Docks
			*/

			// .(@function(arg1))
			// Docks with sl_uuid's are browser docks
			//	Example arg1 = [{ "name": ".", "x": ".", "y": ".", "floating": bool, "sl_uuid": ".", "url": "." },]
			{"dock_queryAll", JS_QUERY_DOCKS},

			// .(@dockuid, @url)
			{"dock_setURL", JS_DOCK_SETURL},

			// .(@dockuid, @jsString)
			{"dock_executeJavascript", JS_DOCK_EXECUTEJAVASCRIPT},

			// .(@dockName, @areaMask)
			//	areaMask can be a combination of Left Right Top Bottom, ie (LeftDockWidgetArea | RightDockWidgetArea) or (TopDockWidgetArea | BottomDockWidgetArea)
			//	These are the current values from Qt
			//		LeftDockWidgetArea = 0x1,
			//		RightDockWidgetArea = 0x2,
			//		TopDockWidgetArea = 0x4,
			//		BottomDockWidgetArea = 0x8,
			{"dock_setArea", JS_DOCK_SETAREA},

			// .(@dockName1, @dockName2)
			//	Swaps the the positions of dock1 with dock2 
			{"dock_swap", JS_DOCK_SWAP},
			
			// .(@title, @url, @sl_uuid)
			//	Swaps the the positions of dock1 with dock2
			{"dock_newBrowserDock", JS_DOCK_NEW_BROWSER_DOCK},

			/***
			* Filesystem
			*/

			// .(@function(arg1), @url)
			// Downloads and unpacks the zip, returning a list of full file paths to the files that were in it
			//	Example arg1 = [{ "path": "..." },]
			{"fs_downloadZip", JS_DOWNLOAD_ZIP},

			// .(@function(arg1), @filepath)
			// Returns the contents of a file as a string. If the filesize is over 1mb this will return an error
			//	Example arg1 = { "contents": "..." }
			{"fs_readFile", JS_READ_FILE},

			// .(@filepaths_jsonStr)
			// Json string, array, [{ path: "..." },] paths must be relative to the streamlabs download folder, ie "/download1234/file.png"
			{"fs_deleteFiles", JS_DELETE_FILES},

			// .(@path)
			// Path must be relative to the streamlabs download folder, ie "/download1234/"
			{"fs_dropFolder", JS_DROP_FOLDER},

			// .(@function(arg1))
			// Returns comprehensive list of everything in our downloads folder
			//	Example arg1 = [{ "path": "..." },]
			{"fs_queryDownloadsFolder", JS_QUERY_DOWNLOADS_FOLDER},

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

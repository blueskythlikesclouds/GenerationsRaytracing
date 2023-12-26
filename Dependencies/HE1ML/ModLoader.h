#pragma once

#define ML_API __cdecl
struct MLUpdateInfo
{
	void* device;
};

#define ML_STRINGIFY(X) #X
#define ML_XSTRINGIFY(X) ML_STRINGIFY(X)

#define ML_API_EXPORT_NAME GetModLoaderAPI

#define ML_ENVAR_NAME "MODLOADER_NAME"
#define ML_ENVAR_VERSION "MODLOADER_VERSION"
#define ML_ENVAR_HOST_MODULE "MODLOADER_HOST_MODULE"
#define ML_ENVAR_PROCESS_RESTARTED "HE1ML_PROCESS_RESTARTED"
#define ML_ENVAR_PROCESS_HAD_DEBUGGER "HE1ML_PROCESS_HAD_DEBUGGER"

#define ML_API_VERSION 0x101000
#define ML_MSG_ADD_LOG_HANDLER 1
#define ML_MSG_REQ_LARGE_ADDRESS_AWARE 2

#define ML_LOG_LEVEL_INFO 0
#define ML_LOG_LEVEL_WARNING 1
#define ML_LOG_LEVEL_ERROR 2

#define ML_LOG_CATEGORY_GENERAL 0
#define ML_LOG_CATEGORY_CRIWARE 1

#define ML_PROPERTY_TYPE_ID 0
#define ML_PROPERTY_TYPE_TITLE 1
#define ML_PROPERTY_TYPE_HMODULE 2
#define ML_PROPERTY_TYPE_INDEX 3
#define ML_PROPERTY_TYPE_CALLER 4
#define ML_PROPERTY_TYPE_COUNT 5

#define ML_MOD_PRIORITY_MAX 0

typedef void ML_API LogEvent_t(void* obj, int level, int category, const char* message, size_t p1, size_t p2, size_t* parray);
struct ModLoaderAPI_t;
struct CommonLoaderAPI;

struct AddLogHandlerMessage_t
{
	void* obj{};
	LogEvent_t* handler{};
};

#ifdef MODLOADER_IMPLEMENTATION
namespace v0
{
#endif
	struct Mod_t
	{
		const char* Name;
		const char* Path;
		const char* ID;
		size_t Priority;
		void* pImpl;
	};

	// stdc++ compatability
	// We can't use std::vector because it's not guaranteed to be ABI compatible
	struct ModList_t
	{
		const Mod_t** first;
		const Mod_t** last;
		const Mod_t** capacity;

#ifdef MODLOADER_IMPLEMENTATION
		ModList_t() : first(nullptr), last(nullptr), capacity(nullptr) {}
		ModList_t(const Mod_t** start, const Mod_t** fin) : first(start), last(fin), capacity(fin) {}
#endif

#ifdef __cplusplus
		const Mod_t** begin() const { return first; }
		const Mod_t** end() const { return last; }

		const Mod_t*& operator[](size_t i) const { return first[i]; }
		size_t size() const { return last - first; }
#endif
	};

	struct ModInfo_t
	{
		ModList_t* ModList;
		Mod_t* CurrentMod;
		const ModLoaderAPI_t* API;
		void* Reserved[2];
	};

#ifdef MODLOADER_IMPLEMENTATION
}
#endif

#define DECLARE_API_FUNC(RETURN_TYPE, NAME, ...) RETURN_TYPE (ML_API *NAME)(__VA_ARGS__) = nullptr;

#ifdef MODLOADER_IMPLEMENTATION
typedef v0::Mod_t Mod_t;
#endif

struct ModLoaderAPI_t
{
	DECLARE_API_FUNC(unsigned int, GetVersion);
	DECLARE_API_FUNC(const CommonLoaderAPI*, GetCommonLoader);
	DECLARE_API_FUNC(const Mod_t*, FindMod, const char* id);
	DECLARE_API_FUNC(void, SendMessageImm, const Mod_t* mod, size_t id, void* data);
	DECLARE_API_FUNC(void, SendMessageToLoader, size_t id, void* data);
	DECLARE_API_FUNC(size_t, SetPriority, const Mod_t* mod, size_t priority);
	DECLARE_API_FUNC(int, BindFile, const Mod_t* mod, const char* path, const char* destination, int priority);
	DECLARE_API_FUNC(int, BindDirectory, const Mod_t* mod, const char* path, const char* destination, int priority);
	DECLARE_API_FUNC(int, BindFileEx, const char* path, const char* destination, int priority);
	DECLARE_API_FUNC(int, BindDirectoryEx, const char* path, const char* destination, int priority);
	DECLARE_API_FUNC(const Mod_t*, FindModEx, const void* data, int property_type);
	DECLARE_API_FUNC(void, Log, int level, int category, const char* message, size_t p1, size_t p2, size_t* parray);
	DECLARE_API_FUNC(void, SetSaveFile, const char* path);
	DECLARE_API_FUNC(bool, LoadExternalModule, const char* path);
};

#undef DECLARE_API_FUNC

struct FilterModArguments_t
{
	const Mod_t* mod;
	const Mod_t* self;
	bool handled{};
};

#ifdef WIN32

inline HMODULE GetModLoaderHModule()
{
	static HMODULE mod = nullptr;
	if (!mod)
	{
		char path[MAX_PATH];
		path[0] = '\0';

		if (GetEnvironmentVariableA(ML_ENVAR_HOST_MODULE, path, MAX_PATH) != 0)
		{
			mod = GetModuleHandleA(path);
		}
	}
	return mod;
}

#ifdef MODLOADER_IMPLEMENTATION
extern "C" __declspec(dllexport) const ModLoaderAPI_t* ML_API ML_API_EXPORT_NAME();
#else
inline const ModLoaderAPI_t* ML_API_EXPORT_NAME()
{
	static const ModLoaderAPI_t* api = nullptr;
	if (!api)
	{
		HMODULE mod = GetModLoaderHModule();
		if (mod)
		{
			const ModLoaderAPI_t*(*callback)() = (const ModLoaderAPI_t*(*)())GetProcAddress(mod, ML_XSTRINGIFY(ML_API_EXPORT_NAME));
			if (callback)
			{
				api = callback();
			}
		}
	}
	return api;
}
#endif

#endif

#ifdef MODLOADER_IMPLEMENTATION
#define MODLOADER_CONFIG_NAME "he1ml.ini"
#define MODLOADER_LEGACY_CONFIG_NAME "cpkredir.ini"

#define LOG(MSG, ...) { LOG_IMPL(MSG "\n", __VA_ARGS__); }

#include <string>
#include <memory>
#include <vector>
#include "Mod.h"
#include "FileBinder.h"
#include "VirtualFileSystem.h"
#include "Globals.h"

extern ModLoaderAPI_t g_ml_api;

class Mod;
class ModLoader
{
public:
	std::string config_path{};
	std::string database_path{};
	std::string save_file{ "hedgehog.sav" };
	std::filesystem::path root_path{};

	bool save_redirection{ false };
	bool save_read_through{ true };

	bool enable_cri_logs{ false };
	bool large_address_aware_requested{ false };

	std::unique_ptr<FileBinder> binder{ new FileBinder() };
	VirtualFileSystem* vfs{ &binder->vfs };
	std::vector<std::unique_ptr<Mod>> mods{};
	std::vector<std::unique_ptr<v0::Mod_t>> mod_handles{};

	std::vector<ModEvent_t*> update_handlers{};
	MLUpdateInfo update_info{};

	std::vector<std::pair<void*, LogEvent_t*>> log_handlers{};
	Mod sys_mod{ this };
	Mod_t sys_mod_handle{ "ModLoader.System", nullptr, "modloader.system", 0, &sys_mod};

	void BasicInit();
	void Init(const char* configPath);
	void LoadDatabase(const std::string& databasePath, bool append = false);
	bool RegisterMod(const std::string& path);
	void BroadcastMessageImm(size_t id, void* data) const;
	bool LoadExternalModule(const char* path, bool raise_init = false);
	void OnUpdate();
	void ProcessMessage(size_t id, void* data);
	void FilterMods();
	void AddLogger(void* obj, LogEvent_t* event)
	{
		log_handlers.emplace_back(obj, event);
	}

	void WriteLog(int level, int category, const char* message, size_t p1, size_t p2, size_t* parray) const;
	void WriteLog(int level, int category, const char* message, size_t p1, size_t p2) const
	{
		WriteLog(level, category, message, p1, p2, nullptr);
	}

	void WriteLog(int level, int category, const char* message, size_t p1) const
	{
		WriteLog(level, category, message, p1, 0, nullptr);
	}

	void WriteLog(int level, int category, const char* message) const
	{
		WriteLog(level, category, message, 0, 0, nullptr);
	}

	void Log(const char* message) const
	{
		WriteLog(ML_LOG_LEVEL_INFO, ML_LOG_CATEGORY_GENERAL, message);
	}

	void LogError(const char* message) const
	{
		WriteLog(ML_LOG_LEVEL_ERROR, ML_LOG_CATEGORY_GENERAL, message);
	}

	void SetUseSaveRedirection(bool value)
	{
		save_redirection = value;
	}

	void SetSaveFile(const char* path);
};

template<typename... T>
inline void LOG_IMPL(const char* msg, T... args)
{
	if (!g_loader)
	{
		return;
	}

	g_loader->WriteLog(ML_LOG_LEVEL_INFO, ML_LOG_CATEGORY_GENERAL, msg, reinterpret_cast<size_t>(args)...);
}

#endif
/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "base/plugins.h"

#include "common/func.h"
#include "common/debug.h"

#ifdef DYNAMIC_MODULES
#include "common/config-manager.h"
#include "common/fs.h"
#endif

// Plugin versioning

int pluginTypeVersions[PLUGIN_TYPE_MAX] = {
	PLUGIN_TYPE_ENGINE_VERSION,
	PLUGIN_TYPE_MUSIC_VERSION,
};


// Abstract plugins

PluginType Plugin::getType() const {
	return _type;
}

const char *Plugin::getName() const {
	return _pluginObject->getName();
}

class StaticPlugin : public Plugin {
public:
	StaticPlugin(PluginObject *pluginobject, PluginType type) {
		assert(pluginobject);
		assert(type < PLUGIN_TYPE_MAX);
		_pluginObject = pluginobject;
		_type = type;
	}

	~StaticPlugin() {
		delete _pluginObject;
	}

	virtual bool loadPlugin()		{ return true; }
	virtual void unloadPlugin()		{}
};

class StaticPluginProvider : public PluginProvider {
public:
	StaticPluginProvider() {
	}

	~StaticPluginProvider() {
	}

	virtual PluginList getPlugins() {
		PluginList pl;

		#define LINK_PLUGIN(ID) \
			extern PluginType g_##ID##_type; \
			extern PluginObject *g_##ID##_getObject(); \
			pl.push_back(new StaticPlugin(g_##ID##_getObject(), g_##ID##_type));

		// "Loader" for the static plugins.
		// Iterate over all registered (static) plugins and load them.

		// Engine plugins
		#if PLUGIN_ENABLED_STATIC(GRIM)
		LINK_PLUGIN(GRIM)
		#endif

		return pl;
	}
};

#ifdef DYNAMIC_MODULES

PluginList FilePluginProvider::getPlugins() {
	PluginList pl;

	// Prepare the list of directories to search
	Common::FSList pluginDirs;

	// Add the default directories
	pluginDirs.push_back(Common::FSNode("."));
	pluginDirs.push_back(Common::FSNode("plugins"));

	// Add the provider's custom directories
	addCustomDirectories(pluginDirs);

	// Add the user specified directory
	Common::String pluginsPath(ConfMan.get("pluginspath"));
	if (!pluginsPath.empty())
		pluginDirs.push_back(Common::FSNode(pluginsPath));

	Common::FSList::const_iterator dir;
	for (dir = pluginDirs.begin(); dir != pluginDirs.end(); ++dir) {
		// Load all plugins.
		// Scan for all plugins in this directory
		Common::FSList files;
		if (!dir->getChildren(files, Common::FSNode::kListFilesOnly)) {
			debug(1, "Couldn't open plugin directory '%s'", dir->getPath().c_str());
			continue;
		} else {
			debug(1, "Reading plugins from plugin directory '%s'", dir->getPath().c_str());
		}

		for (Common::FSList::const_iterator i = files.begin(); i != files.end(); ++i) {
			if (isPluginFilename(*i)) {
				pl.push_back(createPlugin(*i));
			}
		}
	}

	return pl;
}

bool FilePluginProvider::isPluginFilename(const Common::FSNode &node) const {
	Common::String filename = node.getName();

#ifdef PLUGIN_PREFIX
	// Check the plugin prefix
	if (!filename.hasPrefix(PLUGIN_PREFIX))
		return false;
#endif

#ifdef PLUGIN_SUFFIX
	// Check the plugin suffix
	if (!filename.hasSuffix(PLUGIN_SUFFIX))
		return false;
#endif

	return true;
}

void FilePluginProvider::addCustomDirectories(Common::FSList &dirs) const {
#ifdef PLUGIN_DIRECTORY
	dirs.push_back(Common::FSNode(PLUGIN_DIRECTORY));
#endif
}

#endif // DYNAMIC_MODULES

#pragma mark -

DECLARE_SINGLETON(PluginManager);

PluginManager::PluginManager() {
	// Always add the static plugin provider.
	addPluginProvider(new StaticPluginProvider());
}

PluginManager::~PluginManager() {
	// Explicitly unload all loaded plugins
	unloadPlugins();

	// Delete the plugin providers
	for (ProviderList::iterator pp = _providers.begin();
	                            pp != _providers.end();
	                            ++pp) {
		delete *pp;
	}
}

void PluginManager::addPluginProvider(PluginProvider *pp) {
	_providers.push_back(pp);
}

void PluginManager::loadPlugins() {
	for (ProviderList::iterator pp = _providers.begin();
	                            pp != _providers.end();
	                            ++pp) {
		PluginList pl((*pp)->getPlugins());
		Common::for_each(pl.begin(), pl.end(), Common::bind1st(Common::mem_fun(&PluginManager::tryLoadPlugin), this));
	}

}

void PluginManager::unloadPlugins() {
	for (int i = 0; i < PLUGIN_TYPE_MAX; i++)
		unloadPluginsExcept((PluginType)i, NULL);
}

void PluginManager::unloadPluginsExcept(PluginType type, const Plugin *plugin) {
	Plugin *found = NULL;
	for (PluginList::iterator p = _plugins[type].begin(); p != _plugins[type].end(); ++p) {
		if (*p == plugin) {
			found = *p;
		} else {
			(*p)->unloadPlugin();
			delete *p;
		}
	}
	_plugins[type].clear();
	if (found != NULL) {
		_plugins[type].push_back(found);
	}
}

bool PluginManager::tryLoadPlugin(Plugin *plugin) {
	assert(plugin);
	// Try to load the plugin
	if (plugin->loadPlugin()) {
		// The plugin is valid, see if it provides the same module as an
		// already loaded one and should replace it.
		bool found = false;

		PluginList::iterator pl = _plugins[plugin->getType()].begin();
		while (!found && pl != _plugins[plugin->getType()].end()) {
			if (!strcmp(plugin->getName(), (*pl)->getName())) {
				// Found a duplicated module. Replace the old one.
				found = true;
				delete *pl;
				*pl = plugin;
				debug(1, "Replaced the duplicated plugin: '%s'", plugin->getName());
			}
			pl++;
		}

		if (!found) {
			// If it provides a new module, just add it to the list of known plugins.
			_plugins[plugin->getType()].push_back(plugin);
		}

		return true;
	} else {
		// Failed to load the plugin
		delete plugin;
		return false;
	}
}


// Engine plugins

#include "engines/metaengine.h"

DECLARE_SINGLETON(EngineManager);

GameDescriptor EngineManager::findGame(const Common::String &gameName, const EnginePlugin **plugin) const {
	// Find the GameDescriptor for this target
	const EnginePlugin::List &plugins = getPlugins();
	GameDescriptor result;

	if (plugin)
		*plugin = 0;

	EnginePlugin::List::const_iterator iter = plugins.begin();
	for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
		result = (**iter)->findGame(gameName.c_str());
		if (!result.gameid().empty()) {
			if (plugin)
				*plugin = *iter;
			break;
		}
	}
	return result;
}

GameList EngineManager::detectGames(const Common::FSList &fslist) const {
	GameList candidates;

	const EnginePlugin::List &plugins = getPlugins();

	// Iterate over all known games and for each check if it might be
	// the game in the presented directory.
	EnginePlugin::List::const_iterator iter;
	for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
		candidates.push_back((**iter)->detectGames(fslist));
	}

	return candidates;
}

const EnginePlugin::List &EngineManager::getPlugins() const {
	return (const EnginePlugin::List &)PluginManager::instance().getPlugins(PLUGIN_TYPE_ENGINE);
}


// Music plugins

#include "sound/musicplugin.h"

DECLARE_SINGLETON(MusicManager);

const MusicPlugin::List &MusicManager::getPlugins() const {
	return (const MusicPlugin::List &)PluginManager::instance().getPlugins(PLUGIN_TYPE_MUSIC);
}

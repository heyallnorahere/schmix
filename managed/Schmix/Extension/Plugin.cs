namespace Schmix.Extension;

using Coral.Managed;
using Coral.Managed.Interop;

using Schmix.Core;

using System;
using System.Collections.Generic;
using System.Reflection;

public abstract class Plugin : IDisposable
{
    private struct PluginInfo
    {
        public Plugin Instance;

        public string Name;
        public Assembly SourceAssembly;
        public Type PluginType;
    }

    private static readonly Dictionary<string, PluginInfo> sPlugins;
    static Plugin()
    {
        sPlugins = new Dictionary<string, PluginInfo>();
    }

    public static IReadOnlyDictionary<string, Plugin> Plugins
    {
        get
        {
            var plugins = new Dictionary<string, Plugin>();

            foreach ((var name, var info) in sPlugins)
            {
                var plugin = info.Instance;
                plugins.Add(name, plugin);
            }
            
            return plugins;
        }
    }

    internal static int LoadPluginsFromAssembly_Native(int loadContext, int assemblyID)
    {
        // see managed/Coral.Managed.csproj
        Assembly? assembly;
        if (!AssemblyLoader.TryGetAssembly(loadContext, assemblyID, out assembly) || assembly is null)
        {
            Log.Error("Failed to retrieve assembly from Coral!");
            return -1;
        }

        try
        {
            return LoadPluginsFromAssembly(assembly);
        }
        catch (Exception ex)
        {
            Log.Error($"Failed to load plugins for assembly: {ex}");
            return -1;
        }
    }

    internal static int LoadPluginsFromAssembly(Assembly assembly)
    {
        int pluginCount = 0;

        var types = assembly.GetTypes();
        foreach (var type in types)
        {
            if (!type.IsClass)
            {
                continue;
            }

            var attribute = type.GetCustomAttribute<RegisteredPluginAttribute>();
            if (attribute is null)
            {
                Log.Debug($"Skipping type: {type.Namespace ?? "<global>"}.{type.Name}");
                continue;
            }

            var pluginName = attribute.Name;
            if (sPlugins.ContainsKey(pluginName))
            {
                Log.Error($"Plugin already registered: {pluginName}");
                continue;
            }

            if (!type.IsDerivedFrom(typeof(Plugin)))
            {
                Log.Error($"Plugin \"{pluginName}\" is not derived from Plugin!");
                continue;
            }

            var result = (Plugin?)Activator.CreateInstance(type, attribute.Parameters);
            if (result is null)
            {
                Log.Error($"Failed to initialize plugin \"{pluginName}\"");
                continue;
            }

            sPlugins.Add(pluginName, new PluginInfo
            {
                Instance = result,

                Name = pluginName,
                SourceAssembly = assembly,
                PluginType = type
            });

            Log.Info($"Loaded plugin: {pluginName}");
            pluginCount++;
        }

        return pluginCount;
    }

    internal static void UnloadPlugins()
    {
        foreach (var name in sPlugins.Keys)
        {
            var info = sPlugins[name];
            var plugin = info.Instance;

            plugin.Dispose();
        }

        sPlugins.Clear();
    }

    protected Plugin()
    {
        mDisposed = false;
    }

    ~Plugin()
    {
        if (mDisposed)
        {
            return;
        }

        Unload(false);
        mDisposed = true;
    }

    public abstract Module Instantiate();

    protected virtual void Unload(bool disposing)
    {
        // unload plugin-specific data here
    }

    public void Dispose()
    {
        if (mDisposed)
        {
            return;
        }

        Unload(true);
        GC.SuppressFinalize(this);

        mDisposed = true;
    }

    private bool mDisposed;
}

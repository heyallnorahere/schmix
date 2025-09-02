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

    internal static NativeString LoadPluginsFromAssembly_Native(int assemblyID)
    {
        // see managed/Coral.Managed.csproj
        Assembly? assembly;
        if (!AssemblyLoader.TryGetAssembly(assemblyID, out assembly) || assembly is null)
        {
            return "Failed to retrieve assembly from Coral!";
        }

        try
        {
            LoadPluginsFromAssembly(assembly);
            return NativeString.Null();
        }
        catch (Exception ex)
        {
            return ex.ToString();
        }
    }

    internal static void LoadPluginsFromAssembly(Assembly assembly)
    {
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
                continue;
            }

            var pluginName = attribute.Name;
            if (sPlugins.ContainsKey(pluginName))
            {
                // todo: issue error message
                continue;
            }

            if (!type.IsDerivedFrom(typeof(Plugin)))
            {
                // todo: issue warning message
                continue;
            }

            var result = (Plugin?)Activator.CreateInstance(type, attribute.Parameters);
            if (result is null)
            {
                // todo: issue error message
                continue;
            }

            sPlugins.Add(pluginName, new PluginInfo
            {
                Instance = result,

                Name = pluginName,
                SourceAssembly = assembly,
                PluginType = type
            });
        }
    }

    public static Plugin? GetByName(string name)
    {
        PluginInfo info;
        if (sPlugins.TryGetValue(name, out info))
        {
            return info.Instance;
        }

        return null;
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

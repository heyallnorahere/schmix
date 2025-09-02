namespace Schmix.Extension;

using Coral.Managed;
using Coral.Managed.Interop;

using Schmix.Core;

using System;
using System.Collections.Generic;
using System.Reflection;

public abstract class Plugin
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

    internal static Bool32 LoadPluginsFromAssembly_Native(int assemblyID)
    {
        // see managed/Coral.Managed.csproj
        Assembly? assembly;
        if (!AssemblyLoader.TryGetAssembly(assemblyID, out assembly) || assembly is null)
        {
            return false;
        }

        LoadPluginsFromAssembly(assembly);
        return true;
    }

    internal static Plugin? GetByName_Native(NativeString name)
    {
        var value = name.ToString();
        if (value is null)
        {
            return null;
        }

        return GetByName(value);
    }

    internal static void UnloadPlugins()
    {
        foreach (var name in sPlugins.Keys)
        {
            var info = sPlugins[name];
            var plugin = info.Instance;

            plugin.Unload();
        }

        sPlugins.Clear();
    }

    public abstract Module Instantiate();

    protected virtual void Unload()
    {
        // unload plugin-specific data here
    }
}

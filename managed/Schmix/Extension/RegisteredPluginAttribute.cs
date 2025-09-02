namespace Schmix.Extension;

using System;

[AttributeUsage(AttributeTargets.Class)]
public sealed class RegisteredPluginAttribute : Attribute
{
    public RegisteredPluginAttribute(string name, params object?[]? parameters)
    {
        Name = name;
        Parameters = parameters;
    }

    internal string Name { get; }
    internal object?[]? Parameters { get; }
}

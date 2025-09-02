namespace Schmix.Core;

using System;

public static class Extensions
{
    public static bool IsDerivedFrom(this Type derived, Type baseType)
    {
        if (derived == baseType)
        {
            return true;
        }

        var currentBase = derived.BaseType;
        if (currentBase is null)
        {
            return false;
        }

        return IsDerivedFrom(currentBase, baseType);
    }
}

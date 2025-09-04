namespace Schmix.UI;

using ImGuiNET;
using imnodesNET;

using System;
using System.Collections.Generic;

public abstract class Node : IDisposable
{
    private static int sCurrentID = 0;

    public static int MintID() => ++sCurrentID;

    protected Node()
    {
        mDisposed = false;

        mID = mStaticID = -1;
        mInputIDs = new Dictionary<int, int>();
        mOutputIDs = new Dictionary<int, int>();
    }

    ~Node()
    {
        if (mDisposed)
        {
            return;
        }

        Cleanup(false);
        mDisposed = true;
    }

    public abstract string Name { get; }

    public virtual IReadOnlyList<string> Inputs => Array.Empty<string>();
    public virtual IReadOnlyList<string> Outputs => Array.Empty<string>();

    public abstract void Update(IReadOnlyList<Cable?> inputs, IReadOnlyList<Cable?> outputs);

    public void Render()
    {
        if (mStaticID < 0)
        {
            mStaticID = MintID();
        }

        imnodes.BeginStaticAttribute(mStaticID);
        RenderContent();
        imnodes.EndStaticAttribute();
    }

    protected abstract void RenderContent();

    public virtual void OnInputAttached(int index, Cable.Endpoint source)
    {
    }

    public virtual void OnOutputAttached(int index, Cable.Endpoint destination)
    {
    }

    protected virtual void Cleanup(bool disposing)
    {
    }

    public int ID
    {
        get
        {
            if (mID < 0)
            {
                mID = MintID();
            }

            return mID;
        }
    }

    public int GetInputID(int index)
    {
        int inputCount = Inputs.Count;
        if (index >= inputCount)
        {
            return -1;
        }

        if (!mInputIDs.ContainsKey(index))
        {
            mInputIDs.Add(index, MintID());
        }

        return mInputIDs[index];
    }

    public int GetOutputID(int index)
    {
        int outputCount = Outputs.Count;
        if (index >= outputCount)
        {
            return -1;
        }

        if (!mOutputIDs.ContainsKey(index))
        {
            mOutputIDs.Add(index, MintID());
        }

        return mOutputIDs[index];
    }

    public void Dispose()
    {
        if (mDisposed)
        {
            return;
        }

        Cleanup(true);
        GC.SuppressFinalize(this);

        mDisposed = true;
    }

    private bool mDisposed;

    private int mID, mStaticID;
    private readonly Dictionary<int, int> mInputIDs, mOutputIDs;
}

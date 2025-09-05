namespace Schmix.UI;

using Schmix.Audio;

public sealed class Cable : ISignalInput, ISignalOutput
{
    public readonly struct Endpoint
    {
        public Endpoint(Node node, int index)
        {
            Instance = node;
            Index = index;
        }

        public readonly Node Instance;
        public readonly int Index;
    }

    public Cable(Endpoint source, Endpoint destination)
    {
        mID = -1;

        mSource = source;
        mDestination = destination;

        ResetSignal();
    }

    public int ID
    {
        get
        {
            if (mID < 0)
            {
                mID = Node.MintID();
            }

            return mID;
        }
    }

    public Endpoint Source => mSource;
    public Endpoint Destination => mDestination;

    public StereoSignal<double>? Signal => mSignal;

    public void ResetSignal()
    {
        mSignal = null;
    }
    
    public void PutSignal(StereoSignal<double> signal)
    {
        if (mSignal is null)
        {
            mSignal = signal.Copy();
        }
        else
        {
            mSignal += signal;
        }
    }

    private int mID;
    private readonly Endpoint mSource, mDestination;

    private StereoSignal<double>? mSignal;
}

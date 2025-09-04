namespace Schmix.UI;

using Schmix.Extension;

using System.Collections.Generic;

internal sealed class ModuleNode : Node
{
    public ModuleNode(Module module)
    {
        mModule = module;
    }

    public Module Instance => mModule;

    public override string Name => mModule.Name;

    public override IReadOnlyList<string> Inputs
    {
        get
        {
            int inputCount = mModule.InputCount;
            var names = new string[inputCount];

            for (int i = 0; i < inputCount; i++)
            {
                names[i] = mModule.GetInputName(i);
            }

            return names;
        }
    }

    public override IReadOnlyList<string> Outputs
    {
        get
        {
            int outputCount = mModule.OutputCount;
            var names = new string[outputCount];

            for (int i = 0; i < outputCount; i++)
            {
                names[i] = mModule.GetOutputName(i);
            }

            return names;
        }
    }

    protected override void RenderContent() => mModule.DrawProperties();

    public override void Update(IReadOnlyList<Cable?> inputs, IReadOnlyList<Cable?> outputs)
    {
        int samplesRequested = Rack.SamplesRequested;
        if (samplesRequested <= 0)
        {
            return;
        }

        int channels = Rack.Channels;
        int sampleRate = Rack.SampleRate;

        mModule.Process(inputs, outputs, sampleRate, samplesRequested, channels);
    }

    protected override void Cleanup(bool disposing)
    {
        if (disposing)
        {
            mModule.Dispose();
        }
    }

    private Module mModule;
}

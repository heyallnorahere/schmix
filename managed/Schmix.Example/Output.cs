using Schmix.Audio;
using Schmix.Core;
using Schmix.Extension;
using Schmix.UI;

using System.Collections.Generic;

namespace Schmix.Example
{
    internal sealed class OutputModule : Module
    {
        public OutputModule()
        {
            uint id = WindowAudioOutput.DefaultDeviceID;
            mOutput = new WindowAudioOutput(id, Rack.SampleRate, Rack.Channels);
        }

        protected override void Cleanup(bool disposed)
        {
            mOutput.Dispose();
        }

        public override int SamplesRequested
        {
            get
            {
                if (mOutput is null)
                {
                    return 0;
                }

                int queued = mOutput.QueuedSamples;
                int chunkSize = mOutput.SampleRate / 4;

                if (queued < chunkSize)
                {
                    return chunkSize;
                }

                return 0;
            }
        }

        public override int InputCount => 1;

        public override string GetInputName(int index) => index > 0 ? "<unused>" : "Audio";

        public override void Process(IReadOnlyList<IAudioInput?> inputs, IReadOnlyList<IAudioOutput?> outputs, int sampleRate, int samplesRequested, int channels)
        {
            var audioInput = inputs[0];
            var audio = audioInput?.Signal;

            if (audio is not null)
            {
                mOutput.ResetSignal();
                mOutput.PutAudio(audio);
                mOutput.Flush();
            }
        }

        private WindowAudioOutput mOutput;
    }

    [RegisteredPlugin("Output")]
    public sealed class OutputPlugin : Plugin
    {
        public override Module Instantiate() => new OutputModule();
    }
}

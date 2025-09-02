using Schmix.Audio;
using Schmix.Extension;

namespace Schmix.Example
{
    internal sealed class SineModule : Module
    {
        public SineModule()
        {
            mFrequency = 440;
            mSample = 0;
        }

        public override IReadOnlyList<InterfaceSpec> Inputs => new InterfaceSpec[]
        {
            // InterfaceSpec.Note,
        };

        public override IReadOnlyList<InterfaceSpec> Outputs => new InterfaceSpec[]
        {
            InterfaceSpec.Signal
        };

        public override void Process(IReadOnlyList<object?> inputs, IReadOnlyList<object?> outputs, int sampleRate, int samplesRequested, int channels)
        {
            // this function is the main math of your module. in this example, it generates a sine
            // wave from the provided note input, which it passes into the first element of
            // "outputs"

            // we cant do anything if we have no means to communicate with the rack
            if (/* inputs.Count <= 0 ||*/ outputs.Count <= 0)
            {
                return;
            }

            // todo: take notes from input
            var notes = new Note[]
            {
                new Note
                {
                    Frequency = mFrequency,
                    Gain = 1,
                    GainControlled = false,

                    Active = true,
                    TimeActive = (double)mSample / (double)sampleRate,
                    TimeInactive = 0,
                },
            };

            if (outputs[0] is not IAudioOutput output)
            {
                return;
            }

            // the user can press multiple notes at the same time
            foreach (var note in notes)
            {
                if (!note.Active && !note.GainControlled)
                {
                    continue;
                }

                double currentTime = note.TimeActive;
                if (!note.Active)
                {
                    currentTime += note.TimeInactive;
                }

                // the output is in stereo. the number of channels are provided to the module
                // the number of samples requested describes the length of signal the output expects
                var signal = new StereoSignal<double>(channels, samplesRequested);

                // 1/2pi is the frequency of sin in units rad^-1
                // "note.Frequency" is our frequency in units Hz (s^-1)
                // this coefficient converts to our function domain (seconds) to the domain of sin (radians) and scales the input
                double timeToPhase = 2 * Math.PI * note.Frequency;

                for (int i = 0; i < samplesRequested; i++)
                {
                    // "sampleTime" is the time since the start of the note at this sample, in seconds
                    double sampleTime = currentTime + ((double)i / (double)sampleRate);

                    // convert time since start of sample to sine wave phase
                    double phase = timeToPhase * sampleTime;

                    // we already have the amplitude of the wave
                    double amplitude = note.Gain;

                    // this sample is what we pass to the audio output
                    double sample = Math.Sin(phase) * amplitude;

                    for (int j = 0; j < channels; j++)
                    {
                        signal[j][i] = sample;
                    }
                }

                // add this note's frequency to the output
                // this does not queue it on after
                // rather, it adds it to the existing signal for this chunk
                output.PutAudio(signal);
            }

            mSample += samplesRequested;
        }

        private double mFrequency;
        private int mSample;
    }

    [RegisteredPlugin("Sine generator")]
    public sealed class SinePlugin : Plugin
    {
        // this function is called to instantiate one "module" onto the rack
        public override Module Instantiate() => new SineModule();
    }
}

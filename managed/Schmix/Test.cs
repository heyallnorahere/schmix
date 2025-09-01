namespace Schmix;

using System;

public static class Test
{
    internal static unsafe void AddSineSignal_Native(double frequency, void* mixerAddress, uint channel)
    {
        using var mixer = new Mixer(mixerAddress);
        AddSineSignal(frequency, mixer, channel);
    }

    private static int sSample = 0;

    public static void AddSineSignal(double frequency, Mixer mixer, uint channel)
    {
        int chunkSize = mixer.ChunkSize;
        var signal = CreateSine(sSample, frequency, chunkSize, mixer.SampleRate, mixer.AudioChannels);
        sSample += chunkSize;

        mixer.AddSignalToChannel(channel, signal);
    }

    public static StereoSignal<double> CreateSine(int sampleOffset, double frequency, int length, int sampleRate, int channels)
    {
        var signal = new StereoSignal<double>(channels, length);

        for (int i = 0; i < signal.Length; i++)
        {
            int currentSample = sampleOffset + i;
            for (int j = 0; j < signal.Channels; j++)
            {
                double phaseCoefficient = 2 * Math.PI * frequency;
                double sample = Math.Sin(phaseCoefficient * currentSample / sampleRate);

                signal[j][i] = sample;
            }
        }

        return signal;
    }
}

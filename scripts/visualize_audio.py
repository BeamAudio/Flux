import wave
import numpy as np
import matplotlib.pyplot as plt

def plot_audio(filename):
    with wave.open(filename, 'rb') as wav:
        params = wav.getparams()
        n_channels, sampwidth, framerate, n_frames = params[:4]
        str_data = wav.readframes(n_frames)
        wave_data = np.frombuffer(str_data, dtype=np.int16)
        wave_data = wave_data.astype(np.float32) / 32767.0
        
        # Stereo to Mono for simplicity
        if n_channels == 2:
            wave_data = wave_data.reshape(-1, 2)
            wave_data = wave_data.mean(axis=1)

    time = np.linspace(0, len(wave_data) / framerate, num=len(wave_data))

    plt.figure(figsize=(12, 8))

    # Waveform
    plt.subplot(2, 1, 1)
    plt.plot(time, wave_data)
    plt.title(f"Waveform: {filename}")
    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude")

    # Spectrogram
    plt.subplot(2, 1, 2)
    plt.specgram(wave_data, Fs=framerate, NFFT=1024, noverlap=512)
    plt.title("Spectrogram")
    plt.xlabel("Time (s)")
    plt.ylabel("Frequency (Hz)")

    plt.tight_layout()
    plt.savefig("audio_verification.png")
    print(f"Visualizations saved to audio_verification.png")

if __name__ == "__main__":
    plot_audio("output.wav")

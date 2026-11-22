# Native WAV import

`File > Raw Data…`: after the file selector, a RIFF/WAVE file bypasses the raw-import dialog (`is_wav_file()` →
`import_wav()` in `src/FILE.C`). Any other file still uses the original raw-import dialog.

- Walks the RIFF chunks (`fmt `, `data`, skipping `LIST`/`smpl`/…, honouring odd-size padding), so only audio is loaded.
- PCM (tag 1) and WAVE_FORMAT_EXTENSIBLE (assumed PCM). 8-bit unsigned → `(b-128)<<8`; 16-bit little-endian → byte swap;
  24-bit → top 16 bits. Sets sample rate and period. Multi-channel files import the **left channel** with an alert.
- Refused with an alert: 32-bit, floating point, missing `fmt`/`data` chunk, empty data.

## Why it was needed
The original raw loader reads 16-bit words **big-endian** (ST native). WAV is **little-endian**, so 16-bit WAVs loaded as
full-scale byte-swapped noise (`screenshots/raw-load-16bit-wav-byteswapped-noise.png`). 8-bit *unsigned* WAVs (offset 44)
did load through it (`raw-load-8bit-wav-mortar-ok.png`), and trailing RIFF chunks were loaded as audio.

## Tested
16-bit mono 16 kHz; 8-bit; 24-bit stereo 44.1 kHz with a trailing `LIST` chunk — all gave the correct waveform
(`screenshots/native-wav-*.png`). Not yet verified: how the stored rate behaves in playback / MIDI dump.

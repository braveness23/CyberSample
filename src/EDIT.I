/* Prototypes for functions defined in D:\CYBR159\EDIT.C */
void edit_amplitude(register SD_HEADER *samp,
                    register short arg1,
                    register short arg2);
void hndl_fade_in_out(register SD_HEADER *samp);
void hndl_mirage(SD_HEADER *s);
void hndl_smoothing(SD_HEADER *s);
void hndl_rate_conv(SD_HEADER *s);
void hndl_compress(SD_HEADER *s);
void hndl_gate(SD_HEADER *s);
void hndl_bandpass(SD_HEADER *s);
void hndl_highlowpass(SD_HEADER *s,
                      short type);

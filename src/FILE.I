/* Prototypes for functions defined in D:\CYBR159\FILE.C */
short open_sd(SD_HEADER *samp,
              unsigned char *file);
short save_sd(SD_HEADER *samp,
              unsigned char *file);
short delete_file(void);
short import_raw(SD_HEADER *samp);
short hndl_fsel(unsigned char *path,
                unsigned char *file,
                unsigned char *suffix,
                unsigned char *msg);
void hndl_clipboard_to_sample(SD_HEADER *samp);
void hndl_sample_to_clipboard(SD_HEADER *samp);
void scrp_clear(unsigned char *scrppath);
void save_raw(SD_HEADER *samp,
              unsigned char *file);

/* Prototypes for functions defined in D:\CYBR159\GET.C */
void hndl_get(SD_HEADER *samp);
short receive_stnd_data(SD_HEADER *samp,
                        register short spp);
short extract_stnd_parameters(STND_HEADER *temp,
                             SD_HEADER *samp);
void req_data(unsigned char d,
              unsigned char s);
void Wait(unsigned char d,
          unsigned char p);
void ACK(unsigned char d,
         unsigned char p);
void Cancel(unsigned char d,
            unsigned char p);
void NAK(unsigned char d,
         unsigned char p);
short mouse_control(short flag);
void hndl_send(SD_HEADER *samp);
short transmit_stnd_data(SD_HEADER *samp,
                         short truncate,
                         short bits);
void pack_header(STND_HEADER *h,
                 SD_HEADER *samp,
                 short bits);

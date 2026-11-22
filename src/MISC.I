/* Prototypes for functions defined in D:\CYBR159\MISC.C */
void setup_samples(void);
void cleanup_samples(void);
short new_sample(SD_HEADER *samp,
                 short warn);
void init_sample_parameters(SD_HEADER *samp);
long samp_per_msec(SD_HEADER *s);
long samp_per_sec(SD_HEADER *s);
long msec_to_samples(SD_HEADER *s,
                     long msec);
short percent_to_sample_value(short percent);
long sample_length(register SD_HEADER *samp);
long edit_length(register SD_HEADER *samp);
short *off_addr(register SD_HEADER *samp,
                register long offset);
short *edit_address(register SD_HEADER *samp);
short *post_edit_address(register SD_HEADER *samp);
short max_zoom_ratio(register SD_HEADER *samp);
void reset_edit_params(SD_HEADER *samp);
void reset_view_params(SD_HEADER *samp);
void reset_loop1_params(SD_HEADER *samp);
void reset_loop2_params(SD_HEADER *samp);
void reset_filename(SD_HEADER *samp);
void hndl_select_all(SD_HEADER *samp);
void hndl_view_all(SD_HEADER *samp);
short mode_pexec(void);
void hndl_change_rate(SD_HEADER *samp);
void hndl_set_midi_notes(short note);
void situate_object_in_window(short hnd,
                              OBJECT *o);
void calc_exterior(short w_type,
                   GRECT *gint,
                   GRECT *gext);
void calc_interior(short w_type,
                   GRECT *gext,
                   GRECT *gint);
void set_clip(GRECT *r);
void unset_clip(void);
void grect_conv(GRECT *r,
                short *array);
void hndl_play_note(short note);
void force_redraw(short hnd,
                  OBJECT *tree,
                  short index);
void force_menu_event(short title,
                      short item);
short wind_create_open(WINDOW *w,
                       OBJECT *tree);
short wind_close_delete(WINDOW *w);
void hndl_disp_toggles(void);
void force_complete_redraw(void);
void wait_for_button_up(void);
void EditEnd_constrain(SD_HEADER *samp);
void EditStart_constrain(SD_HEADER *samp);
void Loop1Start_constrain(SD_HEADER *samp);
void Loop1End_constrain(SD_HEADER *samp);
void set_viewbar(SD_HEADER *samp);
short hndl_dial(OBJECT *tree,
                short first_edit);
void start_zero(SD_HEADER *samp);
void end_zero(SD_HEADER *samp);
short is_selected(register OBJECT *tree,
                  register short index);
void objc_iselect(register OBJECT *tree,
                  register short index,
                  register short truth);
void objc_deselect(OBJECT *tree,
                   short index);
short find_selected_radio(register OBJECT *tree,
                          register short parent);
void deselect_tree(register OBJECT *tree);

/* png.h */

extern int  il_png_init(il_container *ic);
extern int  il_png_write(il_container *, const uint8 *, int32);
extern void il_png_complete(il_container *ic);
extern unsigned int il_png_write_ready(il_container *ic);
extern void il_png_abort(il_container *ic);
extern void png_delay_time_callback(void *closure);

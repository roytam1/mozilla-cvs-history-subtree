/* gif.h */

extern int  il_gif_init(il_container *ic);
extern int  il_gif_write(il_container *, const uint8 *, int32);
extern void il_gif_complete(il_container *ic);
extern unsigned int il_gif_write_ready(il_container *ic);
extern void il_gif_abort(il_container *ic);
extern void gif_delay_time_callback(void *closure);

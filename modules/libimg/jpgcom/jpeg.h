/* jpeg.h */


extern int  il_jpeg_init(il_container *ic);
extern int  il_jpeg_write(il_container *, const uint8 *, int32);
extern void il_jpeg_complete(il_container *ic);
extern unsigned int il_jpeg_write_ready(il_container *ic);
extern void il_jpeg_abort(il_container *ic);



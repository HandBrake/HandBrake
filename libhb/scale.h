#include <inttypes.h>

typedef struct hb_scale_s hb_scale_t;

hb_scale_t * hb_scale_init( int inWidth, int inHeight, int outWidth,
                            int outHeight, int * crop );
void         hb_scale_process( hb_scale_t *, uint8_t * in, uint8_t * out );
void         hb_scale_close( hb_scale_t ** );

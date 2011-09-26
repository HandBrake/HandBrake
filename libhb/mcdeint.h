struct mcdeint_private_s
{
    int              mcdeint_mode;
    int              mcdeint_qp;

    int              mcdeint_outbuf_size;
    uint8_t        * mcdeint_outbuf;
    AVCodecContext * mcdeint_avctx_enc;
    AVFrame        * mcdeint_frame;
    AVFrame        * mcdeint_frame_dec;
};

typedef struct mcdeint_private_s mcdeint_private_t;

void mcdeint_init( mcdeint_private_t * pv,
                   int mode,
                   int qp,
                   int width,
                   int height );

void mcdeint_close( mcdeint_private_t * pv );

void mcdeint_filter( uint8_t ** dst,
                     uint8_t ** src,
                     int parity,
                     int * width,
                     int * height,
                     mcdeint_private_t * pv );

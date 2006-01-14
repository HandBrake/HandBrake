/* $Id: HandBrake.cpp,v 1.1.1.1 2003/06/24 13:43:48 titer Exp $ */

#include "HBApp.h"

#include <ffmpeg/avcodec.h>

int main()
{
    /* libavcodec initializations */
    avcodec_init();
    register_avcodec( &mpeg4_encoder );

    /* Run the BApplication */
    HBApp * app = new HBApp();
    app->Run();
    delete app;
    return 0;
}

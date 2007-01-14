#ifndef QUEUE_WINDOW_H
#define QUEUE_WINDOW_H

#include <interface/Window.h>
#include <interface/View.h>

#include "hb.h"

class QueueView : public BView
{
    public:
        QueueView( hb_handle_t * handle );
        ~QueueView();

        void HandleMessage( BMessage * msg );

    private:
        void AddStringView( char * string, BRect * r );
        void UpdateQueue();

        hb_handle_t  * fHandle;

        BScrollView  * fScrollView;
        BView        * fQueueView;
        BButton      * fCloseButton;
};

class QueueWindow : public BWindow
{
    public:
        QueueWindow( hb_handle_t * handle );
        void MessageReceived( BMessage * msg );

    private:
        QueueView * fView;
};

#endif

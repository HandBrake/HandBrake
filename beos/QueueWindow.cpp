#include <app/Message.h>
#include <interface/Button.h>
#include <interface/Screen.h>
#include <interface/ScrollView.h>
#include <interface/StringView.h>

#include "QueueWindow.h"

#define MSG_REMOVE 'remo'
#define MSG_CLOSE  'clos'

QueueView::QueueView( hb_handle_t * handle )
    : BView( BRect( 0,0,500,300 ), NULL, B_FOLLOW_NONE, B_WILL_DRAW )
{
    fHandle = handle;

    BRect b = Bounds(), r;

    r = BRect( b.right-90,b.bottom-35,b.right-10,b.bottom-10 );
    BButton * button = new BButton( r, NULL, "Close", new BMessage( MSG_CLOSE ) );
    AddChild( button );

    fScrollView = NULL;
    UpdateQueue();
}

QueueView::~QueueView()
{
}

void QueueView::HandleMessage( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_REMOVE:
            break;
    }
}

void QueueView::AddStringView( char * string, BRect * r )
{
    BStringView * stringView;

    stringView = new BStringView( *r, NULL, string );
    fQueueView->AddChild( stringView );
    free( string );

    r->top    += 20;
    r->bottom += 20;
}

void QueueView::UpdateQueue()
{
    BRect b = Bounds(), r;

    if( fScrollView )
    {
        RemoveChild( fScrollView );
        delete fScrollView;
    }

    r = BRect( b.left+10,b.top+10,b.right-B_V_SCROLL_BAR_WIDTH-10,b.bottom-45 );
    fQueueView  = new BView( r, NULL, B_FOLLOW_NONE, B_WILL_DRAW );
    fScrollView = new BScrollView( NULL, fQueueView, B_FOLLOW_NONE, 0,
                                   false, true, B_FANCY_BORDER );
    AddChild( fScrollView );

    hb_job_t * j;
    hb_title_t * t;
    int i;
    char * s;

    b = fQueueView->Bounds();
    r = BRect( b.left+10,b.top+10,b.right-10,b.top+25 );

    for( i = 0; i < hb_count( fHandle ); i++ )
    {
        j = hb_job( fHandle, i );
        t = j->title;

        asprintf( &s, "DVD: %s", t->dvd );
        AddStringView( s, &r );

        asprintf( &s, "Title: %d", t->index );
        AddStringView( s, &r );

        asprintf( &s, "Chapters: %d to %d", j->chapter_start, j->chapter_end );
        AddStringView( s, &r );

        asprintf( &s, "Pass: %d of %d", MAX( 1, j->pass ), MIN( 2, j->pass + 1 ) );
        AddStringView( s, &r );

        asprintf( &s, "Destination: %s", j->file );
        AddStringView( s, &r );
    }
}

QueueWindow::QueueWindow( hb_handle_t * handle )
    : BWindow( BRect( 0,0,10,10 ), "Queue",
               B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
               B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    /* Add the QueueView */
    fView = new QueueView( handle );
    AddChild( fView );
    
    /* Resize to fit */
    ResizeTo( fView->Bounds().Width(), fView->Bounds().Height() );
    
    /* Center */
    BScreen screen;
    MoveTo( ( screen.Frame().Width() - fView->Bounds().Width() ) / 2,
            ( screen.Frame().Height() - fView->Bounds().Height() ) / 2 );
}

void QueueWindow::MessageReceived( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_REMOVE:
            fView->HandleMessage( msg );
            break;

        case MSG_CLOSE:
            Lock();
            Quit();
            break;

        default:
            BWindow::MessageReceived( msg );
    }
}

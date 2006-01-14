/* $Id: Work.c,v 1.4 2003/11/06 12:33:11 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Thread.h"
#include "Work.h"

/* Local prototypes */
static void WorkThread( void * t );

struct HBWork
{
    HB_WORK_COMMON_MEMBERS
};

struct HBWorkThread
{
    HBHandle * handle;

    HBList   * workList;
    int        firstThread;

    int        die;
    HBThread * thread;
};

HBWorkThread * HBWorkThreadInit( HBHandle * handle, HBTitle * title,
                                 HBAudio * audio, HBAudio * optAudio,
                                 int firstThread )
{
    int i;
    HBWork * w;
    
    HBWorkThread * t;
    if( !( t = malloc( sizeof( HBWorkThread ) ) ) )
    {
        HBLog( "HBWorkThreadInit: malloc() failed, gonna crash" );
        return NULL;
    }

    t->handle = handle;

    /* Build a list of work objects. They all include
       HB_WORK_COMMON_MEMBERS, so we'll be able to do the job without
       knowing what each one actually do */
    t->workList = HBListInit();
    HBListAdd( t->workList, title->mpeg2Dec );
    HBListAdd( t->workList, title->scale );

    if( title->codec == HB_CODEC_FFMPEG )
        HBListAdd( t->workList, title->ffmpegEnc );
    else if( title->codec == HB_CODEC_XVID )
        HBListAdd( t->workList, title->xvidEnc );

    HBListAdd( t->workList, audio->ac3Dec );
    HBListAdd( t->workList, audio->mp3Enc );
    if( optAudio )
    {
        HBListAdd( t->workList, optAudio->ac3Dec );
        HBListAdd( t->workList, optAudio->mp3Enc );
    }

    t->firstThread = firstThread;

    /* Work objects are not thread-safe, so let's init locks so each
       one can not be called anymore when it's doing something. This
       is done by the first worker thread (see HBStartRip) */
    if( t->firstThread )
    {
        for( i = 0; i < HBListCountItems( t->workList ); i++ )
        {
            w = (HBWork*) HBListItemAt( t->workList, i );
            w->lock = HBLockInit();
            w->used = 0;
            w->time = 0;
        }
    }

    /* Actually launch the thread */
    t->die    = 0;
    t->thread = HBThreadInit( "work thread", WorkThread, t,
                              HB_LOW_PRIORITY );

    return t;
}

void HBWorkThreadClose( HBWorkThread ** _t )
{
    HBWorkThread * t = (*_t);
    HBWork * w;

    /* Stop the thread */
    t->die = 1;
    HBThreadClose( &t->thread );

    /* Destroy locks, show stats */
    if( t->firstThread )
    {
        int i;
        uint64_t total = 0;
        
        for( i = 0; i < HBListCountItems( t->workList ); i++ )
        {
            w = (HBWork*) HBListItemAt( t->workList, i );
            HBLockClose( &w->lock );
            total += w->time;
        }

        for( i = 0; i < HBListCountItems( t->workList ); i++ )
        {
            w = (HBWork*) HBListItemAt( t->workList, i );
            HBLog( "HBWorkThreadClose: %- 9s = %05.2f %%", w->name,
                   100.0 * w->time / total );
        }

    }
    
    /* Free memory */
    HBListClose( &t->workList );
    free( t );

    (*_t) = NULL;
}

static void WorkThread( void * _t )
{
    HBWorkThread * t = (HBWorkThread*) _t;
    HBWork       * w;
    int            didSomething, i;
    uint64_t       date;

    for( ;; )
    {
        HBCheckPaused( t->handle );
        
        didSomething = 0;

        for( i = 0; i < HBListCountItems( t->workList ); i++ )
        {
            if( t->die )
            {
                break;
            }
            
            w = (HBWork*) HBListItemAt( t->workList, i );

            /* Check if another thread isn't using this work object.
               If not, lock it */
            HBLockLock( w->lock );
            if( w->used )
            {
                HBLockUnlock( w->lock );
                continue;
            }
            w->used = 1;
            HBLockUnlock( w->lock );
            
            /* Actually do the job */
            date = HBGetDate();
            if( w->work( w ) )
            {
                w->time += HBGetDate() - date;
                didSomething = 1;
            }

            /* Unlock */
            HBLockLock( w->lock );
            w->used = 0;
            HBLockUnlock( w->lock );
        }

        if( t->die )
        {
            break;
        }

        /* If nothing could be done, wait a bit to prevent a useless
           CPU-consuming loop */
        if( !didSomething )
        {
            HBSnooze( 10000 );
        }
    }
}


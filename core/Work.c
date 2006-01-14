/* $Id: Work.c,v 1.12 2004/01/05 16:50:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* Local prototypes */
static void WorkThread( void * t );

struct HBWork
{
    HB_WORK_COMMON_MEMBERS
};

struct HBWorkThread
{
    HBHandle     * handle;

    HBList       * workList;
    int            firstThread;

    volatile int   die;
    HBThread     * thread;
};

HBWorkThread * HBWorkThreadInit( HBHandle * handle, HBTitle * title,
                                 int firstThread )
{
    int i;
    HBWork  * w;
    HBAudio * audio;

    HBWorkThread * t;
    if( !( t = malloc( sizeof( HBWorkThread ) ) ) )
    {
        HBLog( "HBWorkThreadInit: malloc() failed, gonna crash" );
        return NULL;
    }

    t->handle = handle;

    /* Build a list of work objects. They all include
       HB_WORK_COMMON_MEMBERS, so we'll be able to do the job without
       knowing what each one actually does */
    t->workList = HBListInit();
    HBListAdd( t->workList, title->decoder );
    HBListAdd( t->workList, title->scale );
    HBListAdd( t->workList, title->encoder );

    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        HBListAdd( t->workList, audio->decoder );
        HBListAdd( t->workList, audio->encoder );
    }

    t->firstThread = firstThread;

    /* Work objects are not thread-safe, so let's init locks so each
       one can not be called anymore when it's doing something. This
       is done by the first worker thread (see HBStartRip) */
    if( t->firstThread )
    {
        for( i = 0; i < HBListCount( t->workList ); i++ )
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

        for( i = 0; i < HBListCount( t->workList ); i++ )
        {
            w = (HBWork*) HBListItemAt( t->workList, i );
            HBLockClose( &w->lock );
            total += w->time;
        }

        for( i = 0; i < HBListCount( t->workList ); i++ )
        {
            w = (HBWork*) HBListItemAt( t->workList, i );
            HBLog( "HBWorkThreadClose: %- 9s = %05.2f %%", w->name,
                   100.0 * w->time / total );
        }

    }

    /* Free memory */
    HBListClose( &t->workList );
    free( t );

    *_t = NULL;
}

static void WorkThread( void * _t )
{
    HBWorkThread * t = (HBWorkThread*) _t;
    HBWork       * w;
    int            didSomething, i;
    uint64_t       date;

    didSomething = 0;
    
    for( i = 0; !t->die; i++ )
    {
        HBCheckPaused( t->handle );

        if( i == HBListCount( t->workList ) )
        {
            /* If nothing could be done, wait a bit to prevent a useless
               CPU-consuming loop */
            if( !didSomething )
            {
                HBSnooze( 5000 );
            }
            didSomething = 0;
            i            = 0;
        }

        w = (HBWork*) HBListItemAt( t->workList, i );

        /* Check if another thread isn't using this work object */
        HBLockLock( w->lock );
        if( w->used )
        {
            /* It's in use. Forget about this one and try the next
               one */
            HBLockUnlock( w->lock );
            continue;
        }
        /* It's unused, lock it */
        w->used = 1;
        HBLockUnlock( w->lock );

        /* Do the job */
        date = HBGetDate();
        if( w->work( w ) )
        {
            w->time += HBGetDate() - date;
            didSomething = 1;
        }

        /* Unlock it */
        HBLockLock( w->lock );
        w->used = 0;
        HBLockUnlock( w->lock );
    }
}


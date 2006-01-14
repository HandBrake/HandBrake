/* $Id: HBThread.h,v 1.3 2003/08/24 13:27:41 titer Exp $ */

#ifndef _HB_THREAD_H
#define _HB_THREAD_H

class HBThread
{
    public:
                      HBThread( char * name, int priority = 5 );
        virtual       ~HBThread();
        void          Run();
        void          Suspend();
        void          Resume();

    protected:
        volatile bool fDie;
    
    private:
        static long   ThreadFunc( HBThread * _this );
        virtual void  DoWork();
    
        char        * fName;
        int           fThread;
};

#endif

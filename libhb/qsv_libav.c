/* ********************************************************************* *\

Copyright (C) 2013 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

\* ********************************************************************* */

#include "handbrake/project.h"

#if HB_PROJECT_FEATURE_QSV

#include "handbrake/hbffmpeg.h"
#include "handbrake/qsv_libav.h"

void hb_qsv_add_context_usage(hb_qsv_context * qsv, int is_threaded)
{
    int is_active = 0;

    is_active = ff_qsv_atomic_inc(&qsv->is_context_active);
    if (is_active == 1) {
        memset(&qsv->mfx_session, 0, sizeof(mfxSession));
    }
}

int hb_qsv_context_clean(hb_qsv_context * qsv, int full_job)
{
    int is_active = 0;
    mfxStatus sts = MFX_ERR_NONE;

    is_active = ff_qsv_atomic_dec(&qsv->is_context_active);

    // spaces would have to be cleaned on the own,
    // here we care about the rest, common stuff
    if (is_active == 0) {
        if (qsv->mfx_session && !full_job) {
            sts = MFXClose(qsv->mfx_session);
            HB_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            qsv->mfx_session = 0;
            // display must be closed after MFXClose
            hb_display_close(&qsv->display);
            qsv->display = NULL;
        }
    }
    return 0;
}

#endif // HB_PROJECT_FEATURE_QSV

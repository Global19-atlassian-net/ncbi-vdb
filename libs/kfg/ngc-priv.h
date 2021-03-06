#ifndef _h_kfg_ngc_priv_
#define _h_kfg_ngc_priv_

/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/


#ifndef _h_kfg_ngc_
#include <kfg/ngc.h>
#endif

#ifndef _h_klib_data_buffer
#include <klib/data-buffer.h>
#endif

#ifndef _h_klib_text_
#include <klib/text.h>
#endif

#ifndef _h_klib_refcount_
#include <klib/refcount.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct KNgcObj
{
    KRefcount refcount;
    KDataBuffer buffer;
    String version, encryptionKey, downloadTicket, description;
    uint32_t projectId;
};


rc_t KNgcObjGetEncryptionKey(const KNgcObj *self,
    char * buffer, size_t buffer_size, size_t * written);

rc_t KNgcObjMakeFromCmdLine(const KNgcObj ** self);

#ifdef __cplusplus
}
#endif

#endif /* _h_kfg_ngc_priv_ */

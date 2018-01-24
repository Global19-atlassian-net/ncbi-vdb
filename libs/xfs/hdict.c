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

#include <klib/rc.h>
#include <klib/out.h>
#include <klib/container.h>
#include <klib/hashtable.h>
#include <klib/refcount.h>
#include <klib/printf.h>
#include <klib/log.h>
#include <kproc/lock.h>

#include "schwarzschraube.h"
#include "zehr.h"
#include "mehr.h"
#include "hdict.h"

#include <sysalloc.h>

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/
/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/


/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*
 * place, where hashgict is living. No loitering :Lol:
 *
 *_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*
 * JIPPOTAM
 *_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*
 *
 *_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/
struct _HashDictKey {
    SLNode node;
    const char * key;
};

static
rc_t CC
_HashDictKeyDispose ( const struct _HashDictKey * self )
{
    struct _HashDictKey * Key = ( struct _HashDictKey * ) self;

    if ( Key != NULL ) {
        if ( Key -> key != NULL ) {
            free ( ( char * ) Key -> key );
            Key -> key = NULL;
        }
    }

    return 0;
}   /* _HashDictKeyDispose () */

static
rc_t CC
_HashDictKeyMake ( const struct _HashDictKey ** HKey, const char * Key )
{
    rc_t RCt;
    struct _HashDictKey * Ret;

    RCt = 0;
    Ret = NULL;

    XFS_CSAN ( HKey )
    XFS_CAN ( HKey )
    XFS_CAN ( Key )

    Ret = calloc ( 1, sizeof ( struct _HashDictKey ) );
    if ( Ret == NULL ) {
        RCt = XFS_RC ( rcExhausted );
    }
    else {
        RCt = XFS_StrDup ( Key, & ( Ret -> key ) );
        if ( RCt == 0 ) {
            * HKey = Ret;
        }
    }

    if ( RCt != 0 ) {
        * HKey = NULL;

        if ( Ret != NULL ) {
            _HashDictKeyDispose ( Ret );
        }
    }

    return RCt;
}   /* _HashDictKeyMake () */

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*
 * XFSHashDict
 * Tapioca ... that structure contains data for resolved item
 * I do not know where I will use it, but, seems refcounts arent
 * necessary here.
 *_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/

struct XFSHashDict {
    struct KHashTable * hash_table;

    XFSHashDictBanana banana;

    struct SLList keys;
};

XFS_EXTERN
rc_t CC
XFSHashDictMake (
                const struct XFSHashDict ** Dict,
                XFSHashDictBanana Banana
)
{
    rc_t RCt;
    struct XFSHashDict * Ret;

    RCt = 0;
    Ret = NULL;

    XFS_CSAN ( Dict )
    XFS_CAN ( Dict )

    Ret = calloc ( 1, sizeof ( struct XFSHashDict ) );
    if ( Ret == NULL ) {
        RCt = XFS_RC ( rcExhausted );
    }
    else {
        Ret -> hash_table = calloc ( 1, sizeof ( struct KHashTable ) );
        if ( Ret -> hash_table == NULL ) {
            RCt = XFS_RC ( rcExhausted );
        }
        else {
            RCt = KHashTableInit (
                                Ret -> hash_table,
                                sizeof ( char * ), /* key_size ??? */
                                sizeof ( void * ), /* value_size */
                                0,      /* capacity, def value */
                                0,      /* max_load_factor, def value */
                                true    /* key_cstr */
                                );
            if ( RCt == 0 ) {
                SLListInit ( & ( Ret -> keys ) );
                Ret -> banana = Banana;

                * Dict = Ret;
            }
        }
    }

    if ( RCt != 0 ) {
        * Dict = NULL;

        if ( Ret != NULL ) {
            XFSHashDictDispose ( Ret );
        }
    }

    return RCt;
}   /* XFSHashDictMake () */

void
HashDictKeysWhackCallback ( struct SLNode * Node, void * Data )
{
    if ( Node != NULL ) {
        _HashDictKeyDispose ( ( struct _HashDictKey * ) Node );
    }
}   /* HashDictKeysWhackCallback () */

XFS_EXTERN
rc_t CC
XFSHashDictDispose ( const struct XFSHashDict * self )
{
    void * Key, * Val;
    struct XFSHashDict * Dict;

    Key = NULL;
    Val = NULL;
    Dict = ( struct XFSHashDict * ) self;

        /*))    I do believe that will happen on the end of the days,
         //     so no any kind of locking.
        ((*/
    if ( Dict != NULL ) {
        if ( Dict -> banana != NULL ) {
            KHashTableIteratorMake ( Dict -> hash_table );
            while (
                KHashTableIteratorNext (
                                        Dict -> hash_table,
                                        & Key,
                                        & Val
                                        )
            ) {
                Dict -> banana ( Val );
            }
        }

        SLListWhack ( & ( Dict -> keys ), HashDictKeysWhackCallback, NULL ) ;

        Dict -> banana = NULL;

        free ( Dict );
    }

    return 0;
}   /* XFSHashDictDispose () */

static
uint64_t CC
_HashDictKeyHashOfNullTerminatedStringLol ( const char * Key )
{
        /*))    We all know that Key is null terminated, right?
         ((*/
    if ( Key != NULL ) {
        return KHash ( Key, strlen ( Key ) );
    }
    return 0;
}   /* _HashDictKeyHashOfNullTerminatedStringLol () */

XFS_EXTERN
bool CC
XFSHashDictHas ( const struct XFSHashDict * self, const char * Key )
{
    void * Value = NULL;

    return XFSHashDictGet ( self, Value, Key ) == 0;
}   /* XFSHashDictHas () */

XFS_EXTERN
rc_t CC
XFSHashDictGet (
                const struct XFSHashDict * self,
                const void ** Value,
                const char * Key
)
{
    rc_t RCt;
    void * Ret;
    uint64_t KeyHash;

    RCt = 0;
    Ret = NULL;
    KeyHash = 0;

    XFS_CSAN ( Value )
    XFS_CAN ( self )
    XFS_CAN ( Value )
    XFS_CAN ( Key )

    KeyHash = _HashDictKeyHashOfNullTerminatedStringLol ( Key );

    if ( KHashTableFind ( self -> hash_table, Key, KeyHash, & Ret ) ) {
        * Value = Ret;
    }
    else {
        RCt = XFS_RC ( rcNotFound );
    }

    return RCt;
}   /* XFSHashDictGet () */

XFS_EXTERN
rc_t CC
XFSHashDictDel ( const struct XFSHashDict * self, const char * Key )
{
    rc_t RCt;
    uint64_t KeyHash;
    void * KeyVal;

    RCt = 0;
    KeyHash = 0;
    KeyVal = NULL;

    XFS_CAN ( self )
    XFS_CAN ( Key )

    KeyHash = _HashDictKeyHashOfNullTerminatedStringLol ( Key );

        /*)) First we should retrieve value for key and free it if
         //  banana for dictionary is set
        ((*/
    if ( self -> banana != NULL ) {
        if ( KHashTableFind (
                        self -> hash_table,
                        Key,
                        KeyHash,
                        & KeyVal )
        ) {
            self -> banana ( KeyVal );
        }
    }

        /*)) Second, we call secret procedure
         ((*/
    if ( ! KHashTableDelete ( self -> hash_table, Key, KeyHash ) ) {
        RCt = XFS_RC ( rcNotFound );
    }

    return RCt;
}   /* XFSHashDictDel () */

XFS_EXTERN
rc_t CC
XFSHashDictAdd (
                const struct XFSHashDict * self,
                const void * Value,
                const char * Key
)
{
    rc_t RCt;
    const struct _HashDictKey * DictKey;
    uint64_t KeyHash;

    RCt = 0;
    DictKey = NULL;
    KeyHash = 0;

    XFS_CAN ( self )
    XFS_CAN ( Value )
    XFS_CAN ( Key )

        /*) First we should delete previous value if it exists
         /  we don't care about RC code here :LOL:
        (*/
    XFSHashDictDel ( self, Key );

printf ( " [HDAD] [%d] [%p] [%s] [%p]\n", __LINE__, Key, Key, Value );
        /*) Second, we do create keykey and adding new value
         (*/
    RCt = _HashDictKeyMake ( & DictKey, Key );
    if ( RCt == 0 ) {
        SLListPushHead (
                    ( struct SLList * ) & ( self -> keys ),
                    ( struct SLNode * ) DictKey
                    );

            /*) Third, here we actually adding
             (*/
        KeyHash = _HashDictKeyHashOfNullTerminatedStringLol (
                                                        DictKey -> key
                                                        );
printf ( " [HDAD-II] [%d] [%p] [%s] [%p]\n", __LINE__, Key, Key, Value );
        RCt = KHashTableAdd (
                            self -> hash_table,
                            DictKey -> key,
                            KeyHash,
                            & Value
                            );
    }

    return RCt;
}   /* XFSHashDictAdd () */

XFS_EXTERN
rc_t CC
XFSHashDictReserve ( const struct XFSHashDict * self, size_t NewSize )
{
    rc_t RCt;

    RCt = 0;

    // JOJOBA TO DO

    return RCt;
}   /* XFSHashDictReserve () */


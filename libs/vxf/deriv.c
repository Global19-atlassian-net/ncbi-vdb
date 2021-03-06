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
#include <vdb/extern.h>
#include <vdb/xform.h>
#include <arch-impl.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>


#define DERIV_NAME( T )  deriv_ ## T
#define DERIV( T )                                                       \
static                                                                   \
rc_t CC DERIV_NAME ( T ) ( void *data,                                   \
    const VXformInfo *info, int64_t row_id, const VFixedRowResult *rslt, \
    uint32_t argc, const VRowData argv [] )                              \
{                                                                        \
    uint32_t i;                                                          \
    T prior, * dst = rslt -> base;                                       \
    const T * src = argv [ 0 ] . u . data . base;                        \
    dst += rslt -> first_elem;						                     \
    src += argv [ 0 ] . u . data . first_elem;				             \
    for ( prior = 0, i = 0; i < rslt -> elem_count; ++ i )               \
    {                                                                    \
        dst [ i ] = src [ i ] - prior;                                   \
        prior = src [ i ];                                               \
    }                                                                    \
    return 0;                                                            \
}

DERIV ( int8_t )
DERIV ( int16_t )
DERIV ( int32_t )
DERIV ( int64_t )

static VFixedRowFunc deriv_func [] =
{
    DERIV_NAME ( int8_t  ),
    DERIV_NAME ( int16_t ),
    DERIV_NAME ( int32_t ),
    DERIV_NAME ( int64_t )
};
/* deriv
 *  return the 1 derivative of inputs
 *
 *  "T" [ TYPE ] - input and output data type
 *  must be member of  signed integers
 *
 *  "a" [ DATA ] - operand
 *
 *
 * SYNOPSIS:
 *  return 1st derivative for every row with initial element unmodified
 *
 * USAGE:
 *    I32 pos_1st_d = < I32 > deriv ( position );
 */
VTRANSFACT_IMPL ( vdb_deriv, 1, 0, 0 ) ( const void *self, const VXfactInfo *info,
    VFuncDesc *rslt, const VFactoryParams *cp, const VFunctionParams *dp )
{
    int size_idx;
    if ( info -> fdesc . desc . domain != vtdInt)
    {
        return RC ( rcXF, rcFunction, rcConstructing, rcType, rcIncorrect );
    }

    /* TBD - eventually support vector derivatives
       for today, check that dim of T is 1 */
    if ( dp -> argv [ 0 ] . desc . intrinsic_dim != 1 )
        return RC ( rcXF, rcFunction, rcConstructing, rcType, rcIncorrect );

    /* the only numeric types we support are between 8 and 64 bits */
    size_idx = uint32_lsbit ( dp -> argv [ 0 ] . desc . intrinsic_bits ) - 3;
    if ( size_idx < 0 || size_idx > 3 || ( ( dp -> argv [ 0 ] . desc . intrinsic_bits &
                                             ( dp -> argv [ 0 ] . desc . intrinsic_bits - 1 ) ) != 0 ) )
        return RC ( rcXF, rcFunction, rcConstructing, rcType, rcIncorrect );


    rslt -> u . pf = deriv_func [ size_idx ];
    rslt -> variant = vftFixedRow;

    return 0;
}

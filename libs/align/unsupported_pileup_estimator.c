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

#include <align/extern.h>

#include <klib/rc.h>
#include <klib/text.h>
#include <klib/vector.h>

#include <vdb/manager.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>

#define DFLT_CACHE_SIZE 1024 * 1024 * 32

typedef struct PileupEstimator
{
    const VCursor * ref_cursor;
    const VCursor * align_cursor;

    Vector reftable;
    
    uint32_t ref_cursor_idx_SEQ_ID;
    uint32_t ref_cursor_idx_SEQ_LEN;
    uint32_t ref_cursor_idx_MAX_SEQ_LEN;
    uint32_t ref_cursor_idx_PRIMARY_ALIGNMENT_IDS;
    
    uint32_t align_cursor_idx_REF_POS;
    uint32_t align_cursor_idx_REF_LEN;

    uint32_t max_seq_len;
} PileupEstimator;


typedef struct RefEntry
{
    const String *rname;        /* the name of the reference */
    int64_t start_row_id;       /* the start-row-id of this reference in the REFERENCE-table */
    uint64_t count;             /* how many rows for this reference in the REFERENCE-table */
    uint64_t reflen;            /* how many bases does this reference have */
} RefEntry;


static void RefEntry_release( RefEntry * ref )
{
    if ( ref != NULL )
    {
        StringWhack( ref->rname );
        free( ( void * )ref );
    }
}

static rc_t make_ref_entry( RefEntry **self, const String * rname, int64_t start_row_id, uint32_t len )
{
    rc_t rc = 0;
    RefEntry * o = calloc( 1, sizeof *o );
    if ( o == NULL )
        rc = RC( rcAlign, rcQuery, rcConstructing, rcMemory, rcExhausted );
    else
    {
        o->start_row_id = start_row_id;
        o->reflen = len;
        rc = StringCopy( &o->rname, rname );
        if ( rc == 0 )
            *self = o;
        else
            RefEntry_release( o );
    }
    return rc;
}

static void CC RefEntry_releaes_callback( void * item, void * data )
{
    RefEntry_release( item );
}

LIB_EXPORT rc_t CC ReleasePileupEstimator( struct PileupEstimator *self )
{
    rc_t rc = 0;
    if ( self != NULL )
    {
        if ( self->ref_cursor != NULL )
            rc = VCursorRelease( self->ref_cursor );
        if ( rc == 0 && self->align_cursor != NULL )
            rc = VCursorRelease( self->align_cursor );
        VectorWhack( &self->reftable, RefEntry_releaes_callback, NULL );
        if ( rc == 0 )
            free( ( void * )self );
    }
    return rc;
}


/* ===================================================================================================== */


static rc_t AddRefCursor( PileupEstimator *self, const VCursor * ref_cursor )
{
    rc_t rc = VCursorAddRef( ref_cursor );
    if ( rc == 0 )
        self->ref_cursor = ref_cursor;
    if ( rc == 0 )
        rc = VCursorGetColumnIdx( ref_cursor, &self->ref_cursor_idx_SEQ_ID, "SEQ_ID" );
    if ( rc == 0 )
        rc = VCursorGetColumnIdx( ref_cursor, &self->ref_cursor_idx_SEQ_LEN, "SEQ_LEN" );
    if ( rc == 0 )
        rc = VCursorGetColumnIdx( ref_cursor, &self->ref_cursor_idx_MAX_SEQ_LEN, "MAX_SEQ_LEN" );
    if ( rc == 0 )
        rc = VCursorGetColumnIdx( ref_cursor, &self->ref_cursor_idx_PRIMARY_ALIGNMENT_IDS, "PRIMARY_ALIGNMENT_IDS" );
    return rc;
}

static rc_t MakeRefCursor( PileupEstimator *self, const VDatabase * db, size_t cursor_cache_size )
{
    const VTable * tbl;
    rc_t rc = VDatabaseOpenTableRead( db, &tbl, "%s", "REFERENCE" );
    if ( rc == 0 )
    {
        rc = VTableCreateCachedCursorRead( tbl, &self->ref_cursor, cursor_cache_size );
        if ( rc == 0 )
            rc = VCursorAddColumn( self->ref_cursor, &self->ref_cursor_idx_SEQ_ID, "SEQ_ID" );
        if ( rc == 0 )
            rc = VCursorAddColumn( self->ref_cursor, &self->ref_cursor_idx_SEQ_LEN, "SEQ_LEN" );
        if ( rc == 0 )
            rc = VCursorAddColumn( self->ref_cursor, &self->ref_cursor_idx_MAX_SEQ_LEN, "MAX_SEQ_LEN" );
        if ( rc == 0 )
            rc = VCursorAddColumn( self->ref_cursor, &self->ref_cursor_idx_PRIMARY_ALIGNMENT_IDS, "PRIMARY_ALIGNMENT_IDS" );
        if ( rc == 0 )
            rc = VCursorOpen( self->ref_cursor );
        VTableRelease( tbl );
    }
    return rc;
}

static rc_t AddAlignCursor( PileupEstimator *self, const VCursor * align_cursor )
{
    rc_t rc = VCursorAddRef( align_cursor );
    if ( rc == 0 )
        self->align_cursor = align_cursor;
    if ( rc == 0 )    
        rc = VCursorGetColumnIdx( align_cursor, &self->align_cursor_idx_REF_POS, "REF_POS" );
    if ( rc == 0 )
        rc = VCursorGetColumnIdx( align_cursor, &self->align_cursor_idx_REF_LEN, "REF_LEN" );
    return rc;
}

static rc_t MakeAlignCursor( PileupEstimator *self, const VDatabase * db, size_t cursor_cache_size )
{
    const VTable * tbl;
    rc_t rc = VDatabaseOpenTableRead( db, &tbl, "%s", "PRIMARY_ALIGNMENT" );
    if ( rc == 0 )
    {
        rc = VTableCreateCachedCursorRead( tbl, &self->ref_cursor, cursor_cache_size );
        if ( rc == 0 )
            rc = VCursorAddColumn( self->align_cursor, &self->align_cursor_idx_REF_POS, "REF_POS" );
        if ( rc == 0 )
            rc = VCursorAddColumn( self->align_cursor, &self->align_cursor_idx_REF_LEN, "REF_LEN" );
        if ( rc == 0 )
            rc = VCursorOpen( self->align_cursor );
        VTableRelease( tbl );
    }
    return rc;
}

static rc_t InitializePileupEstimator( PileupEstimator *self,
        const char * source,
        size_t cursor_cache_size,
        const VCursor * ref_cursor,
        const VCursor * align_cursor )
{
    rc_t rc = 0;
    VectorInit( &self->reftable, 0, 25 );
    if ( ref_cursor == NULL || align_cursor == NULL )
    {
        if ( source == NULL )
            rc = RC( rcAlign, rcQuery, rcAccessing, rcParam, rcNull );
        else
        {
            const VDBManager * mgr;
            rc = VDBManagerMakeRead( &mgr, NULL );
            if ( rc == 0 )
            {
                const VDatabase * db;
                rc = VDBManagerOpenDBRead( mgr, &db, NULL, "%s", source );
                if ( rc == 0 )
                {
                    if ( cursor_cache_size == 0 )
                        cursor_cache_size = DFLT_CACHE_SIZE;

                    if ( ref_cursor != NULL )
                        rc = AddRefCursor( self, ref_cursor );
                    else
                        rc = MakeRefCursor( self, db, cursor_cache_size );
                    
                    if ( rc == 0 )
                    {
                        if ( align_cursor != NULL )
                            rc = AddAlignCursor( self, align_cursor );
                        else
                            rc = MakeAlignCursor( self, db, cursor_cache_size );
                    }
                    VDatabaseRelease( db );
                }
                VDBManagerRelease( mgr );
            }
        }
    }
    else
    {
        rc = AddRefCursor( self, ref_cursor );
        if ( rc == 0 )
            rc = AddAlignCursor( self, align_cursor );
    }
    return rc;
}


LIB_EXPORT rc_t CC MakePileupEstimator( struct PileupEstimator **self,
        const char * source,
        size_t cursor_cache_size,
        const struct VCursor * ref_cursor,
        const struct VCursor * align_cursor )
{
    rc_t rc = 0;
    if ( self == NULL )
        rc = RC( rcAlign, rcQuery, rcConstructing, rcSelf, rcNull );
    else
    {
        PileupEstimator * o = calloc( 1, sizeof *o );
        *self = NULL;
        if ( o == NULL )
            rc = RC( rcAlign, rcQuery, rcConstructing, rcMemory, rcExhausted );
        else
        {
            rc = InitializePileupEstimator( o, source, cursor_cache_size, ref_cursor, align_cursor );
            if ( rc == 0 )
                *self = o;
            else
                ReleasePileupEstimator( o );
        }
    }
    return rc;
}

/* ===================================================================================================== */

static rc_t ScanRefTable( PileupEstimator *self )
{
    int64_t row;
    uint64_t count;
    RefEntry * ref_entry = NULL;
    String rname;
    uint32_t * ptr;
    uint32_t elem_bits, boff, row_len;
    
    rc_t rc = VCursorIdRange( self->ref_cursor, self->ref_cursor_idx_SEQ_ID, &row, &count );
    for( ; rc == 0 && row < count; ++row )
    {
        /* get max_seq_len if we do not have it yet */
        if ( self->max_seq_len == 0 )
        {
            rc = VCursorCellDataDirect( self->ref_cursor, row, self->ref_cursor_idx_MAX_SEQ_LEN, &elem_bits,
                                        ( const void ** )&ptr, &boff, &row_len );
            if ( rc == 0 && row_len < 1 )
                rc = RC( rcAlign, rcQuery, rcAccessing, rcParam, rcInvalid );
            if ( rc == 0 && ptr[ 0 ] == 0 )
                rc = RC( rcAlign, rcQuery, rcAccessing, rcParam, rcInvalid );
            if ( rc == 0 )
                self->max_seq_len = ptr[ 0 ];
        }
        
        /* get the REFERENCE-NAME */
        if ( rc == 0 )
        {
            rc = VCursorCellDataDirect( self->ref_cursor, row, self->ref_cursor_idx_SEQ_ID, &elem_bits,
                                        ( const void ** )&rname.addr, &boff, &rname.len );
            if ( rc == 0 && rname.len < 1 )
                rc = RC( rcAlign, rcQuery, rcAccessing, rcParam, rcInvalid );
            if ( rc == 0 )
                rname.size = rname.len;
        }
        
        /* get the SEQ_LEN */
        if ( rc == 0 )
        {
            rc = VCursorCellDataDirect( self->ref_cursor, row, self->ref_cursor_idx_SEQ_LEN, &elem_bits,
                                        ( const void ** )&ptr, &boff, &row_len );
            if ( rc == 0 && row_len < 1 )
                rc = RC( rcAlign, rcQuery, rcAccessing, rcParam, rcInvalid );
        }
        
        /* do the counting/inserting ... */
        if ( rc == 0 )
        {
            if ( ref_entry == NULL )
                rc = make_ref_entry( &ref_entry, &rname, row, ptr[ 0 ] );
            else
            {
                int cmp = StringCompare( ref_entry->rname, &rname );
                if ( cmp == 0 )
                {
                    ref_entry->count += 1;
                    ref_entry->reflen += ptr[ 0 ];
                }
                else
                {
                    rc = VectorAppend( &self->reftable, NULL, ref_entry );
                    if ( rc == 0 )
                        rc = make_ref_entry( &ref_entry, &rname, row, ptr[ 0 ] );
                }
            }
        }
    }
    /* insert the last entry */
    if ( rc == 0 && ref_entry != NULL )
        rc = VectorAppend( &self->reftable, NULL, ref_entry );

    return rc;
}

static rc_t FindRefEntry( PileupEstimator *self, RefEntry ** entry, const String * rname )
{
    rc_t rc = 0;
    uint32_t idx, count = VectorLength( &self->reftable );
    *entry = NULL;
    for( idx = 0; ( rc == 0 ) && ( idx < count ) && ( *entry == NULL ); ++idx )
    {
        RefEntry * e = VectorGet( &self->reftable, idx );
        if ( e == NULL )
            rc = RC( rcAlign, rcQuery, rcAccessing, rcItem, rcInvalid );
        else if ( 0 == StringCompare( e->rname, rname ) )
            *entry = e;
    }
    if ( *entry == NULL )
        rc = RC( rcAlign, rcQuery, rcAccessing, rcItem, rcNotFound );
    return rc;
}


static rc_t PerformEstimation( PileupEstimator *self,
                               RefEntry * entry,
                               uint64_t startpos,
                               uint32_t len,
                               uint64_t * result )
{
    rc_t rc = 0;
    uint32_t * coverage = calloc( len, sizeof *coverage );
    if ( coverage == NULL )
        rc = RC( rcAlign, rcQuery, rcConstructing, rcMemory, rcExhausted );
    else
    {
        uint64_t slice_end = startpos + len - 1;
        int64_t ref_row_id = entry->start_row_id;
        uint32_t ref_row_count, prim_id_count, prim_id_idx, ref_pos_count, ref_len_count, elem_bits, boff;
        int64_t * prim_ids;
        uint32_t * ref_pos_ptr;
        uint32_t * ref_len_ptr;
        
        /* adjust start_ref_row and ref_row_count to the given slice... */
        {
            int64_t start_offset = startpos / self->max_seq_len;
            int64_t end_offset = startpos + len - 1;
            end_offset /= self->max_seq_len;
            if ( start_offset > 0 ) start_offset--;
            ref_row_id += start_offset;
            ref_row_count = ( end_offset - start_offset ) + 1;
        }

        /* for each row in REFERENCE that matches the given slice... */
        for( ; rc == 0 && ref_row_id < ref_row_count; ref_row_id++ )
        {
            /* get the primary alignment-id's */
            rc = VCursorCellDataDirect( self->ref_cursor, ref_row_id, self->ref_cursor_idx_PRIMARY_ALIGNMENT_IDS,
                                        &elem_bits, ( const void ** )&prim_ids, &boff, &prim_id_count );
            /* for each alignment-id found */
            for ( prim_id_idx = 0; rc == 0 && prim_id_idx < prim_id_count; prim_id_idx++ )
            {
                /* get REF_POS */
                int64_t prim_id = prim_ids[ prim_id_idx ];
                rc = VCursorCellDataDirect( self->align_cursor, prim_id, self->align_cursor_idx_REF_POS,
                            &elem_bits, ( const void ** )&ref_pos_ptr, &boff, &ref_pos_count );
                if ( ref_pos_count != 1 )
                    rc = RC( rcAlign, rcQuery, rcAccessing, rcItem, rcInvalid );
                
                /* get REF_LEN */
                if ( rc == 0 )
                {
                    rc = VCursorCellDataDirect( self->align_cursor, prim_id, self->align_cursor_idx_REF_LEN,
                                &elem_bits, ( const void ** )&ref_len_ptr, &boff, &ref_len_count );
                    if ( ref_len_count != 1 )
                        rc = RC( rcAlign, rcQuery, rcAccessing, rcItem, rcInvalid );
                }

                /* enter the coverage */
                if ( rc == 0 )
                {
                    uint32_t ref_pos = ref_pos_ptr[ 0 ];
                    uint32_t ref_len = ref_len_ptr[ 0 ];
                    if ( ( ( ref_pos + ref_len - 1 ) >= startpos ) &&
                          ref_pos < slice_end )
                    {
                        int32_t rel_start = ( ref_pos - ( uint32_t )startpos );
                        int32_t i;
                        
                        if ( rel_start < 0 )
                        {
                            ref_len += rel_start;
                            rel_start = 0;
                        }
                        for ( i = rel_start; i < ref_len && i < len; ++i )
                            coverage[ i ]++; /* <==== */
                    }
                
                }
            }
        }

        /* sum up the bases in the slice */
        if ( rc == 0 )
        {
            uint32_t idx;
            uint64_t sum = 0;
            for ( idx = 0; idx < len; ++ idx ) sum += coverage[ idx ];
            *result = sum;
        }
        free( ( void * ) coverage );
    }
    return rc;
}

LIB_EXPORT rc_t CC RunPileupEstimator( struct PileupEstimator *self,
        const String * refname,
        uint64_t startpos,
        uint32_t len,
        uint64_t * result )
{
    rc_t rc = 0;
    if ( self == NULL )
        rc = RC( rcAlign, rcQuery, rcAccessing, rcSelf, rcNull );
    else if ( refname == NULL || result == NULL )
        rc = RC( rcAlign, rcQuery, rcAccessing, rcParam, rcNull );
    else
    {
        *result = 0;
        /* we are using max_seq_len as a flag to determine if we have to scan the reference-table */
        if ( self->max_seq_len == 0 )
            rc = ScanRefTable( self );
        if ( rc == 0 )
        {
            RefEntry * ref_entry;
            rc = FindRefEntry( self, &ref_entry, refname );
            if ( rc == 0 )
            {
                if ( startpos >= ref_entry->reflen ||
                     ( startpos + len ) - 1 >= ref_entry->reflen )
                    rc = RC( rcAlign, rcQuery, rcAccessing, rcItem, rcInvalid );
                else
                    rc = PerformEstimation( self, ref_entry, startpos, len, result );
            }
        }
    }
    return rc;
}

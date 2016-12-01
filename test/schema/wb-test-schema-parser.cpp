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

/**
* Unit tests for schema parser
*/

#include <ktst/unit_test.hpp>

#include "../../libs/schema/SchemaParser.hpp"
#include "../../libs/schema/ParseTree.hpp"

using namespace ncbi::SchemaParser;
#include "../../libs/schema/schema-tokens.h"

using namespace std;
using namespace ncbi::NK;

TEST_SUITE ( SchemaParserTestSuite );

// Token
TEST_CASE(Token_Construct)
{
    SchemaToken st = { KW_virtual, "virtual" };
    Token t ( st );
    REQUIRE_EQ ( (int)KW_virtual, t . GetType() );
    REQUIRE_EQ ( string ( "virtual" ), string ( t . GetValue() ) );
}

// // TokenNode
// TEST_CASE(TokenNode_Construct)
// {
//     TokenNode t(Token())
// }
// // ParseTree

// TEST_CASE(ParseTree_CreateDestroy)
// {
//     ParseTree pt;
// }

// TEST_CASE(ParseTree_AddChild)
// {
//     ParseTree pt;
//     pt. AddChild();
// }

bool ParseAndVerify(const char* p_source)
{
    ParseTree* pt;
    bool ret = SchemaParser () . ParseString ( p_source, pt );
    delete pt;
    return ret;
}

TEST_CASE ( EmptyInput )
{
    REQUIRE ( ParseAndVerify ( "" ) );
}

TEST_CASE ( Version1 )
{
    REQUIRE ( ParseAndVerify ( "version 1; include \"qq\";" ) );
}

TEST_CASE ( Typedef )
{
    REQUIRE ( ParseAndVerify ( "typedef oldName newName;" ) );
}
TEST_CASE ( TypedefDim )
{
    REQUIRE ( ParseAndVerify ( "typedef oldName newName [ 12 ];" ) );
}
TEST_CASE ( TypedefMultipleNames )
{
    REQUIRE ( ParseAndVerify ( "typedef oldName neawName1, newName2 [ 12 ], newName3;" ) );
}

TEST_CASE ( Typeset )
{
    REQUIRE ( ParseAndVerify ( "typeset newName { a, b[1], c };" ) );
}

TEST_CASE ( Format )
{
    REQUIRE ( ParseAndVerify ( "fmtdef newName;" ) );
}
TEST_CASE ( FormatRename )
{
    REQUIRE ( ParseAndVerify ( "fmtdef oldName newName;" ) );
}

TEST_CASE ( Const )
{
    REQUIRE ( ParseAndVerify ( "const t c = 1;" ) );
}
TEST_CASE ( ConstDim )
{
    REQUIRE ( ParseAndVerify ( "const t[2] c = 1;" ) );
}

TEST_CASE ( Alias )
{
    REQUIRE ( ParseAndVerify ( "alias a b;" ) );
}

TEST_CASE ( Extern_AndUntyped )
{
    REQUIRE ( ParseAndVerify ( "extern function __untyped fn ();" ) );
}

TEST_CASE ( Function_RowLength )
{
    REQUIRE ( ParseAndVerify ( "function __row_length fn ();" ) );
}

TEST_CASE ( Function_Naked )
{   //TODO: verify that paramter-less functions are not allowed
    REQUIRE ( ParseAndVerify ( "function t fn ( a b );" ) );
}

TEST_CASE ( Function_ArreyReturn )
{   //TODO: verify that paramter-less functions are not allowed
    REQUIRE ( ParseAndVerify ( "function t[3] fn ( a b );" ) );
}

TEST_CASE ( Function_Schema )
{
    REQUIRE ( ParseAndVerify ( "function < type b,  a / fmt c > t fn ( a b );" ) );
}

TEST_CASE ( Function_Factory )
{
    REQUIRE ( ParseAndVerify ( "function t fn < a b > ( a b );" ) );
}

TEST_CASE ( Function_NoFormals )
{
    REQUIRE ( ParseAndVerify ( "function t fn ();" ) );
}
TEST_CASE ( Function_Formals_OptionalOnly )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( * a b );" ) );
}
TEST_CASE ( Function_Formals_MandatoryAndOptional_NoComma )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b * a b );" ) );
}
TEST_CASE ( Function_Formals_MandatoryAndOptional )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b, * a b );" ) );
}
TEST_CASE ( Function_Formals_MandatoryAndVariadic )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b, ...  );" ) );
}
TEST_CASE ( Function_Formals_MandatoryOptionalAndVariadic )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b, * a b, ...  );" ) );
}
TEST_CASE ( Function_Formals_Control )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( control a b );" ) );
}

TEST_CASE ( Function_Prologue_Rename )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b ) = fn;" ) );
}
TEST_CASE ( Function_Prologue_Script_Return )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b ) { return 1; };" ) );
}
TEST_CASE ( Function_Prologue_Script_AssignFormat )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b ) { a / b c = 1; };" ) );
}
TEST_CASE ( Function_Prologue_Script_Assign )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b ) { a c = 1; };" ) );
}
TEST_CASE ( Function_Prologue_Script_Trigger )
{
    REQUIRE ( ParseAndVerify ( "function t fn ( a b ) { trigger c = 1; };" ) );
}

TEST_CASE ( Script )
{
    REQUIRE ( ParseAndVerify ( "schema t fn ( a b );" ) );
}
TEST_CASE ( Script_Function )
{
    REQUIRE ( ParseAndVerify ( "schema function t fn ( a b );" ) );
}

TEST_CASE ( Validate )
{
    REQUIRE ( ParseAndVerify ( "validate function t fn ( a b );" ) );
}

TEST_CASE ( Physical_Shorthand )
{
    REQUIRE ( ParseAndVerify ( "physical t fn #1.0 = { return 1; };" ) );
}

TEST_CASE ( Physical_Longhand )
{
    REQUIRE ( ParseAndVerify (
        "physical t fn #1.0 { decode { return 1; } ; encode { return 1; } ; __row_length = f () };" ) );
}

TEST_CASE ( Table_NoParents )
{
    REQUIRE ( ParseAndVerify ( "table t #1.1.1 { t a = 1; };" ) );
}
TEST_CASE ( Table_Parents )
{
    REQUIRE ( ParseAndVerify ( "table t #1.1.1 = t1, t2, t3 { t a = 1; };" ) );
}

TEST_CASE ( Table_Empty )
{
    REQUIRE ( ParseAndVerify ( "table t #1 {};" ) );
}
TEST_CASE ( Table_ProdStmt )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { t a = 1; };" ) );
}
TEST_CASE ( Table_ProdStmt_NoFormals )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { t p = < t > fn < 1 > (); };" ) );
}

TEST_CASE ( Table_Column )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column t c; };" ) );
}
TEST_CASE ( Table_Column_PhysicalEncoding )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column <1> p c; };" ) );
}
TEST_CASE ( Table_Column_PhysicalEncoding_WithVersion )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column <1> p#1.2.3 c; };" ) );
}
TEST_CASE ( Table_Column_PhysicalEncoding_WithFactory )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column <1> p <1> c; };" ) );
}
TEST_CASE ( Table_Column_PhysicalEncoding_WithVersionAndFactory )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column <1> p#1 <1> c; };" ) );
}
TEST_CASE ( Table_Column_Default )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { default column t c; };" ) );
}
TEST_CASE ( Table_Column_Extern_WithPhysical )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { extern column < U32 > izip_encoding #1 CHANNEL; };" ) );
}
TEST_CASE ( Table_Column_Extern_WithNakedPhysical )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { extern column bool_encoding #1 HIGH_QUALITY; };" ) );
}
TEST_CASE ( Table_Column_Extern_WithPhysicalFactory )
{
    REQUIRE ( ParseAndVerify ( "table t #1 {  column F32_4ch_encoding < 24 > BASE_FRACTION; };" ) );
}

TEST_CASE ( Table_Column_Readonly )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { readonly column t c; };" ) );
}
TEST_CASE ( Table_Column_AllMods )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { default extern readonly column t c; };" ) );
}

TEST_CASE ( Table_Column_default )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column default t c; };" ) );
}
TEST_CASE ( Table_Column_limit )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column limit = 1; };" ) );
}
TEST_CASE ( Table_Column_default_limit )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column default limit = 1; };" ) );
}

TEST_CASE ( Table_Column_withBody )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column t c { read = 1 | 2; validate = 2 | 3; limit = 100}; };" ) );
}
TEST_CASE ( Table_Column_withExpr )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { column t c = 0 | 1; };" ) );
}

TEST_CASE ( Table_DefaultView )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { default view \"QQ\"; };" ) );
}

TEST_CASE ( Table_PhysMbr_Static )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { static t .c = 1; };" ) );
}
TEST_CASE ( Table_PhysMbr_Physical )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { physical t .c = 1; };" ) );
}
TEST_CASE ( Table_PhysMbr_PhysicalWithVErsion )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { physical column NCBI #1 .CLIP_ADAPTER_LEFT; };" ) );
}

TEST_CASE ( Table_PhysMbr_StaticPhysical )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { static physical t .c = 1; };" ) );
}
TEST_CASE ( Table_PhysMbr_WithColumn )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { static column t .c = 1; };" ) );
}

TEST_CASE ( Table_PhysMbr_WithSchema )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { static column <1> t .c = 1; };" ) );
}
TEST_CASE ( Table_PhysMbr_WithVErsion )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { static column t#1 .c = 1; };" ) );
}
TEST_CASE ( Table_PhysMbr_WithFactory )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { static column t<1> .c = 1; };" ) );
}
TEST_CASE ( Table_PhysMbr_WithAll )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { static column <1>t#2.3.4<5> .c = 1; };" ) );
}

TEST_CASE ( Table_Untyped )
{
    REQUIRE ( ParseAndVerify ( "table t #1 { __untyped = a:b(); };" ) );
}

TEST_CASE ( Database_Empty )
{
    REQUIRE ( ParseAndVerify ( "database a:b #1.2.3 {};" ) );
}
TEST_CASE ( Database_WithParent )
{
    REQUIRE ( ParseAndVerify ( "database a:b #1.2.3 = ns:dad #4.5.6 {};" ) );
}

TEST_CASE ( Database_DbMember )
{
    REQUIRE ( ParseAndVerify ( "database d#1 { database ns : db DB; };" ) );
}
TEST_CASE ( Database_DbMember_WithTemplate )
{
    REQUIRE ( ParseAndVerify ( "database d#1 { template database ns : db DB; };" ) );
}

TEST_CASE ( Database_TableMember )
{
    REQUIRE ( ParseAndVerify ( "database d#1 { table ns : tbl T; };" ) );
}
TEST_CASE ( Database_TableMember_WithTemplate )
{
    REQUIRE ( ParseAndVerify ( "database d#1 { template table ns : tbl T; };" ) );
}

TEST_CASE ( Include )
{
    REQUIRE ( ParseAndVerify ( "include 'insdc/sra.vschema';" ) );
}

// Version 2

TEST_CASE ( VersionOther )
{
    REQUIRE ( ParseAndVerify ( "version 3.14; $" ) ); //TODO
}


//////////////////////////////////////////// Main
#include <kapp/args.h>
#include <kfg/config.h>
#include <klib/out.h>

extern "C"
{

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}

const char UsageDefaultName[] = "wb-test-schema-parser";

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg ( "Usage:\n" "\t%s [options] -o path\n\n", progname );
}

rc_t CC Usage( const Args* args )
{
    return 0;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    return SchemaParserTestSuite(argc, argv);
}

}

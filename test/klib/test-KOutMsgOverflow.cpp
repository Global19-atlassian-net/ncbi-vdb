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
* A test to reproduce heap-buffer-overflow generated by gcc's AddressSanitizer:
*
* All further test cases where sizeof fmt < 4
* produce access to memory outside of fmt boundaries.
*
* To trigger the problem:
* Use gcc 7.3, glibc 2.29),
* add ASAN options (-fsanitize=address) to build/Makefile.gcc,
* make test-KOutMsgOverflow
* run test-KOutMsgOverflow
*
* The error is in memcmp(( format ), ( literal ), sizeof ( literal )) call
* in KOutVMsg when sizeof ( format ) < sizeof ( literal )
*/

#include <klib/out.h>
#include <klib/text.h> /* String */
#include <klib/writer.h> /* KWrtInit */

#include <ktst/unit_test.hpp>

using std::string;

TEST_SUITE(KOutTestSuite);

#define ALL

// short-circuit formats in KMsgOut
rc_t CC writerFn(void * self,
    const char * buffer, size_t bufsize, size_t * num_writ)
{
    string & res = *(string *)self;
    res += string(buffer, bufsize);
    *num_writ = bufsize;
    return 0;
}

#ifdef ALL
TEST_CASE(TestOUTMSGNull) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC_FAIL(KOutMsg(NULL));
    REQUIRE_EQ(output.find("outmsg failure"), (std::size_t)0);
}

TEST_CASE(TestOUTMSG1) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg(""));
    REQUIRE_EQ(output, string(""));
}

TEST_CASE(TestOUTMSG2) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("1"));
    REQUIRE_EQ(output, string("1"));
}

TEST_CASE(TestOUTMSG2Eol) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("\n"));
    REQUIRE_EQ(output, string("\n"));
}

TEST_CASE(TestOUTMSG3) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("12"));
    REQUIRE_EQ(output, string("12"));
}

TEST_CASE(TestOUTMSG4) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("123"));
    REQUIRE_EQ(output, string("123"));
}
#endif

TEST_CASE(TestOUTMSG5) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("1234"));
    REQUIRE_EQ(output, string("1234"));
}

TEST_CASE(TestOUTMSGs) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("%s", "string"));
    REQUIRE_EQ(output, string("string"));
}

TEST_CASE(TestOUTMSGss) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("%.*s", 6, "STRING"));
    REQUIRE_EQ(output, string("STRING"));
}

TEST_CASE(TestOUTMSGS) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    String s;
    CONST_STRING(&s, "String");
    REQUIRE_RC(KOutMsg("%S", &s));
    REQUIRE_EQ(output, string("String"));
}

TEST_CASE(TestOUTMSGc) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("%c", 'c'));
    REQUIRE_EQ(output, string("c"));
}

TEST_CASE(TestOUTMSG_s) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("%s.", "string"));
    REQUIRE_EQ(output, string("string."));
}

TEST_CASE(TestOUTMSG_ss) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("%.*s.", 6, "STRING"));
    REQUIRE_EQ(output, string("STRING."));
}

TEST_CASE(TestOUTMS_GS) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    String s;
    CONST_STRING(&s, "String");
    REQUIRE_RC(KOutMsg("%S.", &s));
    REQUIRE_EQ(output, string("String."));
}

TEST_CASE(TestOUTMSG_c) {
    string output;
    REQUIRE_RC(KOutHandlerSet(writerFn, &output));
    REQUIRE_RC(KOutMsg("%c.", 'c'));
    REQUIRE_EQ(output, string("c."));
}

extern "C" {
#ifdef WINDOWS
    #define main wmain
#endif
    int main(int argc, char *argv[]) {
        KWrtInit(argv[0], 0);
        return KOutTestSuite(argc, argv);
    }
}
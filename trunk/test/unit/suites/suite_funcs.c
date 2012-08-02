#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "utils.h"


#if 0

#define test_one( b, a, expect ) \
{                                                                        \
    char o[4096*2];                                                      \
                                                                         \
    printf( "'%s' + '%s'\n", b, a );                                     \
                                                                         \
    if( uu_make_absname( o, b, a ) )                                     \
        CU_FAIL( "uu_absname failed" );                                  \
                                                                         \
    if( expect && strcmp( o, expect ) )                                  \
    {                                                                    \
        printf( "Expected '%s', got '%s'\n", expect, o );                \
        CU_FAIL( expect );                                               \
    }                                                                    \
}


TEST_FUNCT(filename) {
    //printf("test case 1\n");
     /* Фейковый код */
    //CU_ASSERT_EQUAL(0, 0);


    test_one("abc", 		"def", 		"/abc/def"	);
    test_one("..", 		"def",		"/def"		);
    test_one("abc", 		"..",		"/"		);
    test_one("abc/.", 		"//def",	"/abc/def"	);
    test_one("abc/../", 	"def",		"/def"		);
    test_one("./abc", 		"../def",	"/def"		);
    test_one("abc", 		"def/../xyz", 	"/abc/xyz"	);
    test_one("/abc/../.../aa", 	"../def",	"/.../def"	);
    test_one("/abc/xyz", 	"..",		"/abc"		);
    test_one("../../abc", 	"def",		"/abc/def"	);
    test_one("////abc//.././", 	"def",		"/def" 		);

    test_one("abc/../", 	".", 		"/"		);
    test_one("./abc", 		".",		"/abc"		);
    test_one("abc", 		"def/../xyz/.",	"/abc/xyz"	);
    test_one("/abc/../.../aa", 	"./def",	"/.../aa/def"	);
    test_one("/abc/xyz.//.", 	".", 		"/abc/xyz."	);
    test_one("../../abc..", 	".",		"/abc.."	);
    test_one("////abc//.././..", ".", 		"/"		);

    int nb;

#define NPART 10

    const char *oname[NPART];
    int olen[NPART];

    nb = uu_break_path( "/aa/bb/ccc", NPART, oname, olen );

    int i;
    for( i = 0; i < nb; i ++ )
    {
        printf("part %d '%.*s'\n", i, olen[i], oname[i] );
    }

}

#endif

#include <video/rect.h>

TEST_FUNCT(rectangles)
{
    //printf("test case 2\n");
    /* Фейковый код */
    //CU_ASSERT_EQUAL(1, 1);

    rect_t out1, out2, oldw, neww;

    oldw.x = 1;
    oldw.y = 1;
    oldw.xsize = 10;
    oldw.ysize = 10;

    neww.x = 2;
    neww.y = 2;
    neww.xsize = 10;
    neww.ysize = 10;

    //int o2 =
    rect_sub( &out1, &out2, &oldw, &neww );

    //rect_dump( &out1 );
    //rect_dump( &out2 );

    test_check_eq(out1.x,1);
    test_check_eq(out1.y,1);
    test_check_eq(out1.xsize,1);
    test_check_eq(out1.ysize,10);

    test_check_eq(out2.x,1);
    test_check_eq(out2.y,1);
    test_check_eq(out2.xsize,10);
    test_check_eq(out2.ysize,1);

    rect_t a, b;

    a.x     = 10; a.y     = 10;
    a.xsize = 10; a.ysize = 10;

    b.x     = 15; b.y     = 15;
    b.xsize = 10; b.ysize = 10;

    test_check_false( rect_includes( &a, &b ) );
    test_check_true( rect_intersects( &a, &b ) );

    b.xsize = 2;  b.ysize = 2;

    test_check_true( rect_includes( &a, &b ) );
    test_check_true( rect_intersects( &a, &b ) );

    b.x     = 35; b.y     = 35;

    test_check_false( rect_includes( &a, &b ) );
    test_check_false( rect_intersects( &a, &b ) );

}

void runSuite(void) {
    /* Код тест-сьюта */

    //printf("test suite\n");

    CU_pSuite suite = CUnitCreateSuite("Functions");
    if (suite)
    {
        //ADD_SUITE_TEST(suite, filename)
        ADD_SUITE_TEST(suite, rectangles)
    }
}

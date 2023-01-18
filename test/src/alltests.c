#include "ccheckmate.h"
#include "test_intimage.c"

BEGIN_CCHECK_MATE
    #ifdef FAST
    fprintf(stderr, "\nRunning tests in release mode.\n");
    #endif
    testIntImage();
END_CCHECK_MATE

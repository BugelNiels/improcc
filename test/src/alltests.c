#include "ccheckmate.h"
#include "intimage/test_morphology.c"

ccm_begin_test_suite;
#ifdef FAST
fprintf(stderr, "\nRunning tests in release mode.\n");
#endif
morphology();
ccm_end_test_suite;

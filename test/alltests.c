#include "ccheckmate/ccheckmate.h"

void testTrue() { assert_true(1); }

BEGIN_CCHECK_MATE
#ifdef FAST
fprintf(stderr, "\nRunning tests in release mode.\n");
#endif
start_section("Testing true");
ccm_test(testTrue);
END_CCHECK_MATE

// Isolated Windows build smoke test for PW-LoadTrace.h.
// Compile with VS2022 x64; does not need licensed GameGuru media.
#include "PW-LoadTrace.h"
int main()
{
    PWLoadTraceScope testLoad("test.synthetic_load");
    Sleep(3);
    return 0;
}

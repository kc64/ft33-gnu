typedef unsigned char       byte;
typedef unsigned int        word;
typedef unsigned long       dword;
typedef int                 sword;
typedef long                sdword;

#define TRUE            1
#define FALSE           0

typedef struct {
    byte ticks;
    byte Chan[16];
    } sSDStep;

sSDStep *ptrSDSequences[16];
word SDSequenceLengths[16];

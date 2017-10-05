
#include "types.h"

typedef struct {
    byte start;
    byte stop;
    } sDimChanStep;

typedef struct {
    byte ticks;
    sDimChanStep Chan[8];
    } sDimStep;

sDimStep *ptrDimSequences[16];
word DimSequenceLengths[16];

/*       
const sDimStep dim_seq0[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq1[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq2[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq3[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq4[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq5[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq6[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq7[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq8[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq9[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq10[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq11[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq12[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq13[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq14[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
const sDimStep dim_seq15[] = { {.Chan[0]={0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}, {0, 255}},
                                {.Chan[0]={0, 120}, {120, 0}, {0, 120}, {0, 120}, {0, 120}, {120, 0}, {255, 0}, {0, 120}} };
                                


const sDimStep *ptrDimSequences[] = { dim_seq0, dim_seq1, dim_seq2, dim_seq3, dim_seq4, dim_seq5, dim_seq6, dim_seq7, 
                                        dim_seq8, dim_seq9, dim_seq10, dim_seq11, dim_seq12, dim_seq13, dim_seq14, dim_seq15 };

const word DimSequenceLengths[] = { sizeof(dim_seq0) / sizeof (sDimStep),
                                    sizeof(dim_seq1) / sizeof (sDimStep),
                                    sizeof(dim_seq2) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq3) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq4) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq5) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq6) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq7) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq8) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq9) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq10) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq11) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq12) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq13) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq14) / sizeof (sDimStep),                                    
                                    sizeof(dim_seq15) / sizeof (sDimStep) };
*/

    
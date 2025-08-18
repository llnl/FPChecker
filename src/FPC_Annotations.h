#ifndef FPC_ANNOTATIONS_H
#define FPC_ANNOTATIONS_H

#define FPC_INSTRUMENT_BLOCK __attribute__((annotate("_FPC_INSTRUMENT_BLOCK_"))) int _marker __attribute__((unused)) = 0;
#define FPC_INSTRUMENT_FUNC __attribute__((annotate("_FPC_INSTRUMENT_FUNCTION_")))
#define FPC_CALCULATE_ERROR __attribute__((noinline)) __attribute__((annotate("_FPC_CALCULATE_ERROR_"))) // *** Error Calculation *** //

#endif // FPC_ANNOTATIONS_H
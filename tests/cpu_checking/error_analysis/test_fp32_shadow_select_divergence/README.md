This regression test checks FP-controlled select instructions when FP32 and shadow FP64 comparisons disagree.
FPChecker should use the shadow comparison to report the FP64-vs-FP32 error for the selected value.

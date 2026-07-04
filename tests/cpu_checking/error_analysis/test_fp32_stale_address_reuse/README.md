This regression test checks that stale shadow metadata is not reused after memory is changed outside FPChecker instrumentation.
The runtime should reconcile the shadow value with the actual FP32 memory value on load.

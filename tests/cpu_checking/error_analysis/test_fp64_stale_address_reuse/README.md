This regression test checks that the FP64 runtime does not reuse stale shadow metadata after memory is changed outside instrumentation.
The runtime should reconcile the shadow value with the actual double value on load.

This regression test checks shadow propagation through STL vector copies across compilation units.
The final FPChecker error should match the explicit FP64 reference after vector copy, growth, transform, and reduction.

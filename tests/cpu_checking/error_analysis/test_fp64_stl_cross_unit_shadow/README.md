This regression test checks FP64 shadow propagation through STL vector copies across compilation units.
The final FPChecker error should match the explicit long-double reference after vector copy, growth, transform, and reduction.

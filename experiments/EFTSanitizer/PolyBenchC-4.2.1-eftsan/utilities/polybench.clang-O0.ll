; ModuleID = 'utilities/polybench.c'
source_filename = "utilities/polybench.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@polybench_papi_counters_threadid = dso_local global i32 0, align 4
@polybench_program_total_flops = dso_local global double 0.000000e+00, align 8
@.str = private unnamed_addr constant [12 x i8] c"tmp <= 10.0\00", align 1
@.str.1 = private unnamed_addr constant [22 x i8] c"utilities/polybench.c\00", align 1
@__PRETTY_FUNCTION__.polybench_flush_cache = private unnamed_addr constant [29 x i8] c"void polybench_flush_cache()\00", align 1
@polybench_t_start = dso_local global double 0.000000e+00, align 8
@polybench_t_end = dso_local global double 0.000000e+00, align 8
@.str.2 = private unnamed_addr constant [7 x i8] c"%0.6f\0A\00", align 1
@polybench_c_start = dso_local global i64 0, align 8
@polybench_c_end = dso_local global i64 0, align 8
@polybench_inter_array_padding_sz = internal global i64 0, align 8
@stderr = external dso_local global ptr, align 8
@.str.3 = private unnamed_addr constant [51 x i8] c"[PolyBench] posix_memalign: cannot allocate memory\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_flush_cache() #0 {
  %1 = alloca i32, align 4
  %2 = alloca ptr, align 8
  %3 = alloca i32, align 4
  %4 = alloca double, align 8
  store i32 4194560, ptr %1, align 4
  %5 = load i32, ptr %1, align 4
  %6 = sext i32 %5 to i64
  %7 = call noalias ptr @calloc(i64 noundef %6, i64 noundef 8) #5
  store ptr %7, ptr %2, align 8
  store double 0.000000e+00, ptr %4, align 8
  store i32 0, ptr %3, align 4
  br label %8

8:                                                ; preds = %20, %0
  %9 = load i32, ptr %3, align 4
  %10 = load i32, ptr %1, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %12, label %23

12:                                               ; preds = %8
  %13 = load ptr, ptr %2, align 8
  %14 = load i32, ptr %3, align 4
  %15 = sext i32 %14 to i64
  %16 = getelementptr inbounds double, ptr %13, i64 %15
  %17 = load double, ptr %16, align 8
  %18 = load double, ptr %4, align 8
  %19 = fadd double %18, %17
  store double %19, ptr %4, align 8
  br label %20

20:                                               ; preds = %12
  %21 = load i32, ptr %3, align 4
  %22 = add nsw i32 %21, 1
  store i32 %22, ptr %3, align 4
  br label %8, !llvm.loop !5

23:                                               ; preds = %8
  %24 = load double, ptr %4, align 8
  %25 = fcmp ole double %24, 1.000000e+01
  br i1 %25, label %26, label %27

26:                                               ; preds = %23
  br label %28

27:                                               ; preds = %23
  call void @__assert_fail(ptr noundef @.str, ptr noundef @.str.1, i32 noundef 123, ptr noundef @__PRETTY_FUNCTION__.polybench_flush_cache) #6
  unreachable

28:                                               ; preds = %26
  %29 = load ptr, ptr %2, align 8
  call void @free(ptr noundef %29) #7
  ret void
}

; Function Attrs: nounwind allocsize(0,1)
declare dso_local noalias ptr @calloc(i64 noundef, i64 noundef) #1

; Function Attrs: noreturn nounwind
declare dso_local void @__assert_fail(ptr noundef, ptr noundef, i32 noundef, ptr noundef) #2

; Function Attrs: nounwind
declare dso_local void @free(ptr noundef) #3

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_prepare_instruments() #0 {
  call void @polybench_flush_cache()
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_timer_start() #0 {
  call void @polybench_prepare_instruments()
  %1 = call double @rtclock()
  store double %1, ptr @polybench_t_start, align 8
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_timer_stop() #0 {
  %1 = call double @rtclock()
  store double %1, ptr @polybench_t_end, align 8
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_timer_print() #0 {
  %1 = load double, ptr @polybench_t_end, align 8
  %2 = load double, ptr @polybench_t_start, align 8
  %3 = fsub double %1, %2
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.2, double noundef %3)
  ret void
}

declare dso_local i32 @printf(ptr noundef, ...) #4

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_free_data(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  call void @free(ptr noundef %3) #7
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local ptr @polybench_alloc_data(i64 noundef %0, i32 noundef %1) #0 {
  %3 = alloca i64, align 8
  %4 = alloca i32, align 4
  %5 = alloca i64, align 8
  %6 = alloca ptr, align 8
  store i64 %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  %7 = load i64, ptr %3, align 8
  store i64 %7, ptr %5, align 8
  %8 = load i32, ptr %4, align 4
  %9 = sext i32 %8 to i64
  %10 = load i64, ptr %5, align 8
  %11 = mul i64 %10, %9
  store i64 %11, ptr %5, align 8
  %12 = load i64, ptr %5, align 8
  %13 = call ptr @xmalloc(i64 noundef %12)
  store ptr %13, ptr %6, align 8
  %14 = load ptr, ptr %6, align 8
  ret ptr %14
}

; Function Attrs: noinline nounwind uwtable
define internal ptr @xmalloc(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  %3 = alloca ptr, align 8
  %4 = alloca i64, align 8
  %5 = alloca i32, align 4
  store i64 %0, ptr %2, align 8
  store ptr null, ptr %3, align 8
  %6 = load i64, ptr @polybench_inter_array_padding_sz, align 8
  %7 = add i64 %6, 0
  store i64 %7, ptr @polybench_inter_array_padding_sz, align 8
  %8 = load i64, ptr %2, align 8
  %9 = load i64, ptr @polybench_inter_array_padding_sz, align 8
  %10 = add i64 %8, %9
  store i64 %10, ptr %4, align 8
  %11 = load i64, ptr %4, align 8
  %12 = call i32 @posix_memalign(ptr noundef %3, i64 noundef 4096, i64 noundef %11) #7
  store i32 %12, ptr %5, align 4
  %13 = load ptr, ptr %3, align 8
  %14 = icmp ne ptr %13, null
  br i1 %14, label %15, label %18

15:                                               ; preds = %1
  %16 = load i32, ptr %5, align 4
  %17 = icmp ne i32 %16, 0
  br i1 %17, label %18, label %21

18:                                               ; preds = %15, %1
  %19 = load ptr, ptr @stderr, align 8
  %20 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %19, ptr noundef @.str.3) #7
  call void @exit(i32 noundef 1) #6
  unreachable

21:                                               ; preds = %15
  %22 = load ptr, ptr %3, align 8
  ret ptr %22
}

; Function Attrs: noinline nounwind uwtable
define internal double @rtclock() #0 {
  ret double 0.000000e+00
}

; Function Attrs: nounwind
declare dso_local i32 @posix_memalign(ptr noundef, i64 noundef, i64 noundef) #3

; Function Attrs: nounwind
declare dso_local i32 @fprintf(ptr noundef, ptr noundef, ...) #3

; Function Attrs: noreturn nounwind
declare dso_local void @exit(i32 noundef) #2

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind allocsize(0,1) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { noreturn nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { nounwind allocsize(0,1) }
attributes #6 = { noreturn nounwind }
attributes #7 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 7, !"Dwarf Version", i32 4}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}

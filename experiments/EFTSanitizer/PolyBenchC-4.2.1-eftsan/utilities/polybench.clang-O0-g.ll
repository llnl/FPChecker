; ModuleID = 'utilities/polybench.c'
source_filename = "utilities/polybench.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@polybench_papi_counters_threadid = dso_local global i32 0, align 4, !dbg !0
@polybench_program_total_flops = dso_local global double 0.000000e+00, align 8, !dbg !9
@.str = private unnamed_addr constant [12 x i8] c"tmp <= 10.0\00", align 1, !dbg !11
@.str.1 = private unnamed_addr constant [22 x i8] c"utilities/polybench.c\00", align 1, !dbg !17
@__PRETTY_FUNCTION__.polybench_flush_cache = private unnamed_addr constant [29 x i8] c"void polybench_flush_cache()\00", align 1, !dbg !22
@polybench_t_start = dso_local global double 0.000000e+00, align 8, !dbg !33
@polybench_t_end = dso_local global double 0.000000e+00, align 8, !dbg !35
@.str.2 = private unnamed_addr constant [7 x i8] c"%0.6f\0A\00", align 1, !dbg !28
@polybench_c_start = dso_local global i64 0, align 8, !dbg !37
@polybench_c_end = dso_local global i64 0, align 8, !dbg !40
@polybench_inter_array_padding_sz = internal global i64 0, align 8, !dbg !47
@stderr = external dso_local global ptr, align 8
@.str.3 = private unnamed_addr constant [51 x i8] c"[PolyBench] posix_memalign: cannot allocate memory\00", align 1, !dbg !42

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_flush_cache() #0 !dbg !59 {
  %1 = alloca i32, align 4
  %2 = alloca ptr, align 8
  %3 = alloca i32, align 4
  %4 = alloca double, align 8
    #dbg_declare(ptr %1, !63, !DIExpression(), !64)
  store i32 4194560, ptr %1, align 4, !dbg !64
    #dbg_declare(ptr %2, !65, !DIExpression(), !66)
  %5 = load i32, ptr %1, align 4, !dbg !67
  %6 = sext i32 %5 to i64, !dbg !67
  %7 = call noalias ptr @calloc(i64 noundef %6, i64 noundef 8) #5, !dbg !68
  store ptr %7, ptr %2, align 8, !dbg !66
    #dbg_declare(ptr %3, !69, !DIExpression(), !70)
    #dbg_declare(ptr %4, !71, !DIExpression(), !72)
  store double 0.000000e+00, ptr %4, align 8, !dbg !72
  store i32 0, ptr %3, align 4, !dbg !73
  br label %8, !dbg !75

8:                                                ; preds = %20, %0
  %9 = load i32, ptr %3, align 4, !dbg !76
  %10 = load i32, ptr %1, align 4, !dbg !78
  %11 = icmp slt i32 %9, %10, !dbg !79
  br i1 %11, label %12, label %23, !dbg !80

12:                                               ; preds = %8
  %13 = load ptr, ptr %2, align 8, !dbg !81
  %14 = load i32, ptr %3, align 4, !dbg !82
  %15 = sext i32 %14 to i64, !dbg !81
  %16 = getelementptr inbounds double, ptr %13, i64 %15, !dbg !81
  %17 = load double, ptr %16, align 8, !dbg !81
  %18 = load double, ptr %4, align 8, !dbg !83
  %19 = fadd double %18, %17, !dbg !83
  store double %19, ptr %4, align 8, !dbg !83
  br label %20, !dbg !84

20:                                               ; preds = %12
  %21 = load i32, ptr %3, align 4, !dbg !85
  %22 = add nsw i32 %21, 1, !dbg !85
  store i32 %22, ptr %3, align 4, !dbg !85
  br label %8, !dbg !86, !llvm.loop !87

23:                                               ; preds = %8
  %24 = load double, ptr %4, align 8, !dbg !90
  %25 = fcmp ole double %24, 1.000000e+01, !dbg !90
  br i1 %25, label %26, label %27, !dbg !90

26:                                               ; preds = %23
  br label %28, !dbg !90

27:                                               ; preds = %23
  call void @__assert_fail(ptr noundef @.str, ptr noundef @.str.1, i32 noundef 123, ptr noundef @__PRETTY_FUNCTION__.polybench_flush_cache) #6, !dbg !90
  unreachable, !dbg !90

28:                                               ; preds = %26
  %29 = load ptr, ptr %2, align 8, !dbg !93
  call void @free(ptr noundef %29) #7, !dbg !94
  ret void, !dbg !95
}

; Function Attrs: nounwind allocsize(0,1)
declare dso_local noalias ptr @calloc(i64 noundef, i64 noundef) #1

; Function Attrs: noreturn nounwind
declare dso_local void @__assert_fail(ptr noundef, ptr noundef, i32 noundef, ptr noundef) #2

; Function Attrs: nounwind
declare dso_local void @free(ptr noundef) #3

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_prepare_instruments() #0 !dbg !96 {
  call void @polybench_flush_cache(), !dbg !97
  ret void, !dbg !98
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_timer_start() #0 !dbg !99 {
  call void @polybench_prepare_instruments(), !dbg !100
  %1 = call double @rtclock(), !dbg !101
  store double %1, ptr @polybench_t_start, align 8, !dbg !102
  ret void, !dbg !103
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_timer_stop() #0 !dbg !104 {
  %1 = call double @rtclock(), !dbg !105
  store double %1, ptr @polybench_t_end, align 8, !dbg !106
  ret void, !dbg !107
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_timer_print() #0 !dbg !108 {
  %1 = load double, ptr @polybench_t_end, align 8, !dbg !109
  %2 = load double, ptr @polybench_t_start, align 8, !dbg !110
  %3 = fsub double %1, %2, !dbg !111
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.2, double noundef %3), !dbg !112
  ret void, !dbg !113
}

declare dso_local i32 @printf(ptr noundef, ...) #4

; Function Attrs: noinline nounwind uwtable
define dso_local void @polybench_free_data(ptr noundef %0) #0 !dbg !114 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
    #dbg_declare(ptr %2, !117, !DIExpression(), !118)
  %3 = load ptr, ptr %2, align 8, !dbg !119
  call void @free(ptr noundef %3) #7, !dbg !120
  ret void, !dbg !121
}

; Function Attrs: noinline nounwind uwtable
define dso_local ptr @polybench_alloc_data(i64 noundef %0, i32 noundef %1) #0 !dbg !122 {
  %3 = alloca i64, align 8
  %4 = alloca i32, align 4
  %5 = alloca i64, align 8
  %6 = alloca ptr, align 8
  store i64 %0, ptr %3, align 8
    #dbg_declare(ptr %3, !125, !DIExpression(), !126)
  store i32 %1, ptr %4, align 4
    #dbg_declare(ptr %4, !127, !DIExpression(), !128)
    #dbg_declare(ptr %5, !129, !DIExpression(), !130)
  %7 = load i64, ptr %3, align 8, !dbg !131
  store i64 %7, ptr %5, align 8, !dbg !130
  %8 = load i32, ptr %4, align 4, !dbg !132
  %9 = sext i32 %8 to i64, !dbg !132
  %10 = load i64, ptr %5, align 8, !dbg !133
  %11 = mul i64 %10, %9, !dbg !133
  store i64 %11, ptr %5, align 8, !dbg !133
    #dbg_declare(ptr %6, !134, !DIExpression(), !135)
  %12 = load i64, ptr %5, align 8, !dbg !136
  %13 = call ptr @xmalloc(i64 noundef %12), !dbg !137
  store ptr %13, ptr %6, align 8, !dbg !135
  %14 = load ptr, ptr %6, align 8, !dbg !138
  ret ptr %14, !dbg !139
}

; Function Attrs: noinline nounwind uwtable
define internal ptr @xmalloc(i64 noundef %0) #0 !dbg !140 {
  %2 = alloca i64, align 8
  %3 = alloca ptr, align 8
  %4 = alloca i64, align 8
  %5 = alloca i32, align 4
  store i64 %0, ptr %2, align 8
    #dbg_declare(ptr %2, !143, !DIExpression(), !144)
    #dbg_declare(ptr %3, !145, !DIExpression(), !146)
  store ptr null, ptr %3, align 8, !dbg !146
  %6 = load i64, ptr @polybench_inter_array_padding_sz, align 8, !dbg !147
  %7 = add i64 %6, 0, !dbg !147
  store i64 %7, ptr @polybench_inter_array_padding_sz, align 8, !dbg !147
    #dbg_declare(ptr %4, !148, !DIExpression(), !149)
  %8 = load i64, ptr %2, align 8, !dbg !150
  %9 = load i64, ptr @polybench_inter_array_padding_sz, align 8, !dbg !151
  %10 = add i64 %8, %9, !dbg !152
  store i64 %10, ptr %4, align 8, !dbg !149
    #dbg_declare(ptr %5, !153, !DIExpression(), !154)
  %11 = load i64, ptr %4, align 8, !dbg !155
  %12 = call i32 @posix_memalign(ptr noundef %3, i64 noundef 4096, i64 noundef %11) #7, !dbg !156
  store i32 %12, ptr %5, align 4, !dbg !154
  %13 = load ptr, ptr %3, align 8, !dbg !157
  %14 = icmp ne ptr %13, null, !dbg !157
  br i1 %14, label %15, label %18, !dbg !159

15:                                               ; preds = %1
  %16 = load i32, ptr %5, align 4, !dbg !160
  %17 = icmp ne i32 %16, 0, !dbg !160
  br i1 %17, label %18, label %21, !dbg !159

18:                                               ; preds = %15, %1
  %19 = load ptr, ptr @stderr, align 8, !dbg !161
  %20 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %19, ptr noundef @.str.3) #7, !dbg !163
  call void @exit(i32 noundef 1) #6, !dbg !164
  unreachable, !dbg !164

21:                                               ; preds = %15
  %22 = load ptr, ptr %3, align 8, !dbg !165
  ret ptr %22, !dbg !166
}

; Function Attrs: noinline nounwind uwtable
define internal double @rtclock() #0 !dbg !167 {
  ret double 0.000000e+00, !dbg !170
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

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!53, !54, !55, !56, !57}
!llvm.ident = !{!58}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "polybench_papi_counters_threadid", scope: !2, file: !3, line: 45, type: !52, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C11, file: !3, producer: "clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !4, globals: !8, splitDebugInlining: false, nameTableKind: None)
!3 = !DIFile(filename: "utilities/polybench.c", directory: "/g/g90/sharmin1/tutorial/FPChecker/experiments/benchmark/PolyBenchC-4.2.1")
!4 = !{!5, !7}
!5 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !6, size: 64)
!6 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!7 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!8 = !{!0, !9, !11, !17, !22, !28, !33, !35, !37, !40, !42, !47}
!9 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression())
!10 = distinct !DIGlobalVariable(name: "polybench_program_total_flops", scope: !2, file: !3, line: 46, type: !6, isLocal: false, isDefinition: true)
!11 = !DIGlobalVariableExpression(var: !12, expr: !DIExpression())
!12 = distinct !DIGlobalVariable(scope: null, file: !3, line: 123, type: !13, isLocal: true, isDefinition: true)
!13 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 96, elements: !15)
!14 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!15 = !{!16}
!16 = !DISubrange(count: 12)
!17 = !DIGlobalVariableExpression(var: !18, expr: !DIExpression())
!18 = distinct !DIGlobalVariable(scope: null, file: !3, line: 123, type: !19, isLocal: true, isDefinition: true)
!19 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 176, elements: !20)
!20 = !{!21}
!21 = !DISubrange(count: 22)
!22 = !DIGlobalVariableExpression(var: !23, expr: !DIExpression())
!23 = distinct !DIGlobalVariable(scope: null, file: !3, line: 123, type: !24, isLocal: true, isDefinition: true)
!24 = !DICompositeType(tag: DW_TAG_array_type, baseType: !25, size: 232, elements: !26)
!25 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !14)
!26 = !{!27}
!27 = !DISubrange(count: 29)
!28 = !DIGlobalVariableExpression(var: !29, expr: !DIExpression())
!29 = distinct !DIGlobalVariable(scope: null, file: !3, line: 402, type: !30, isLocal: true, isDefinition: true)
!30 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 56, elements: !31)
!31 = !{!32}
!32 = !DISubrange(count: 7)
!33 = !DIGlobalVariableExpression(var: !34, expr: !DIExpression())
!34 = distinct !DIGlobalVariable(name: "polybench_t_start", scope: !2, file: !3, line: 78, type: !6, isLocal: false, isDefinition: true)
!35 = !DIGlobalVariableExpression(var: !36, expr: !DIExpression())
!36 = distinct !DIGlobalVariable(name: "polybench_t_end", scope: !2, file: !3, line: 78, type: !6, isLocal: false, isDefinition: true)
!37 = !DIGlobalVariableExpression(var: !38, expr: !DIExpression())
!38 = distinct !DIGlobalVariable(name: "polybench_c_start", scope: !2, file: !3, line: 80, type: !39, isLocal: false, isDefinition: true)
!39 = !DIBasicType(name: "unsigned long long", size: 64, encoding: DW_ATE_unsigned)
!40 = !DIGlobalVariableExpression(var: !41, expr: !DIExpression())
!41 = distinct !DIGlobalVariable(name: "polybench_c_end", scope: !2, file: !3, line: 80, type: !39, isLocal: false, isDefinition: true)
!42 = !DIGlobalVariableExpression(var: !43, expr: !DIExpression())
!43 = distinct !DIGlobalVariable(scope: null, file: !3, line: 526, type: !44, isLocal: true, isDefinition: true)
!44 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 408, elements: !45)
!45 = !{!46}
!46 = !DISubrange(count: 51)
!47 = !DIGlobalVariableExpression(var: !48, expr: !DIExpression())
!48 = distinct !DIGlobalVariable(name: "polybench_inter_array_padding_sz", scope: !2, file: !3, line: 75, type: !49, isLocal: true, isDefinition: true)
!49 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !50, line: 18, baseType: !51)
!50 = !DIFile(filename: "/usr/bin/../lib/clang/21/include/__stddef_size_t.h", directory: "")
!51 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!52 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!53 = !{i32 7, !"Dwarf Version", i32 4}
!54 = !{i32 2, !"Debug Info Version", i32 3}
!55 = !{i32 1, !"wchar_size", i32 4}
!56 = !{i32 7, !"uwtable", i32 2}
!57 = !{i32 7, !"frame-pointer", i32 2}
!58 = !{!"clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)"}
!59 = distinct !DISubprogram(name: "polybench_flush_cache", scope: !3, file: !3, line: 112, type: !60, scopeLine: 113, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !62)
!60 = !DISubroutineType(types: !61)
!61 = !{null}
!62 = !{}
!63 = !DILocalVariable(name: "cs", scope: !59, file: !3, line: 114, type: !52)
!64 = !DILocation(line: 114, column: 7, scope: !59)
!65 = !DILocalVariable(name: "flush", scope: !59, file: !3, line: 115, type: !5)
!66 = !DILocation(line: 115, column: 11, scope: !59)
!67 = !DILocation(line: 115, column: 37, scope: !59)
!68 = !DILocation(line: 115, column: 29, scope: !59)
!69 = !DILocalVariable(name: "i", scope: !59, file: !3, line: 116, type: !52)
!70 = !DILocation(line: 116, column: 7, scope: !59)
!71 = !DILocalVariable(name: "tmp", scope: !59, file: !3, line: 117, type: !6)
!72 = !DILocation(line: 117, column: 10, scope: !59)
!73 = !DILocation(line: 121, column: 10, scope: !74)
!74 = distinct !DILexicalBlock(scope: !59, file: !3, line: 121, column: 3)
!75 = !DILocation(line: 121, column: 8, scope: !74)
!76 = !DILocation(line: 121, column: 15, scope: !77)
!77 = distinct !DILexicalBlock(scope: !74, file: !3, line: 121, column: 3)
!78 = !DILocation(line: 121, column: 19, scope: !77)
!79 = !DILocation(line: 121, column: 17, scope: !77)
!80 = !DILocation(line: 121, column: 3, scope: !74)
!81 = !DILocation(line: 122, column: 12, scope: !77)
!82 = !DILocation(line: 122, column: 18, scope: !77)
!83 = !DILocation(line: 122, column: 9, scope: !77)
!84 = !DILocation(line: 122, column: 5, scope: !77)
!85 = !DILocation(line: 121, column: 24, scope: !77)
!86 = !DILocation(line: 121, column: 3, scope: !77)
!87 = distinct !{!87, !80, !88, !89}
!88 = !DILocation(line: 122, column: 19, scope: !74)
!89 = !{!"llvm.loop.mustprogress"}
!90 = !DILocation(line: 123, column: 3, scope: !91)
!91 = distinct !DILexicalBlock(scope: !92, file: !3, line: 123, column: 3)
!92 = distinct !DILexicalBlock(scope: !59, file: !3, line: 123, column: 3)
!93 = !DILocation(line: 124, column: 9, scope: !59)
!94 = !DILocation(line: 124, column: 3, scope: !59)
!95 = !DILocation(line: 125, column: 1, scope: !59)
!96 = distinct !DISubprogram(name: "polybench_prepare_instruments", scope: !3, file: !3, line: 353, type: !60, scopeLine: 354, spFlags: DISPFlagDefinition, unit: !2)
!97 = !DILocation(line: 356, column: 3, scope: !96)
!98 = !DILocation(line: 361, column: 1, scope: !96)
!99 = distinct !DISubprogram(name: "polybench_timer_start", scope: !3, file: !3, line: 364, type: !60, scopeLine: 365, spFlags: DISPFlagDefinition, unit: !2)
!100 = !DILocation(line: 366, column: 3, scope: !99)
!101 = !DILocation(line: 368, column: 23, scope: !99)
!102 = !DILocation(line: 368, column: 21, scope: !99)
!103 = !DILocation(line: 372, column: 1, scope: !99)
!104 = distinct !DISubprogram(name: "polybench_timer_stop", scope: !3, file: !3, line: 375, type: !60, scopeLine: 376, spFlags: DISPFlagDefinition, unit: !2)
!105 = !DILocation(line: 378, column: 21, scope: !104)
!106 = !DILocation(line: 378, column: 19, scope: !104)
!107 = !DILocation(line: 385, column: 1, scope: !104)
!108 = distinct !DISubprogram(name: "polybench_timer_print", scope: !3, file: !3, line: 388, type: !60, scopeLine: 389, spFlags: DISPFlagDefinition, unit: !2)
!109 = !DILocation(line: 402, column: 26, scope: !108)
!110 = !DILocation(line: 402, column: 44, scope: !108)
!111 = !DILocation(line: 402, column: 42, scope: !108)
!112 = !DILocation(line: 402, column: 7, scope: !108)
!113 = !DILocation(line: 407, column: 1, scope: !108)
!114 = distinct !DISubprogram(name: "polybench_free_data", scope: !3, file: !3, line: 547, type: !115, scopeLine: 548, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !62)
!115 = !DISubroutineType(types: !116)
!116 = !{null, !7}
!117 = !DILocalVariable(name: "ptr", arg: 1, scope: !114, file: !3, line: 547, type: !7)
!118 = !DILocation(line: 547, column: 32, scope: !114)
!119 = !DILocation(line: 552, column: 9, scope: !114)
!120 = !DILocation(line: 552, column: 3, scope: !114)
!121 = !DILocation(line: 554, column: 1, scope: !114)
!122 = distinct !DISubprogram(name: "polybench_alloc_data", scope: !3, file: !3, line: 557, type: !123, scopeLine: 558, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !62)
!123 = !DISubroutineType(types: !124)
!124 = !{!7, !39, !52}
!125 = !DILocalVariable(name: "n", arg: 1, scope: !122, file: !3, line: 557, type: !39)
!126 = !DILocation(line: 557, column: 51, scope: !122)
!127 = !DILocalVariable(name: "elt_size", arg: 2, scope: !122, file: !3, line: 557, type: !52)
!128 = !DILocation(line: 557, column: 58, scope: !122)
!129 = !DILocalVariable(name: "val", scope: !122, file: !3, line: 564, type: !49)
!130 = !DILocation(line: 564, column: 10, scope: !122)
!131 = !DILocation(line: 564, column: 16, scope: !122)
!132 = !DILocation(line: 565, column: 10, scope: !122)
!133 = !DILocation(line: 565, column: 7, scope: !122)
!134 = !DILocalVariable(name: "ret", scope: !122, file: !3, line: 566, type: !7)
!135 = !DILocation(line: 566, column: 9, scope: !122)
!136 = !DILocation(line: 566, column: 24, scope: !122)
!137 = !DILocation(line: 566, column: 15, scope: !122)
!138 = !DILocation(line: 568, column: 10, scope: !122)
!139 = !DILocation(line: 568, column: 3, scope: !122)
!140 = distinct !DISubprogram(name: "xmalloc", scope: !3, file: !3, line: 517, type: !141, scopeLine: 518, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !62)
!141 = !DISubroutineType(types: !142)
!142 = !{!7, !49}
!143 = !DILocalVariable(name: "alloc_sz", arg: 1, scope: !140, file: !3, line: 517, type: !49)
!144 = !DILocation(line: 517, column: 16, scope: !140)
!145 = !DILocalVariable(name: "ret", scope: !140, file: !3, line: 519, type: !7)
!146 = !DILocation(line: 519, column: 9, scope: !140)
!147 = !DILocation(line: 521, column: 36, scope: !140)
!148 = !DILocalVariable(name: "padded_sz", scope: !140, file: !3, line: 522, type: !49)
!149 = !DILocation(line: 522, column: 10, scope: !140)
!150 = !DILocation(line: 522, column: 22, scope: !140)
!151 = !DILocation(line: 522, column: 33, scope: !140)
!152 = !DILocation(line: 522, column: 31, scope: !140)
!153 = !DILocalVariable(name: "err", scope: !140, file: !3, line: 523, type: !52)
!154 = !DILocation(line: 523, column: 7, scope: !140)
!155 = !DILocation(line: 523, column: 41, scope: !140)
!156 = !DILocation(line: 523, column: 13, scope: !140)
!157 = !DILocation(line: 524, column: 9, scope: !158)
!158 = distinct !DILexicalBlock(scope: !140, file: !3, line: 524, column: 7)
!159 = !DILocation(line: 524, column: 13, scope: !158)
!160 = !DILocation(line: 524, column: 16, scope: !158)
!161 = !DILocation(line: 526, column: 16, scope: !162)
!162 = distinct !DILexicalBlock(scope: !158, file: !3, line: 525, column: 5)
!163 = !DILocation(line: 526, column: 7, scope: !162)
!164 = !DILocation(line: 527, column: 7, scope: !162)
!165 = !DILocation(line: 543, column: 10, scope: !140)
!166 = !DILocation(line: 543, column: 3, scope: !140)
!167 = distinct !DISubprogram(name: "rtclock", scope: !3, file: !3, line: 83, type: !168, scopeLine: 84, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2)
!168 = !DISubroutineType(types: !169)
!169 = !{!6}
!170 = !DILocation(line: 93, column: 5, scope: !167)

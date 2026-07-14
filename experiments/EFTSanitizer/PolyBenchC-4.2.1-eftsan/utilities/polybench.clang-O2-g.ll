; ModuleID = 'utilities/polybench.c'
source_filename = "utilities/polybench.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@polybench_papi_counters_threadid = dso_local local_unnamed_addr global i32 0, align 4, !dbg !0
@polybench_program_total_flops = dso_local local_unnamed_addr global double 0.000000e+00, align 8, !dbg !9
@polybench_t_start = dso_local local_unnamed_addr global double 0.000000e+00, align 8, !dbg !33
@polybench_t_end = dso_local local_unnamed_addr global double 0.000000e+00, align 8, !dbg !35
@.str.2 = private unnamed_addr constant [7 x i8] c"%0.6f\0A\00", align 1, !dbg !28
@polybench_c_start = dso_local local_unnamed_addr global i64 0, align 8, !dbg !37
@polybench_c_end = dso_local local_unnamed_addr global i64 0, align 8, !dbg !40
@stderr = external dso_local local_unnamed_addr global ptr, align 8
@.str.3 = private unnamed_addr constant [51 x i8] c"[PolyBench] posix_memalign: cannot allocate memory\00", align 1, !dbg !42

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local void @polybench_flush_cache() local_unnamed_addr #0 !dbg !59 {
    #dbg_value(i32 4194560, !63, !DIExpression(), !67)
    #dbg_value(ptr poison, !64, !DIExpression(), !67)
    #dbg_value(double 0.000000e+00, !66, !DIExpression(), !67)
    #dbg_value(i32 0, !65, !DIExpression(), !67)
    #dbg_value(double 0.000000e+00, !66, !DIExpression(), !67)
    #dbg_value(i32 poison, !65, !DIExpression(), !67)
  ret void, !dbg !68
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr captures(none)) #1

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare !dbg !69 dso_local void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr captures(none)) #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local void @polybench_prepare_instruments() local_unnamed_addr #0 !dbg !73 {
  ret void, !dbg !74
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(write, argmem: none, inaccessiblemem: none) uwtable
define dso_local void @polybench_timer_start() local_unnamed_addr #3 !dbg !75 {
  store double 0.000000e+00, ptr @polybench_t_start, align 8, !dbg !76, !tbaa !77
  ret void, !dbg !81
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(write, argmem: none, inaccessiblemem: none) uwtable
define dso_local void @polybench_timer_stop() local_unnamed_addr #3 !dbg !82 {
  store double 0.000000e+00, ptr @polybench_t_end, align 8, !dbg !83, !tbaa !77
  ret void, !dbg !84
}

; Function Attrs: nofree nounwind uwtable
define dso_local void @polybench_timer_print() local_unnamed_addr #4 !dbg !85 {
  %1 = load double, ptr @polybench_t_end, align 8, !dbg !86, !tbaa !77
  %2 = load double, ptr @polybench_t_start, align 8, !dbg !87, !tbaa !77
  %3 = fsub double %1, %2, !dbg !88
  %4 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.2, double noundef %3), !dbg !89
  ret void, !dbg !90
}

; Function Attrs: nofree nounwind
declare !dbg !91 dso_local noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #5

; Function Attrs: mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite) uwtable
define dso_local void @polybench_free_data(ptr noundef captures(none) %0) local_unnamed_addr #6 !dbg !97 {
    #dbg_value(ptr %0, !99, !DIExpression(), !100)
  tail call void @free(ptr noundef %0) #9, !dbg !101
  ret void, !dbg !102
}

; Function Attrs: nofree nounwind uwtable
define dso_local nonnull ptr @polybench_alloc_data(i64 noundef %0, i32 noundef %1) local_unnamed_addr #4 !dbg !103 {
  %3 = alloca ptr, align 8, !DIAssignID !111
    #dbg_value(i64 %0, !107, !DIExpression(), !112)
    #dbg_value(i32 %1, !108, !DIExpression(), !112)
    #dbg_value(i64 %0, !109, !DIExpression(), !112)
  %4 = sext i32 %1 to i64, !dbg !113
  %5 = mul i64 %0, %4, !dbg !114
    #dbg_value(i64 %5, !109, !DIExpression(), !112)
    #dbg_assign(i1 poison, !115, !DIExpression(), !111, ptr %3, !DIExpression(), !123)
    #dbg_value(i64 %5, !120, !DIExpression(), !123)
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %3) #9, !dbg !125
  store ptr null, ptr %3, align 8, !dbg !126, !tbaa !127, !DIAssignID !129
    #dbg_assign(ptr null, !115, !DIExpression(), !129, ptr %3, !DIExpression(), !123)
    #dbg_value(i64 %5, !121, !DIExpression(), !123)
  %6 = call i32 @posix_memalign(ptr noundef nonnull %3, i64 noundef 4096, i64 noundef %5) #9, !dbg !130
    #dbg_value(i32 %6, !122, !DIExpression(), !123)
  %7 = load ptr, ptr %3, align 8, !dbg !131, !tbaa !127
  %8 = icmp eq ptr %7, null, !dbg !131
  %9 = icmp ne i32 %6, 0
  %10 = select i1 %8, i1 true, i1 %9, !dbg !133
  br i1 %10, label %11, label %14, !dbg !133

11:                                               ; preds = %2
  %12 = load ptr, ptr @stderr, align 8, !dbg !134, !tbaa !136
  %13 = call i64 @fwrite(ptr nonnull @.str.3, i64 50, i64 1, ptr %12) #10, !dbg !138
  call void @exit(i32 noundef 1) #11, !dbg !139
  unreachable, !dbg !139

14:                                               ; preds = %2
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %3) #9, !dbg !140
    #dbg_value(ptr %7, !110, !DIExpression(), !112)
  ret ptr %7, !dbg !141
}

; Function Attrs: nofree nounwind
declare !dbg !142 dso_local i32 @posix_memalign(ptr noundef, i64 noundef, i64 noundef) local_unnamed_addr #5

; Function Attrs: nofree noreturn nounwind
declare !dbg !146 dso_local void @exit(i32 noundef) local_unnamed_addr #7

; Function Attrs: nofree nounwind
declare noundef i64 @fwrite(ptr noundef readonly captures(none), i64 noundef, i64 noundef, ptr noundef captures(none)) local_unnamed_addr #8

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(write, argmem: none, inaccessiblemem: none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nofree nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { nofree nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { nofree noreturn nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #8 = { nofree nounwind }
attributes #9 = { nounwind }
attributes #10 = { cold }
attributes #11 = { cold noreturn nounwind }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!53, !54, !55, !56, !57}
!llvm.ident = !{!58}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "polybench_papi_counters_threadid", scope: !2, file: !3, line: 45, type: !52, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C11, file: !3, producer: "clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !4, globals: !8, splitDebugInlining: false, nameTableKind: None)
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
!47 = !DIGlobalVariableExpression(var: !48, expr: !DIExpression(DW_OP_constu, 0, DW_OP_stack_value))
!48 = distinct !DIGlobalVariable(name: "polybench_inter_array_padding_sz", scope: !2, file: !3, line: 75, type: !49, isLocal: true, isDefinition: true)
!49 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !50, line: 18, baseType: !51)
!50 = !DIFile(filename: "/usr/bin/../lib/clang/21/include/__stddef_size_t.h", directory: "")
!51 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!52 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!53 = !{i32 7, !"Dwarf Version", i32 4}
!54 = !{i32 2, !"Debug Info Version", i32 3}
!55 = !{i32 1, !"wchar_size", i32 4}
!56 = !{i32 7, !"uwtable", i32 2}
!57 = !{i32 7, !"debug-info-assignment-tracking", i1 true}
!58 = !{!"clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)"}
!59 = distinct !DISubprogram(name: "polybench_flush_cache", scope: !3, file: !3, line: 112, type: !60, scopeLine: 113, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !62)
!60 = !DISubroutineType(types: !61)
!61 = !{null}
!62 = !{!63, !64, !65, !66}
!63 = !DILocalVariable(name: "cs", scope: !59, file: !3, line: 114, type: !52)
!64 = !DILocalVariable(name: "flush", scope: !59, file: !3, line: 115, type: !5)
!65 = !DILocalVariable(name: "i", scope: !59, file: !3, line: 116, type: !52)
!66 = !DILocalVariable(name: "tmp", scope: !59, file: !3, line: 117, type: !6)
!67 = !DILocation(line: 0, scope: !59)
!68 = !DILocation(line: 125, column: 1, scope: !59)
!69 = !DISubprogram(name: "free", scope: !70, file: !70, line: 563, type: !71, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!70 = !DIFile(filename: "/usr/include/stdlib.h", directory: "")
!71 = !DISubroutineType(types: !72)
!72 = !{null, !7}
!73 = distinct !DISubprogram(name: "polybench_prepare_instruments", scope: !3, file: !3, line: 353, type: !60, scopeLine: 354, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2)
!74 = !DILocation(line: 361, column: 1, scope: !73)
!75 = distinct !DISubprogram(name: "polybench_timer_start", scope: !3, file: !3, line: 364, type: !60, scopeLine: 365, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2)
!76 = !DILocation(line: 368, column: 21, scope: !75)
!77 = !{!78, !78, i64 0}
!78 = !{!"double", !79, i64 0}
!79 = !{!"omnipotent char", !80, i64 0}
!80 = !{!"Simple C/C++ TBAA"}
!81 = !DILocation(line: 372, column: 1, scope: !75)
!82 = distinct !DISubprogram(name: "polybench_timer_stop", scope: !3, file: !3, line: 375, type: !60, scopeLine: 376, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2)
!83 = !DILocation(line: 378, column: 19, scope: !82)
!84 = !DILocation(line: 385, column: 1, scope: !82)
!85 = distinct !DISubprogram(name: "polybench_timer_print", scope: !3, file: !3, line: 388, type: !60, scopeLine: 389, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2)
!86 = !DILocation(line: 402, column: 26, scope: !85)
!87 = !DILocation(line: 402, column: 44, scope: !85)
!88 = !DILocation(line: 402, column: 42, scope: !85)
!89 = !DILocation(line: 402, column: 7, scope: !85)
!90 = !DILocation(line: 407, column: 1, scope: !85)
!91 = !DISubprogram(name: "printf", scope: !92, file: !92, line: 332, type: !93, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!92 = !DIFile(filename: "/usr/include/stdio.h", directory: "")
!93 = !DISubroutineType(types: !94)
!94 = !{!52, !95, null}
!95 = !DIDerivedType(tag: DW_TAG_restrict_type, baseType: !96)
!96 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !25, size: 64)
!97 = distinct !DISubprogram(name: "polybench_free_data", scope: !3, file: !3, line: 547, type: !71, scopeLine: 548, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !98)
!98 = !{!99}
!99 = !DILocalVariable(name: "ptr", arg: 1, scope: !97, file: !3, line: 547, type: !7)
!100 = !DILocation(line: 0, scope: !97)
!101 = !DILocation(line: 552, column: 3, scope: !97)
!102 = !DILocation(line: 554, column: 1, scope: !97)
!103 = distinct !DISubprogram(name: "polybench_alloc_data", scope: !3, file: !3, line: 557, type: !104, scopeLine: 558, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !106)
!104 = !DISubroutineType(types: !105)
!105 = !{!7, !39, !52}
!106 = !{!107, !108, !109, !110}
!107 = !DILocalVariable(name: "n", arg: 1, scope: !103, file: !3, line: 557, type: !39)
!108 = !DILocalVariable(name: "elt_size", arg: 2, scope: !103, file: !3, line: 557, type: !52)
!109 = !DILocalVariable(name: "val", scope: !103, file: !3, line: 564, type: !49)
!110 = !DILocalVariable(name: "ret", scope: !103, file: !3, line: 566, type: !7)
!111 = distinct !DIAssignID()
!112 = !DILocation(line: 0, scope: !103)
!113 = !DILocation(line: 565, column: 10, scope: !103)
!114 = !DILocation(line: 565, column: 7, scope: !103)
!115 = !DILocalVariable(name: "ret", scope: !116, file: !3, line: 519, type: !7)
!116 = distinct !DISubprogram(name: "xmalloc", scope: !3, file: !3, line: 517, type: !117, scopeLine: 518, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !119)
!117 = !DISubroutineType(types: !118)
!118 = !{!7, !49}
!119 = !{!120, !115, !121, !122}
!120 = !DILocalVariable(name: "alloc_sz", arg: 1, scope: !116, file: !3, line: 517, type: !49)
!121 = !DILocalVariable(name: "padded_sz", scope: !116, file: !3, line: 522, type: !49)
!122 = !DILocalVariable(name: "err", scope: !116, file: !3, line: 523, type: !52)
!123 = !DILocation(line: 0, scope: !116, inlinedAt: !124)
!124 = distinct !DILocation(line: 566, column: 15, scope: !103)
!125 = !DILocation(line: 519, column: 3, scope: !116, inlinedAt: !124)
!126 = !DILocation(line: 519, column: 9, scope: !116, inlinedAt: !124)
!127 = !{!128, !128, i64 0}
!128 = !{!"any pointer", !79, i64 0}
!129 = distinct !DIAssignID()
!130 = !DILocation(line: 523, column: 13, scope: !116, inlinedAt: !124)
!131 = !DILocation(line: 524, column: 9, scope: !132, inlinedAt: !124)
!132 = distinct !DILexicalBlock(scope: !116, file: !3, line: 524, column: 7)
!133 = !DILocation(line: 524, column: 13, scope: !132, inlinedAt: !124)
!134 = !DILocation(line: 526, column: 16, scope: !135, inlinedAt: !124)
!135 = distinct !DILexicalBlock(scope: !132, file: !3, line: 525, column: 5)
!136 = !{!137, !137, i64 0}
!137 = !{!"p1 _ZTS8_IO_FILE", !128, i64 0}
!138 = !DILocation(line: 526, column: 7, scope: !135, inlinedAt: !124)
!139 = !DILocation(line: 527, column: 7, scope: !135, inlinedAt: !124)
!140 = !DILocation(line: 544, column: 1, scope: !116, inlinedAt: !124)
!141 = !DILocation(line: 568, column: 3, scope: !103)
!142 = !DISubprogram(name: "posix_memalign", scope: !70, file: !70, line: 577, type: !143, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!143 = !DISubroutineType(types: !144)
!144 = !{!52, !145, !49, !49}
!145 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !7, size: 64)
!146 = !DISubprogram(name: "exit", scope: !70, file: !70, line: 614, type: !147, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: DISPFlagOptimized)
!147 = !DISubroutineType(types: !148)
!148 = !{null, !52}

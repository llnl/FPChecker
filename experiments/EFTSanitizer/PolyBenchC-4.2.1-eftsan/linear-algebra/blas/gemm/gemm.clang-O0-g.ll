; ModuleID = 'gemm.c'
source_filename = "gemm.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@stderr = external dso_local global ptr, align 8
@.str = private unnamed_addr constant [23 x i8] c"==BEGIN DUMP_ARRAYS==\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [15 x i8] c"begin dump: %s\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [3 x i8] c"C\0A\00", align 1, !dbg !12
@.str.3 = private unnamed_addr constant [22 x i8] c"Max value in C: %.7e\0A\00", align 1, !dbg !17
@.str.4 = private unnamed_addr constant [17 x i8] c"Norm of C: %.7e\0A\00", align 1, !dbg !22
@.str.5 = private unnamed_addr constant [30 x i8] c"Max value in C_double: %.17e\0A\00", align 1, !dbg !27
@.str.6 = private unnamed_addr constant [25 x i8] c"Norm of C_double: %.17e\0A\00", align 1, !dbg !32
@.str.7 = private unnamed_addr constant [19 x i8] c"Norm error: %.17e\0A\00", align 1, !dbg !37
@.str.8 = private unnamed_addr constant [17 x i8] c"\0Aend   dump: %s\0A\00", align 1, !dbg !42
@.str.9 = private unnamed_addr constant [2 x i8] c"C\00", align 1, !dbg !44
@.str.10 = private unnamed_addr constant [23 x i8] c"==END   DUMP_ARRAYS==\0A\00", align 1, !dbg !49

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 noundef %0, ptr noundef %1) #0 !dbg !79 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca ptr, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca float, align 4
  %10 = alloca float, align 4
  %11 = alloca double, align 8
  %12 = alloca double, align 8
  %13 = alloca ptr, align 8
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca ptr, align 8
  %18 = alloca ptr, align 8
  store i32 0, ptr %3, align 4
  store i32 %0, ptr %4, align 4
    #dbg_declare(ptr %4, !86, !DIExpression(), !87)
  store ptr %1, ptr %5, align 8
    #dbg_declare(ptr %5, !88, !DIExpression(), !89)
    #dbg_declare(ptr %6, !90, !DIExpression(), !91)
  store i32 20, ptr %6, align 4, !dbg !91
    #dbg_declare(ptr %7, !92, !DIExpression(), !93)
  store i32 25, ptr %7, align 4, !dbg !93
    #dbg_declare(ptr %8, !94, !DIExpression(), !95)
  store i32 30, ptr %8, align 4, !dbg !95
    #dbg_declare(ptr %9, !96, !DIExpression(), !97)
    #dbg_declare(ptr %10, !98, !DIExpression(), !99)
    #dbg_declare(ptr %11, !100, !DIExpression(), !101)
    #dbg_declare(ptr %12, !102, !DIExpression(), !103)
    #dbg_declare(ptr %13, !104, !DIExpression(), !105)
  %19 = call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 4), !dbg !105
  store ptr %19, ptr %13, align 8, !dbg !105
    #dbg_declare(ptr %14, !106, !DIExpression(), !107)
  %20 = call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 4), !dbg !107
  store ptr %20, ptr %14, align 8, !dbg !107
    #dbg_declare(ptr %15, !108, !DIExpression(), !109)
  %21 = call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 4), !dbg !109
  store ptr %21, ptr %15, align 8, !dbg !109
    #dbg_declare(ptr %16, !110, !DIExpression(), !111)
  %22 = call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 8), !dbg !111
  store ptr %22, ptr %16, align 8, !dbg !111
    #dbg_declare(ptr %17, !112, !DIExpression(), !113)
  %23 = call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 8), !dbg !113
  store ptr %23, ptr %17, align 8, !dbg !113
    #dbg_declare(ptr %18, !114, !DIExpression(), !115)
  %24 = call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 8), !dbg !115
  store ptr %24, ptr %18, align 8, !dbg !115
  %25 = load i32, ptr %6, align 4, !dbg !116
  %26 = load i32, ptr %7, align 4, !dbg !117
  %27 = load i32, ptr %8, align 4, !dbg !118
  %28 = load ptr, ptr %13, align 8, !dbg !119
  %29 = getelementptr inbounds [20 x [25 x float]], ptr %28, i64 0, i64 0, !dbg !119
  %30 = load ptr, ptr %14, align 8, !dbg !120
  %31 = getelementptr inbounds [20 x [30 x float]], ptr %30, i64 0, i64 0, !dbg !120
  %32 = load ptr, ptr %15, align 8, !dbg !121
  %33 = getelementptr inbounds [30 x [25 x float]], ptr %32, i64 0, i64 0, !dbg !121
  call void @init_array(i32 noundef %25, i32 noundef %26, i32 noundef %27, ptr noundef %9, ptr noundef %10, ptr noundef %29, ptr noundef %31, ptr noundef %33), !dbg !122
  %34 = load i32, ptr %6, align 4, !dbg !123
  %35 = load i32, ptr %7, align 4, !dbg !124
  %36 = load i32, ptr %8, align 4, !dbg !125
  %37 = load ptr, ptr %16, align 8, !dbg !126
  %38 = getelementptr inbounds [20 x [25 x double]], ptr %37, i64 0, i64 0, !dbg !126
  %39 = load ptr, ptr %17, align 8, !dbg !127
  %40 = getelementptr inbounds [20 x [30 x double]], ptr %39, i64 0, i64 0, !dbg !127
  %41 = load ptr, ptr %18, align 8, !dbg !128
  %42 = getelementptr inbounds [30 x [25 x double]], ptr %41, i64 0, i64 0, !dbg !128
  call void @init_array_double(i32 noundef %34, i32 noundef %35, i32 noundef %36, ptr noundef %11, ptr noundef %12, ptr noundef %38, ptr noundef %40, ptr noundef %42), !dbg !129
  %43 = load i32, ptr %6, align 4, !dbg !130
  %44 = load i32, ptr %7, align 4, !dbg !131
  %45 = load i32, ptr %8, align 4, !dbg !132
  %46 = load float, ptr %9, align 4, !dbg !133
  %47 = load float, ptr %10, align 4, !dbg !134
  %48 = load ptr, ptr %13, align 8, !dbg !135
  %49 = getelementptr inbounds [20 x [25 x float]], ptr %48, i64 0, i64 0, !dbg !135
  %50 = load ptr, ptr %14, align 8, !dbg !136
  %51 = getelementptr inbounds [20 x [30 x float]], ptr %50, i64 0, i64 0, !dbg !136
  %52 = load ptr, ptr %15, align 8, !dbg !137
  %53 = getelementptr inbounds [30 x [25 x float]], ptr %52, i64 0, i64 0, !dbg !137
  call void @kernel_gemm(i32 noundef %43, i32 noundef %44, i32 noundef %45, float noundef %46, float noundef %47, ptr noundef %49, ptr noundef %51, ptr noundef %53), !dbg !138
  %54 = load i32, ptr %6, align 4, !dbg !139
  %55 = load i32, ptr %7, align 4, !dbg !140
  %56 = load i32, ptr %8, align 4, !dbg !141
  %57 = load double, ptr %11, align 8, !dbg !142
  %58 = load double, ptr %12, align 8, !dbg !143
  %59 = load ptr, ptr %16, align 8, !dbg !144
  %60 = getelementptr inbounds [20 x [25 x double]], ptr %59, i64 0, i64 0, !dbg !144
  %61 = load ptr, ptr %17, align 8, !dbg !145
  %62 = getelementptr inbounds [20 x [30 x double]], ptr %61, i64 0, i64 0, !dbg !145
  %63 = load ptr, ptr %18, align 8, !dbg !146
  %64 = getelementptr inbounds [30 x [25 x double]], ptr %63, i64 0, i64 0, !dbg !146
  call void @kernel_gemm_double(i32 noundef %54, i32 noundef %55, i32 noundef %56, double noundef %57, double noundef %58, ptr noundef %60, ptr noundef %62, ptr noundef %64), !dbg !147
  %65 = load i32, ptr %6, align 4, !dbg !148
  %66 = load i32, ptr %7, align 4, !dbg !148
  %67 = load ptr, ptr %13, align 8, !dbg !148
  %68 = getelementptr inbounds [20 x [25 x float]], ptr %67, i64 0, i64 0, !dbg !148
  %69 = load ptr, ptr %16, align 8, !dbg !148
  %70 = getelementptr inbounds [20 x [25 x double]], ptr %69, i64 0, i64 0, !dbg !148
  call void @print_array(i32 noundef %65, i32 noundef %66, ptr noundef %68, ptr noundef %70), !dbg !148
  %71 = load ptr, ptr %13, align 8, !dbg !149
  call void @free(ptr noundef %71) #4, !dbg !149
  %72 = load ptr, ptr %14, align 8, !dbg !150
  call void @free(ptr noundef %72) #4, !dbg !150
  %73 = load ptr, ptr %15, align 8, !dbg !151
  call void @free(ptr noundef %73) #4, !dbg !151
  %74 = load ptr, ptr %16, align 8, !dbg !152
  call void @free(ptr noundef %74) #4, !dbg !152
  %75 = load ptr, ptr %17, align 8, !dbg !153
  call void @free(ptr noundef %75) #4, !dbg !153
  %76 = load ptr, ptr %18, align 8, !dbg !154
  call void @free(ptr noundef %76) #4, !dbg !154
  ret i32 0, !dbg !155
}

declare dso_local ptr @polybench_alloc_data(i64 noundef, i32 noundef) #1

; Function Attrs: noinline nounwind uwtable
define internal void @init_array(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr noundef %3, ptr noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !156 {
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca ptr, align 8
  %13 = alloca ptr, align 8
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca i32, align 4
  %18 = alloca i32, align 4
  store i32 %0, ptr %9, align 4
    #dbg_declare(ptr %9, !164, !DIExpression(), !165)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !166, !DIExpression(), !167)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !168, !DIExpression(), !169)
  store ptr %3, ptr %12, align 8
    #dbg_declare(ptr %12, !170, !DIExpression(), !171)
  store ptr %4, ptr %13, align 8
    #dbg_declare(ptr %13, !172, !DIExpression(), !173)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !174, !DIExpression(), !175)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !176, !DIExpression(), !177)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !178, !DIExpression(), !179)
    #dbg_declare(ptr %17, !180, !DIExpression(), !181)
    #dbg_declare(ptr %18, !182, !DIExpression(), !183)
  %19 = load ptr, ptr %12, align 8, !dbg !184
  store float 1.500000e+00, ptr %19, align 4, !dbg !185
  %20 = load ptr, ptr %13, align 8, !dbg !186
  store float 0x3FF3333340000000, ptr %20, align 4, !dbg !187
  store i32 0, ptr %17, align 4, !dbg !188
  br label %21, !dbg !190

21:                                               ; preds = %52, %8
  %22 = load i32, ptr %17, align 4, !dbg !191
  %23 = load i32, ptr %9, align 4, !dbg !193
  %24 = icmp slt i32 %22, %23, !dbg !194
  br i1 %24, label %25, label %55, !dbg !195

25:                                               ; preds = %21
  store i32 0, ptr %18, align 4, !dbg !196
  br label %26, !dbg !198

26:                                               ; preds = %48, %25
  %27 = load i32, ptr %18, align 4, !dbg !199
  %28 = load i32, ptr %10, align 4, !dbg !201
  %29 = icmp slt i32 %27, %28, !dbg !202
  br i1 %29, label %30, label %51, !dbg !203

30:                                               ; preds = %26
  %31 = load i32, ptr %17, align 4, !dbg !204
  %32 = load i32, ptr %18, align 4, !dbg !205
  %33 = mul nsw i32 %31, %32, !dbg !206
  %34 = add nsw i32 %33, 1, !dbg !207
  %35 = load i32, ptr %9, align 4, !dbg !208
  %36 = srem i32 %34, %35, !dbg !209
  %37 = sitofp i32 %36 to float, !dbg !210
  %38 = load i32, ptr %9, align 4, !dbg !211
  %39 = sitofp i32 %38 to float, !dbg !211
  %40 = fdiv float %37, %39, !dbg !212
  %41 = load ptr, ptr %14, align 8, !dbg !213
  %42 = load i32, ptr %17, align 4, !dbg !214
  %43 = sext i32 %42 to i64, !dbg !213
  %44 = getelementptr inbounds [25 x float], ptr %41, i64 %43, !dbg !213
  %45 = load i32, ptr %18, align 4, !dbg !215
  %46 = sext i32 %45 to i64, !dbg !213
  %47 = getelementptr inbounds [25 x float], ptr %44, i64 0, i64 %46, !dbg !213
  store float %40, ptr %47, align 4, !dbg !216
  br label %48, !dbg !213

48:                                               ; preds = %30
  %49 = load i32, ptr %18, align 4, !dbg !217
  %50 = add nsw i32 %49, 1, !dbg !217
  store i32 %50, ptr %18, align 4, !dbg !217
  br label %26, !dbg !218, !llvm.loop !219

51:                                               ; preds = %26
  br label %52, !dbg !220

52:                                               ; preds = %51
  %53 = load i32, ptr %17, align 4, !dbg !222
  %54 = add nsw i32 %53, 1, !dbg !222
  store i32 %54, ptr %17, align 4, !dbg !222
  br label %21, !dbg !223, !llvm.loop !224

55:                                               ; preds = %21
  store i32 0, ptr %17, align 4, !dbg !226
  br label %56, !dbg !228

56:                                               ; preds = %87, %55
  %57 = load i32, ptr %17, align 4, !dbg !229
  %58 = load i32, ptr %9, align 4, !dbg !231
  %59 = icmp slt i32 %57, %58, !dbg !232
  br i1 %59, label %60, label %90, !dbg !233

60:                                               ; preds = %56
  store i32 0, ptr %18, align 4, !dbg !234
  br label %61, !dbg !236

61:                                               ; preds = %83, %60
  %62 = load i32, ptr %18, align 4, !dbg !237
  %63 = load i32, ptr %11, align 4, !dbg !239
  %64 = icmp slt i32 %62, %63, !dbg !240
  br i1 %64, label %65, label %86, !dbg !241

65:                                               ; preds = %61
  %66 = load i32, ptr %17, align 4, !dbg !242
  %67 = load i32, ptr %18, align 4, !dbg !243
  %68 = add nsw i32 %67, 1, !dbg !244
  %69 = mul nsw i32 %66, %68, !dbg !245
  %70 = load i32, ptr %11, align 4, !dbg !246
  %71 = srem i32 %69, %70, !dbg !247
  %72 = sitofp i32 %71 to float, !dbg !248
  %73 = load i32, ptr %11, align 4, !dbg !249
  %74 = sitofp i32 %73 to float, !dbg !249
  %75 = fdiv float %72, %74, !dbg !250
  %76 = load ptr, ptr %15, align 8, !dbg !251
  %77 = load i32, ptr %17, align 4, !dbg !252
  %78 = sext i32 %77 to i64, !dbg !251
  %79 = getelementptr inbounds [30 x float], ptr %76, i64 %78, !dbg !251
  %80 = load i32, ptr %18, align 4, !dbg !253
  %81 = sext i32 %80 to i64, !dbg !251
  %82 = getelementptr inbounds [30 x float], ptr %79, i64 0, i64 %81, !dbg !251
  store float %75, ptr %82, align 4, !dbg !254
  br label %83, !dbg !251

83:                                               ; preds = %65
  %84 = load i32, ptr %18, align 4, !dbg !255
  %85 = add nsw i32 %84, 1, !dbg !255
  store i32 %85, ptr %18, align 4, !dbg !255
  br label %61, !dbg !256, !llvm.loop !257

86:                                               ; preds = %61
  br label %87, !dbg !258

87:                                               ; preds = %86
  %88 = load i32, ptr %17, align 4, !dbg !259
  %89 = add nsw i32 %88, 1, !dbg !259
  store i32 %89, ptr %17, align 4, !dbg !259
  br label %56, !dbg !260, !llvm.loop !261

90:                                               ; preds = %56
  store i32 0, ptr %17, align 4, !dbg !263
  br label %91, !dbg !265

91:                                               ; preds = %122, %90
  %92 = load i32, ptr %17, align 4, !dbg !266
  %93 = load i32, ptr %11, align 4, !dbg !268
  %94 = icmp slt i32 %92, %93, !dbg !269
  br i1 %94, label %95, label %125, !dbg !270

95:                                               ; preds = %91
  store i32 0, ptr %18, align 4, !dbg !271
  br label %96, !dbg !273

96:                                               ; preds = %118, %95
  %97 = load i32, ptr %18, align 4, !dbg !274
  %98 = load i32, ptr %10, align 4, !dbg !276
  %99 = icmp slt i32 %97, %98, !dbg !277
  br i1 %99, label %100, label %121, !dbg !278

100:                                              ; preds = %96
  %101 = load i32, ptr %17, align 4, !dbg !279
  %102 = load i32, ptr %18, align 4, !dbg !280
  %103 = add nsw i32 %102, 2, !dbg !281
  %104 = mul nsw i32 %101, %103, !dbg !282
  %105 = load i32, ptr %10, align 4, !dbg !283
  %106 = srem i32 %104, %105, !dbg !284
  %107 = sitofp i32 %106 to float, !dbg !285
  %108 = load i32, ptr %10, align 4, !dbg !286
  %109 = sitofp i32 %108 to float, !dbg !286
  %110 = fdiv float %107, %109, !dbg !287
  %111 = load ptr, ptr %16, align 8, !dbg !288
  %112 = load i32, ptr %17, align 4, !dbg !289
  %113 = sext i32 %112 to i64, !dbg !288
  %114 = getelementptr inbounds [25 x float], ptr %111, i64 %113, !dbg !288
  %115 = load i32, ptr %18, align 4, !dbg !290
  %116 = sext i32 %115 to i64, !dbg !288
  %117 = getelementptr inbounds [25 x float], ptr %114, i64 0, i64 %116, !dbg !288
  store float %110, ptr %117, align 4, !dbg !291
  br label %118, !dbg !288

118:                                              ; preds = %100
  %119 = load i32, ptr %18, align 4, !dbg !292
  %120 = add nsw i32 %119, 1, !dbg !292
  store i32 %120, ptr %18, align 4, !dbg !292
  br label %96, !dbg !293, !llvm.loop !294

121:                                              ; preds = %96
  br label %122, !dbg !295

122:                                              ; preds = %121
  %123 = load i32, ptr %17, align 4, !dbg !296
  %124 = add nsw i32 %123, 1, !dbg !296
  store i32 %124, ptr %17, align 4, !dbg !296
  br label %91, !dbg !297, !llvm.loop !298

125:                                              ; preds = %91
  ret void, !dbg !300
}

; Function Attrs: noinline nounwind uwtable
define internal void @init_array_double(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr noundef %3, ptr noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !301 {
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca ptr, align 8
  %13 = alloca ptr, align 8
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca i32, align 4
  %18 = alloca i32, align 4
  store i32 %0, ptr %9, align 4
    #dbg_declare(ptr %9, !309, !DIExpression(), !310)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !311, !DIExpression(), !312)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !313, !DIExpression(), !314)
  store ptr %3, ptr %12, align 8
    #dbg_declare(ptr %12, !315, !DIExpression(), !316)
  store ptr %4, ptr %13, align 8
    #dbg_declare(ptr %13, !317, !DIExpression(), !318)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !319, !DIExpression(), !320)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !321, !DIExpression(), !322)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !323, !DIExpression(), !324)
    #dbg_declare(ptr %17, !325, !DIExpression(), !326)
    #dbg_declare(ptr %18, !327, !DIExpression(), !328)
  %19 = load ptr, ptr %12, align 8, !dbg !329
  store double 1.500000e+00, ptr %19, align 8, !dbg !330
  %20 = load ptr, ptr %13, align 8, !dbg !331
  store double 1.200000e+00, ptr %20, align 8, !dbg !332
  store i32 0, ptr %17, align 4, !dbg !333
  br label %21, !dbg !335

21:                                               ; preds = %52, %8
  %22 = load i32, ptr %17, align 4, !dbg !336
  %23 = load i32, ptr %9, align 4, !dbg !338
  %24 = icmp slt i32 %22, %23, !dbg !339
  br i1 %24, label %25, label %55, !dbg !340

25:                                               ; preds = %21
  store i32 0, ptr %18, align 4, !dbg !341
  br label %26, !dbg !343

26:                                               ; preds = %48, %25
  %27 = load i32, ptr %18, align 4, !dbg !344
  %28 = load i32, ptr %10, align 4, !dbg !346
  %29 = icmp slt i32 %27, %28, !dbg !347
  br i1 %29, label %30, label %51, !dbg !348

30:                                               ; preds = %26
  %31 = load i32, ptr %17, align 4, !dbg !349
  %32 = load i32, ptr %18, align 4, !dbg !350
  %33 = mul nsw i32 %31, %32, !dbg !351
  %34 = add nsw i32 %33, 1, !dbg !352
  %35 = load i32, ptr %9, align 4, !dbg !353
  %36 = srem i32 %34, %35, !dbg !354
  %37 = sitofp i32 %36 to double, !dbg !355
  %38 = load i32, ptr %9, align 4, !dbg !356
  %39 = sitofp i32 %38 to double, !dbg !356
  %40 = fdiv double %37, %39, !dbg !357
  %41 = load ptr, ptr %14, align 8, !dbg !358
  %42 = load i32, ptr %17, align 4, !dbg !359
  %43 = sext i32 %42 to i64, !dbg !358
  %44 = getelementptr inbounds [25 x double], ptr %41, i64 %43, !dbg !358
  %45 = load i32, ptr %18, align 4, !dbg !360
  %46 = sext i32 %45 to i64, !dbg !358
  %47 = getelementptr inbounds [25 x double], ptr %44, i64 0, i64 %46, !dbg !358
  store double %40, ptr %47, align 8, !dbg !361
  br label %48, !dbg !358

48:                                               ; preds = %30
  %49 = load i32, ptr %18, align 4, !dbg !362
  %50 = add nsw i32 %49, 1, !dbg !362
  store i32 %50, ptr %18, align 4, !dbg !362
  br label %26, !dbg !363, !llvm.loop !364

51:                                               ; preds = %26
  br label %52, !dbg !365

52:                                               ; preds = %51
  %53 = load i32, ptr %17, align 4, !dbg !366
  %54 = add nsw i32 %53, 1, !dbg !366
  store i32 %54, ptr %17, align 4, !dbg !366
  br label %21, !dbg !367, !llvm.loop !368

55:                                               ; preds = %21
  store i32 0, ptr %17, align 4, !dbg !370
  br label %56, !dbg !372

56:                                               ; preds = %87, %55
  %57 = load i32, ptr %17, align 4, !dbg !373
  %58 = load i32, ptr %9, align 4, !dbg !375
  %59 = icmp slt i32 %57, %58, !dbg !376
  br i1 %59, label %60, label %90, !dbg !377

60:                                               ; preds = %56
  store i32 0, ptr %18, align 4, !dbg !378
  br label %61, !dbg !380

61:                                               ; preds = %83, %60
  %62 = load i32, ptr %18, align 4, !dbg !381
  %63 = load i32, ptr %11, align 4, !dbg !383
  %64 = icmp slt i32 %62, %63, !dbg !384
  br i1 %64, label %65, label %86, !dbg !385

65:                                               ; preds = %61
  %66 = load i32, ptr %17, align 4, !dbg !386
  %67 = load i32, ptr %18, align 4, !dbg !387
  %68 = add nsw i32 %67, 1, !dbg !388
  %69 = mul nsw i32 %66, %68, !dbg !389
  %70 = load i32, ptr %11, align 4, !dbg !390
  %71 = srem i32 %69, %70, !dbg !391
  %72 = sitofp i32 %71 to double, !dbg !392
  %73 = load i32, ptr %11, align 4, !dbg !393
  %74 = sitofp i32 %73 to double, !dbg !393
  %75 = fdiv double %72, %74, !dbg !394
  %76 = load ptr, ptr %15, align 8, !dbg !395
  %77 = load i32, ptr %17, align 4, !dbg !396
  %78 = sext i32 %77 to i64, !dbg !395
  %79 = getelementptr inbounds [30 x double], ptr %76, i64 %78, !dbg !395
  %80 = load i32, ptr %18, align 4, !dbg !397
  %81 = sext i32 %80 to i64, !dbg !395
  %82 = getelementptr inbounds [30 x double], ptr %79, i64 0, i64 %81, !dbg !395
  store double %75, ptr %82, align 8, !dbg !398
  br label %83, !dbg !395

83:                                               ; preds = %65
  %84 = load i32, ptr %18, align 4, !dbg !399
  %85 = add nsw i32 %84, 1, !dbg !399
  store i32 %85, ptr %18, align 4, !dbg !399
  br label %61, !dbg !400, !llvm.loop !401

86:                                               ; preds = %61
  br label %87, !dbg !402

87:                                               ; preds = %86
  %88 = load i32, ptr %17, align 4, !dbg !403
  %89 = add nsw i32 %88, 1, !dbg !403
  store i32 %89, ptr %17, align 4, !dbg !403
  br label %56, !dbg !404, !llvm.loop !405

90:                                               ; preds = %56
  store i32 0, ptr %17, align 4, !dbg !407
  br label %91, !dbg !409

91:                                               ; preds = %122, %90
  %92 = load i32, ptr %17, align 4, !dbg !410
  %93 = load i32, ptr %11, align 4, !dbg !412
  %94 = icmp slt i32 %92, %93, !dbg !413
  br i1 %94, label %95, label %125, !dbg !414

95:                                               ; preds = %91
  store i32 0, ptr %18, align 4, !dbg !415
  br label %96, !dbg !417

96:                                               ; preds = %118, %95
  %97 = load i32, ptr %18, align 4, !dbg !418
  %98 = load i32, ptr %10, align 4, !dbg !420
  %99 = icmp slt i32 %97, %98, !dbg !421
  br i1 %99, label %100, label %121, !dbg !422

100:                                              ; preds = %96
  %101 = load i32, ptr %17, align 4, !dbg !423
  %102 = load i32, ptr %18, align 4, !dbg !424
  %103 = add nsw i32 %102, 2, !dbg !425
  %104 = mul nsw i32 %101, %103, !dbg !426
  %105 = load i32, ptr %10, align 4, !dbg !427
  %106 = srem i32 %104, %105, !dbg !428
  %107 = sitofp i32 %106 to double, !dbg !429
  %108 = load i32, ptr %10, align 4, !dbg !430
  %109 = sitofp i32 %108 to double, !dbg !430
  %110 = fdiv double %107, %109, !dbg !431
  %111 = load ptr, ptr %16, align 8, !dbg !432
  %112 = load i32, ptr %17, align 4, !dbg !433
  %113 = sext i32 %112 to i64, !dbg !432
  %114 = getelementptr inbounds [25 x double], ptr %111, i64 %113, !dbg !432
  %115 = load i32, ptr %18, align 4, !dbg !434
  %116 = sext i32 %115 to i64, !dbg !432
  %117 = getelementptr inbounds [25 x double], ptr %114, i64 0, i64 %116, !dbg !432
  store double %110, ptr %117, align 8, !dbg !435
  br label %118, !dbg !432

118:                                              ; preds = %100
  %119 = load i32, ptr %18, align 4, !dbg !436
  %120 = add nsw i32 %119, 1, !dbg !436
  store i32 %120, ptr %18, align 4, !dbg !436
  br label %96, !dbg !437, !llvm.loop !438

121:                                              ; preds = %96
  br label %122, !dbg !439

122:                                              ; preds = %121
  %123 = load i32, ptr %17, align 4, !dbg !440
  %124 = add nsw i32 %123, 1, !dbg !440
  store i32 %124, ptr %17, align 4, !dbg !440
  br label %91, !dbg !441, !llvm.loop !442

125:                                              ; preds = %91
  ret void, !dbg !444
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemm(i32 noundef %0, i32 noundef %1, i32 noundef %2, float noundef %3, float noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !445 {
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca float, align 4
  %13 = alloca float, align 4
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca i32, align 4
  %18 = alloca i32, align 4
  %19 = alloca i32, align 4
  store i32 %0, ptr %9, align 4
    #dbg_declare(ptr %9, !448, !DIExpression(), !449)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !450, !DIExpression(), !451)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !452, !DIExpression(), !453)
  store float %3, ptr %12, align 4
    #dbg_declare(ptr %12, !454, !DIExpression(), !455)
  store float %4, ptr %13, align 4
    #dbg_declare(ptr %13, !456, !DIExpression(), !457)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !458, !DIExpression(), !459)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !460, !DIExpression(), !461)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !462, !DIExpression(), !463)
    #dbg_declare(ptr %17, !464, !DIExpression(), !465)
    #dbg_declare(ptr %18, !466, !DIExpression(), !467)
    #dbg_declare(ptr %19, !468, !DIExpression(), !469)
  store i32 0, ptr %17, align 4, !dbg !470
  br label %20, !dbg !472

20:                                               ; preds = %89, %8
  %21 = load i32, ptr %17, align 4, !dbg !473
  %22 = load i32, ptr %9, align 4, !dbg !475
  %23 = icmp slt i32 %21, %22, !dbg !476
  br i1 %23, label %24, label %92, !dbg !477

24:                                               ; preds = %20
  store i32 0, ptr %18, align 4, !dbg !478
  br label %25, !dbg !481

25:                                               ; preds = %40, %24
  %26 = load i32, ptr %18, align 4, !dbg !482
  %27 = load i32, ptr %10, align 4, !dbg !484
  %28 = icmp slt i32 %26, %27, !dbg !485
  br i1 %28, label %29, label %43, !dbg !486

29:                                               ; preds = %25
  %30 = load float, ptr %13, align 4, !dbg !487
  %31 = load ptr, ptr %14, align 8, !dbg !488
  %32 = load i32, ptr %17, align 4, !dbg !489
  %33 = sext i32 %32 to i64, !dbg !488
  %34 = getelementptr inbounds [25 x float], ptr %31, i64 %33, !dbg !488
  %35 = load i32, ptr %18, align 4, !dbg !490
  %36 = sext i32 %35 to i64, !dbg !488
  %37 = getelementptr inbounds [25 x float], ptr %34, i64 0, i64 %36, !dbg !488
  %38 = load float, ptr %37, align 4, !dbg !491
  %39 = fmul float %38, %30, !dbg !491
  store float %39, ptr %37, align 4, !dbg !491
  br label %40, !dbg !488

40:                                               ; preds = %29
  %41 = load i32, ptr %18, align 4, !dbg !492
  %42 = add nsw i32 %41, 1, !dbg !492
  store i32 %42, ptr %18, align 4, !dbg !492
  br label %25, !dbg !493, !llvm.loop !494

43:                                               ; preds = %25
  store i32 0, ptr %19, align 4, !dbg !496
  br label %44, !dbg !498

44:                                               ; preds = %85, %43
  %45 = load i32, ptr %19, align 4, !dbg !499
  %46 = load i32, ptr %11, align 4, !dbg !501
  %47 = icmp slt i32 %45, %46, !dbg !502
  br i1 %47, label %48, label %88, !dbg !503

48:                                               ; preds = %44
  store i32 0, ptr %18, align 4, !dbg !504
  br label %49, !dbg !507

49:                                               ; preds = %81, %48
  %50 = load i32, ptr %18, align 4, !dbg !508
  %51 = load i32, ptr %10, align 4, !dbg !510
  %52 = icmp slt i32 %50, %51, !dbg !511
  br i1 %52, label %53, label %84, !dbg !512

53:                                               ; preds = %49
  %54 = load float, ptr %12, align 4, !dbg !513
  %55 = load ptr, ptr %15, align 8, !dbg !514
  %56 = load i32, ptr %17, align 4, !dbg !515
  %57 = sext i32 %56 to i64, !dbg !514
  %58 = getelementptr inbounds [30 x float], ptr %55, i64 %57, !dbg !514
  %59 = load i32, ptr %19, align 4, !dbg !516
  %60 = sext i32 %59 to i64, !dbg !514
  %61 = getelementptr inbounds [30 x float], ptr %58, i64 0, i64 %60, !dbg !514
  %62 = load float, ptr %61, align 4, !dbg !514
  %63 = fmul float %54, %62, !dbg !517
  %64 = load ptr, ptr %16, align 8, !dbg !518
  %65 = load i32, ptr %19, align 4, !dbg !519
  %66 = sext i32 %65 to i64, !dbg !518
  %67 = getelementptr inbounds [25 x float], ptr %64, i64 %66, !dbg !518
  %68 = load i32, ptr %18, align 4, !dbg !520
  %69 = sext i32 %68 to i64, !dbg !518
  %70 = getelementptr inbounds [25 x float], ptr %67, i64 0, i64 %69, !dbg !518
  %71 = load float, ptr %70, align 4, !dbg !518
  %72 = load ptr, ptr %14, align 8, !dbg !521
  %73 = load i32, ptr %17, align 4, !dbg !522
  %74 = sext i32 %73 to i64, !dbg !521
  %75 = getelementptr inbounds [25 x float], ptr %72, i64 %74, !dbg !521
  %76 = load i32, ptr %18, align 4, !dbg !523
  %77 = sext i32 %76 to i64, !dbg !521
  %78 = getelementptr inbounds [25 x float], ptr %75, i64 0, i64 %77, !dbg !521
  %79 = load float, ptr %78, align 4, !dbg !524
  %80 = call float @llvm.fmuladd.f32(float %63, float %71, float %79), !dbg !524
  store float %80, ptr %78, align 4, !dbg !524
  br label %81, !dbg !521

81:                                               ; preds = %53
  %82 = load i32, ptr %18, align 4, !dbg !525
  %83 = add nsw i32 %82, 1, !dbg !525
  store i32 %83, ptr %18, align 4, !dbg !525
  br label %49, !dbg !526, !llvm.loop !527

84:                                               ; preds = %49
  br label %85, !dbg !529

85:                                               ; preds = %84
  %86 = load i32, ptr %19, align 4, !dbg !530
  %87 = add nsw i32 %86, 1, !dbg !530
  store i32 %87, ptr %19, align 4, !dbg !530
  br label %44, !dbg !531, !llvm.loop !532

88:                                               ; preds = %44
  br label %89, !dbg !534

89:                                               ; preds = %88
  %90 = load i32, ptr %17, align 4, !dbg !535
  %91 = add nsw i32 %90, 1, !dbg !535
  store i32 %91, ptr %17, align 4, !dbg !535
  br label %20, !dbg !536, !llvm.loop !537

92:                                               ; preds = %20
  ret void, !dbg !539
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemm_double(i32 noundef %0, i32 noundef %1, i32 noundef %2, double noundef %3, double noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !540 {
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca double, align 8
  %13 = alloca double, align 8
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca i32, align 4
  %18 = alloca i32, align 4
  %19 = alloca i32, align 4
  store i32 %0, ptr %9, align 4
    #dbg_declare(ptr %9, !543, !DIExpression(), !544)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !545, !DIExpression(), !546)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !547, !DIExpression(), !548)
  store double %3, ptr %12, align 8
    #dbg_declare(ptr %12, !549, !DIExpression(), !550)
  store double %4, ptr %13, align 8
    #dbg_declare(ptr %13, !551, !DIExpression(), !552)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !553, !DIExpression(), !554)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !555, !DIExpression(), !556)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !557, !DIExpression(), !558)
    #dbg_declare(ptr %17, !559, !DIExpression(), !560)
    #dbg_declare(ptr %18, !561, !DIExpression(), !562)
    #dbg_declare(ptr %19, !563, !DIExpression(), !564)
  store i32 0, ptr %17, align 4, !dbg !565
  br label %20, !dbg !567

20:                                               ; preds = %89, %8
  %21 = load i32, ptr %17, align 4, !dbg !568
  %22 = load i32, ptr %9, align 4, !dbg !570
  %23 = icmp slt i32 %21, %22, !dbg !571
  br i1 %23, label %24, label %92, !dbg !572

24:                                               ; preds = %20
  store i32 0, ptr %18, align 4, !dbg !573
  br label %25, !dbg !576

25:                                               ; preds = %40, %24
  %26 = load i32, ptr %18, align 4, !dbg !577
  %27 = load i32, ptr %10, align 4, !dbg !579
  %28 = icmp slt i32 %26, %27, !dbg !580
  br i1 %28, label %29, label %43, !dbg !581

29:                                               ; preds = %25
  %30 = load double, ptr %13, align 8, !dbg !582
  %31 = load ptr, ptr %14, align 8, !dbg !583
  %32 = load i32, ptr %17, align 4, !dbg !584
  %33 = sext i32 %32 to i64, !dbg !583
  %34 = getelementptr inbounds [25 x double], ptr %31, i64 %33, !dbg !583
  %35 = load i32, ptr %18, align 4, !dbg !585
  %36 = sext i32 %35 to i64, !dbg !583
  %37 = getelementptr inbounds [25 x double], ptr %34, i64 0, i64 %36, !dbg !583
  %38 = load double, ptr %37, align 8, !dbg !586
  %39 = fmul double %38, %30, !dbg !586
  store double %39, ptr %37, align 8, !dbg !586
  br label %40, !dbg !583

40:                                               ; preds = %29
  %41 = load i32, ptr %18, align 4, !dbg !587
  %42 = add nsw i32 %41, 1, !dbg !587
  store i32 %42, ptr %18, align 4, !dbg !587
  br label %25, !dbg !588, !llvm.loop !589

43:                                               ; preds = %25
  store i32 0, ptr %19, align 4, !dbg !591
  br label %44, !dbg !593

44:                                               ; preds = %85, %43
  %45 = load i32, ptr %19, align 4, !dbg !594
  %46 = load i32, ptr %11, align 4, !dbg !596
  %47 = icmp slt i32 %45, %46, !dbg !597
  br i1 %47, label %48, label %88, !dbg !598

48:                                               ; preds = %44
  store i32 0, ptr %18, align 4, !dbg !599
  br label %49, !dbg !602

49:                                               ; preds = %81, %48
  %50 = load i32, ptr %18, align 4, !dbg !603
  %51 = load i32, ptr %10, align 4, !dbg !605
  %52 = icmp slt i32 %50, %51, !dbg !606
  br i1 %52, label %53, label %84, !dbg !607

53:                                               ; preds = %49
  %54 = load double, ptr %12, align 8, !dbg !608
  %55 = load ptr, ptr %15, align 8, !dbg !609
  %56 = load i32, ptr %17, align 4, !dbg !610
  %57 = sext i32 %56 to i64, !dbg !609
  %58 = getelementptr inbounds [30 x double], ptr %55, i64 %57, !dbg !609
  %59 = load i32, ptr %19, align 4, !dbg !611
  %60 = sext i32 %59 to i64, !dbg !609
  %61 = getelementptr inbounds [30 x double], ptr %58, i64 0, i64 %60, !dbg !609
  %62 = load double, ptr %61, align 8, !dbg !609
  %63 = fmul double %54, %62, !dbg !612
  %64 = load ptr, ptr %16, align 8, !dbg !613
  %65 = load i32, ptr %19, align 4, !dbg !614
  %66 = sext i32 %65 to i64, !dbg !613
  %67 = getelementptr inbounds [25 x double], ptr %64, i64 %66, !dbg !613
  %68 = load i32, ptr %18, align 4, !dbg !615
  %69 = sext i32 %68 to i64, !dbg !613
  %70 = getelementptr inbounds [25 x double], ptr %67, i64 0, i64 %69, !dbg !613
  %71 = load double, ptr %70, align 8, !dbg !613
  %72 = load ptr, ptr %14, align 8, !dbg !616
  %73 = load i32, ptr %17, align 4, !dbg !617
  %74 = sext i32 %73 to i64, !dbg !616
  %75 = getelementptr inbounds [25 x double], ptr %72, i64 %74, !dbg !616
  %76 = load i32, ptr %18, align 4, !dbg !618
  %77 = sext i32 %76 to i64, !dbg !616
  %78 = getelementptr inbounds [25 x double], ptr %75, i64 0, i64 %77, !dbg !616
  %79 = load double, ptr %78, align 8, !dbg !619
  %80 = call double @llvm.fmuladd.f64(double %63, double %71, double %79), !dbg !619
  store double %80, ptr %78, align 8, !dbg !619
  br label %81, !dbg !616

81:                                               ; preds = %53
  %82 = load i32, ptr %18, align 4, !dbg !620
  %83 = add nsw i32 %82, 1, !dbg !620
  store i32 %83, ptr %18, align 4, !dbg !620
  br label %49, !dbg !621, !llvm.loop !622

84:                                               ; preds = %49
  br label %85, !dbg !624

85:                                               ; preds = %84
  %86 = load i32, ptr %19, align 4, !dbg !625
  %87 = add nsw i32 %86, 1, !dbg !625
  store i32 %87, ptr %19, align 4, !dbg !625
  br label %44, !dbg !626, !llvm.loop !627

88:                                               ; preds = %44
  br label %89, !dbg !629

89:                                               ; preds = %88
  %90 = load i32, ptr %17, align 4, !dbg !630
  %91 = add nsw i32 %90, 1, !dbg !630
  store i32 %91, ptr %17, align 4, !dbg !630
  br label %20, !dbg !631, !llvm.loop !632

92:                                               ; preds = %20
  ret void, !dbg !634
}

; Function Attrs: noinline nounwind uwtable
define internal void @print_array(i32 noundef %0, i32 noundef %1, ptr noundef %2, ptr noundef %3) #0 !dbg !635 {
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca ptr, align 8
  %8 = alloca ptr, align 8
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca float, align 4
  %12 = alloca float, align 4
  %13 = alloca float, align 4
  %14 = alloca double, align 8
  %15 = alloca double, align 8
  %16 = alloca double, align 8
  %17 = alloca float, align 4
  %18 = alloca double, align 8
  %19 = alloca float, align 4
  %20 = alloca double, align 8
  %21 = alloca double, align 8
  store i32 %0, ptr %5, align 4
    #dbg_declare(ptr %5, !638, !DIExpression(), !639)
  store i32 %1, ptr %6, align 4
    #dbg_declare(ptr %6, !640, !DIExpression(), !641)
  store ptr %2, ptr %7, align 8
    #dbg_declare(ptr %7, !642, !DIExpression(), !643)
  store ptr %3, ptr %8, align 8
    #dbg_declare(ptr %8, !644, !DIExpression(), !645)
    #dbg_declare(ptr %9, !646, !DIExpression(), !647)
    #dbg_declare(ptr %10, !648, !DIExpression(), !649)
    #dbg_declare(ptr %11, !650, !DIExpression(), !651)
  store float 0.000000e+00, ptr %11, align 4, !dbg !651
    #dbg_declare(ptr %12, !652, !DIExpression(), !653)
  store float 0.000000e+00, ptr %12, align 4, !dbg !653
    #dbg_declare(ptr %13, !654, !DIExpression(), !655)
  store float 0.000000e+00, ptr %13, align 4, !dbg !655
    #dbg_declare(ptr %14, !656, !DIExpression(), !657)
  store double 0.000000e+00, ptr %14, align 8, !dbg !657
    #dbg_declare(ptr %15, !658, !DIExpression(), !659)
  store double 0.000000e+00, ptr %15, align 8, !dbg !659
    #dbg_declare(ptr %16, !660, !DIExpression(), !661)
  store double 0.000000e+00, ptr %16, align 8, !dbg !661
  %22 = load ptr, ptr @stderr, align 8, !dbg !662
  %23 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %22, ptr noundef @.str) #4, !dbg !662
  %24 = load ptr, ptr @stderr, align 8, !dbg !663
  %25 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %24, ptr noundef @.str.1, ptr noundef @.str.2) #4, !dbg !663
  store i32 0, ptr %9, align 4, !dbg !664
  br label %26, !dbg !666

26:                                               ; preds = %80, %4
  %27 = load i32, ptr %9, align 4, !dbg !667
  %28 = load i32, ptr %5, align 4, !dbg !669
  %29 = icmp slt i32 %27, %28, !dbg !670
  br i1 %29, label %30, label %83, !dbg !671

30:                                               ; preds = %26
  store i32 0, ptr %10, align 4, !dbg !672
  br label %31, !dbg !674

31:                                               ; preds = %76, %30
  %32 = load i32, ptr %10, align 4, !dbg !675
  %33 = load i32, ptr %6, align 4, !dbg !677
  %34 = icmp slt i32 %32, %33, !dbg !678
  br i1 %34, label %35, label %79, !dbg !679

35:                                               ; preds = %31
    #dbg_declare(ptr %17, !680, !DIExpression(), !682)
  %36 = load ptr, ptr %7, align 8, !dbg !683
  %37 = load i32, ptr %9, align 4, !dbg !684
  %38 = sext i32 %37 to i64, !dbg !683
  %39 = getelementptr inbounds [25 x float], ptr %36, i64 %38, !dbg !683
  %40 = load i32, ptr %10, align 4, !dbg !685
  %41 = sext i32 %40 to i64, !dbg !683
  %42 = getelementptr inbounds [25 x float], ptr %39, i64 0, i64 %41, !dbg !683
  %43 = load float, ptr %42, align 4, !dbg !683
  store float %43, ptr %17, align 4, !dbg !682
    #dbg_declare(ptr %18, !686, !DIExpression(), !687)
  %44 = load ptr, ptr %8, align 8, !dbg !688
  %45 = load i32, ptr %9, align 4, !dbg !689
  %46 = sext i32 %45 to i64, !dbg !688
  %47 = getelementptr inbounds [25 x double], ptr %44, i64 %46, !dbg !688
  %48 = load i32, ptr %10, align 4, !dbg !690
  %49 = sext i32 %48 to i64, !dbg !688
  %50 = getelementptr inbounds [25 x double], ptr %47, i64 0, i64 %49, !dbg !688
  %51 = load double, ptr %50, align 8, !dbg !688
  store double %51, ptr %18, align 8, !dbg !687
  %52 = load float, ptr %17, align 4, !dbg !691
  %53 = fcmp olt float %52, 0.000000e+00, !dbg !693
  br i1 %53, label %54, label %57, !dbg !693

54:                                               ; preds = %35
  %55 = load float, ptr %17, align 4, !dbg !694
  %56 = fneg float %55, !dbg !695
  store float %56, ptr %17, align 4, !dbg !696
  br label %57, !dbg !697

57:                                               ; preds = %54, %35
  %58 = load double, ptr %18, align 8, !dbg !698
  %59 = fcmp olt double %58, 0.000000e+00, !dbg !700
  br i1 %59, label %60, label %63, !dbg !700

60:                                               ; preds = %57
  %61 = load double, ptr %18, align 8, !dbg !701
  %62 = fneg double %61, !dbg !702
  store double %62, ptr %18, align 8, !dbg !703
  br label %63, !dbg !704

63:                                               ; preds = %60, %57
  %64 = load float, ptr %17, align 4, !dbg !705
  %65 = load float, ptr %11, align 4, !dbg !707
  %66 = fcmp ogt float %64, %65, !dbg !708
  br i1 %66, label %67, label %69, !dbg !708

67:                                               ; preds = %63
  %68 = load float, ptr %17, align 4, !dbg !709
  store float %68, ptr %11, align 4, !dbg !711
  br label %69, !dbg !712

69:                                               ; preds = %67, %63
  %70 = load double, ptr %18, align 8, !dbg !713
  %71 = load double, ptr %14, align 8, !dbg !715
  %72 = fcmp ogt double %70, %71, !dbg !716
  br i1 %72, label %73, label %75, !dbg !716

73:                                               ; preds = %69
  %74 = load double, ptr %18, align 8, !dbg !717
  store double %74, ptr %14, align 8, !dbg !719
  br label %75, !dbg !720

75:                                               ; preds = %73, %69
  br label %76, !dbg !721

76:                                               ; preds = %75
  %77 = load i32, ptr %10, align 4, !dbg !722
  %78 = add nsw i32 %77, 1, !dbg !722
  store i32 %78, ptr %10, align 4, !dbg !722
  br label %31, !dbg !723, !llvm.loop !724

79:                                               ; preds = %31
  br label %80, !dbg !725

80:                                               ; preds = %79
  %81 = load i32, ptr %9, align 4, !dbg !726
  %82 = add nsw i32 %81, 1, !dbg !726
  store i32 %82, ptr %9, align 4, !dbg !726
  br label %26, !dbg !727, !llvm.loop !728

83:                                               ; preds = %26
  %84 = load float, ptr %11, align 4, !dbg !730
  %85 = fcmp une float %84, 0.000000e+00, !dbg !732
  br i1 %85, label %86, label %121, !dbg !732

86:                                               ; preds = %83
  store i32 0, ptr %9, align 4, !dbg !733
  br label %87, !dbg !736

87:                                               ; preds = %115, %86
  %88 = load i32, ptr %9, align 4, !dbg !737
  %89 = load i32, ptr %5, align 4, !dbg !739
  %90 = icmp slt i32 %88, %89, !dbg !740
  br i1 %90, label %91, label %118, !dbg !741

91:                                               ; preds = %87
  store i32 0, ptr %10, align 4, !dbg !742
  br label %92, !dbg !745

92:                                               ; preds = %111, %91
  %93 = load i32, ptr %10, align 4, !dbg !746
  %94 = load i32, ptr %6, align 4, !dbg !748
  %95 = icmp slt i32 %93, %94, !dbg !749
  br i1 %95, label %96, label %114, !dbg !750

96:                                               ; preds = %92
    #dbg_declare(ptr %19, !751, !DIExpression(), !753)
  %97 = load ptr, ptr %7, align 8, !dbg !754
  %98 = load i32, ptr %9, align 4, !dbg !755
  %99 = sext i32 %98 to i64, !dbg !754
  %100 = getelementptr inbounds [25 x float], ptr %97, i64 %99, !dbg !754
  %101 = load i32, ptr %10, align 4, !dbg !756
  %102 = sext i32 %101 to i64, !dbg !754
  %103 = getelementptr inbounds [25 x float], ptr %100, i64 0, i64 %102, !dbg !754
  %104 = load float, ptr %103, align 4, !dbg !754
  %105 = load float, ptr %11, align 4, !dbg !757
  %106 = fdiv float %104, %105, !dbg !758
  store float %106, ptr %19, align 4, !dbg !753
  %107 = load float, ptr %19, align 4, !dbg !759
  %108 = load float, ptr %19, align 4, !dbg !760
  %109 = load float, ptr %12, align 4, !dbg !761
  %110 = call float @llvm.fmuladd.f32(float %107, float %108, float %109), !dbg !761
  store float %110, ptr %12, align 4, !dbg !761
  br label %111, !dbg !762

111:                                              ; preds = %96
  %112 = load i32, ptr %10, align 4, !dbg !763
  %113 = add nsw i32 %112, 1, !dbg !763
  store i32 %113, ptr %10, align 4, !dbg !763
  br label %92, !dbg !764, !llvm.loop !765

114:                                              ; preds = %92
  br label %115, !dbg !767

115:                                              ; preds = %114
  %116 = load i32, ptr %9, align 4, !dbg !768
  %117 = add nsw i32 %116, 1, !dbg !768
  store i32 %117, ptr %9, align 4, !dbg !768
  br label %87, !dbg !769, !llvm.loop !770

118:                                              ; preds = %87
  %119 = load float, ptr %12, align 4, !dbg !772
  %120 = call float @sqrtf(float noundef %119) #4, !dbg !772
  store float %120, ptr %13, align 4, !dbg !773
  br label %121, !dbg !774

121:                                              ; preds = %118, %83
  %122 = load double, ptr %14, align 8, !dbg !775
  %123 = fcmp une double %122, 0.000000e+00, !dbg !777
  br i1 %123, label %124, label %159, !dbg !777

124:                                              ; preds = %121
  store i32 0, ptr %9, align 4, !dbg !778
  br label %125, !dbg !781

125:                                              ; preds = %153, %124
  %126 = load i32, ptr %9, align 4, !dbg !782
  %127 = load i32, ptr %5, align 4, !dbg !784
  %128 = icmp slt i32 %126, %127, !dbg !785
  br i1 %128, label %129, label %156, !dbg !786

129:                                              ; preds = %125
  store i32 0, ptr %10, align 4, !dbg !787
  br label %130, !dbg !790

130:                                              ; preds = %149, %129
  %131 = load i32, ptr %10, align 4, !dbg !791
  %132 = load i32, ptr %6, align 4, !dbg !793
  %133 = icmp slt i32 %131, %132, !dbg !794
  br i1 %133, label %134, label %152, !dbg !795

134:                                              ; preds = %130
    #dbg_declare(ptr %20, !796, !DIExpression(), !798)
  %135 = load ptr, ptr %8, align 8, !dbg !799
  %136 = load i32, ptr %9, align 4, !dbg !800
  %137 = sext i32 %136 to i64, !dbg !799
  %138 = getelementptr inbounds [25 x double], ptr %135, i64 %137, !dbg !799
  %139 = load i32, ptr %10, align 4, !dbg !801
  %140 = sext i32 %139 to i64, !dbg !799
  %141 = getelementptr inbounds [25 x double], ptr %138, i64 0, i64 %140, !dbg !799
  %142 = load double, ptr %141, align 8, !dbg !799
  %143 = load double, ptr %14, align 8, !dbg !802
  %144 = fdiv double %142, %143, !dbg !803
  store double %144, ptr %20, align 8, !dbg !798
  %145 = load double, ptr %20, align 8, !dbg !804
  %146 = load double, ptr %20, align 8, !dbg !805
  %147 = load double, ptr %15, align 8, !dbg !806
  %148 = call double @llvm.fmuladd.f64(double %145, double %146, double %147), !dbg !806
  store double %148, ptr %15, align 8, !dbg !806
  br label %149, !dbg !807

149:                                              ; preds = %134
  %150 = load i32, ptr %10, align 4, !dbg !808
  %151 = add nsw i32 %150, 1, !dbg !808
  store i32 %151, ptr %10, align 4, !dbg !808
  br label %130, !dbg !809, !llvm.loop !810

152:                                              ; preds = %130
  br label %153, !dbg !812

153:                                              ; preds = %152
  %154 = load i32, ptr %9, align 4, !dbg !813
  %155 = add nsw i32 %154, 1, !dbg !813
  store i32 %155, ptr %9, align 4, !dbg !813
  br label %125, !dbg !814, !llvm.loop !815

156:                                              ; preds = %125
  %157 = load double, ptr %15, align 8, !dbg !817
  %158 = call double @sqrt(double noundef %157) #4, !dbg !818
  store double %158, ptr %16, align 8, !dbg !819
  br label %159, !dbg !820

159:                                              ; preds = %156, %121
  %160 = load ptr, ptr @stderr, align 8, !dbg !821
  %161 = load float, ptr %11, align 4, !dbg !822
  %162 = fpext float %161 to double, !dbg !822
  %163 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %160, ptr noundef @.str.3, double noundef %162) #4, !dbg !823
  %164 = load ptr, ptr @stderr, align 8, !dbg !824
  %165 = load float, ptr %13, align 4, !dbg !825
  %166 = fpext float %165 to double, !dbg !825
  %167 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %164, ptr noundef @.str.4, double noundef %166) #4, !dbg !826
  %168 = load ptr, ptr @stderr, align 8, !dbg !827
  %169 = load double, ptr %14, align 8, !dbg !828
  %170 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %168, ptr noundef @.str.5, double noundef %169) #4, !dbg !829
  %171 = load ptr, ptr @stderr, align 8, !dbg !830
  %172 = load double, ptr %16, align 8, !dbg !831
  %173 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %171, ptr noundef @.str.6, double noundef %172) #4, !dbg !832
    #dbg_declare(ptr %21, !833, !DIExpression(), !834)
  %174 = load double, ptr %16, align 8, !dbg !835
  %175 = load float, ptr %13, align 4, !dbg !836
  %176 = fpext float %175 to double, !dbg !837
  %177 = fsub double %174, %176, !dbg !838
  store double %177, ptr %21, align 8, !dbg !834
  %178 = load ptr, ptr @stderr, align 8, !dbg !839
  %179 = load double, ptr %21, align 8, !dbg !840
  %180 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %178, ptr noundef @.str.7, double noundef %179) #4, !dbg !841
  %181 = load ptr, ptr @stderr, align 8, !dbg !842
  %182 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %181, ptr noundef @.str.8, ptr noundef @.str.9) #4, !dbg !842
  %183 = load ptr, ptr @stderr, align 8, !dbg !843
  %184 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %183, ptr noundef @.str.10) #4, !dbg !843
  ret void, !dbg !844
}

; Function Attrs: nounwind
declare dso_local void @free(ptr noundef) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fmuladd.f32(float, float, float) #3

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.fmuladd.f64(double, double, double) #3

; Function Attrs: nounwind
declare dso_local i32 @fprintf(ptr noundef, ptr noundef, ...) #2

; Function Attrs: nounwind
declare dso_local float @sqrtf(float noundef) #2

; Function Attrs: nounwind
declare dso_local double @sqrt(double noundef) #2

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!51}
!llvm.module.flags = !{!73, !74, !75, !76, !77}
!llvm.ident = !{!78}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 87, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "gemm.c", directory: "/g/g90/sharmin1/tutorial/FPChecker/experiments/benchmark/PolyBenchC-4.2.1/linear-algebra/blas/gemm")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 184, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 23)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 88, type: !9, isLocal: true, isDefinition: true)
!9 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 120, elements: !10)
!10 = !{!11}
!11 = !DISubrange(count: 15)
!12 = !DIGlobalVariableExpression(var: !13, expr: !DIExpression())
!13 = distinct !DIGlobalVariable(scope: null, file: !2, line: 88, type: !14, isLocal: true, isDefinition: true)
!14 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 24, elements: !15)
!15 = !{!16}
!16 = !DISubrange(count: 3)
!17 = !DIGlobalVariableExpression(var: !18, expr: !DIExpression())
!18 = distinct !DIGlobalVariable(scope: null, file: !2, line: 134, type: !19, isLocal: true, isDefinition: true)
!19 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 176, elements: !20)
!20 = !{!21}
!21 = !DISubrange(count: 22)
!22 = !DIGlobalVariableExpression(var: !23, expr: !DIExpression())
!23 = distinct !DIGlobalVariable(scope: null, file: !2, line: 135, type: !24, isLocal: true, isDefinition: true)
!24 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 136, elements: !25)
!25 = !{!26}
!26 = !DISubrange(count: 17)
!27 = !DIGlobalVariableExpression(var: !28, expr: !DIExpression())
!28 = distinct !DIGlobalVariable(scope: null, file: !2, line: 136, type: !29, isLocal: true, isDefinition: true)
!29 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 240, elements: !30)
!30 = !{!31}
!31 = !DISubrange(count: 30)
!32 = !DIGlobalVariableExpression(var: !33, expr: !DIExpression())
!33 = distinct !DIGlobalVariable(scope: null, file: !2, line: 137, type: !34, isLocal: true, isDefinition: true)
!34 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 200, elements: !35)
!35 = !{!36}
!36 = !DISubrange(count: 25)
!37 = !DIGlobalVariableExpression(var: !38, expr: !DIExpression())
!38 = distinct !DIGlobalVariable(scope: null, file: !2, line: 140, type: !39, isLocal: true, isDefinition: true)
!39 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 152, elements: !40)
!40 = !{!41}
!41 = !DISubrange(count: 19)
!42 = !DIGlobalVariableExpression(var: !43, expr: !DIExpression())
!43 = distinct !DIGlobalVariable(scope: null, file: !2, line: 142, type: !24, isLocal: true, isDefinition: true)
!44 = !DIGlobalVariableExpression(var: !45, expr: !DIExpression())
!45 = distinct !DIGlobalVariable(scope: null, file: !2, line: 142, type: !46, isLocal: true, isDefinition: true)
!46 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 16, elements: !47)
!47 = !{!48}
!48 = !DISubrange(count: 2)
!49 = !DIGlobalVariableExpression(var: !50, expr: !DIExpression())
!50 = distinct !DIGlobalVariable(scope: null, file: !2, line: 143, type: !3, isLocal: true, isDefinition: true)
!51 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !52, globals: !72, splitDebugInlining: false, nameTableKind: None)
!52 = !{!53, !58, !61, !64, !67, !69, !71, !55, !66}
!53 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !54, size: 64)
!54 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 16000, elements: !56)
!55 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!56 = !{!57, !36}
!57 = !DISubrange(count: 20)
!58 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !59, size: 64)
!59 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 19200, elements: !60)
!60 = !{!57, !31}
!61 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !62, size: 64)
!62 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 24000, elements: !63)
!63 = !{!31, !36}
!64 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !65, size: 64)
!65 = !DICompositeType(tag: DW_TAG_array_type, baseType: !66, size: 32000, elements: !56)
!66 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!67 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !68, size: 64)
!68 = !DICompositeType(tag: DW_TAG_array_type, baseType: !66, size: 38400, elements: !60)
!69 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !70, size: 64)
!70 = !DICompositeType(tag: DW_TAG_array_type, baseType: !66, size: 48000, elements: !63)
!71 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!72 = !{!0, !7, !12, !17, !22, !27, !32, !37, !42, !44, !49}
!73 = !{i32 7, !"Dwarf Version", i32 4}
!74 = !{i32 2, !"Debug Info Version", i32 3}
!75 = !{i32 1, !"wchar_size", i32 4}
!76 = !{i32 7, !"uwtable", i32 2}
!77 = !{i32 7, !"frame-pointer", i32 2}
!78 = !{!"clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)"}
!79 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 202, type: !80, scopeLine: 203, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !51, retainedNodes: !85)
!80 = !DISubroutineType(types: !81)
!81 = !{!82, !82, !83}
!82 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!83 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !84, size: 64)
!84 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!85 = !{}
!86 = !DILocalVariable(name: "argc", arg: 1, scope: !79, file: !2, line: 202, type: !82)
!87 = !DILocation(line: 202, column: 14, scope: !79)
!88 = !DILocalVariable(name: "argv", arg: 2, scope: !79, file: !2, line: 202, type: !83)
!89 = !DILocation(line: 202, column: 27, scope: !79)
!90 = !DILocalVariable(name: "ni", scope: !79, file: !2, line: 205, type: !82)
!91 = !DILocation(line: 205, column: 7, scope: !79)
!92 = !DILocalVariable(name: "nj", scope: !79, file: !2, line: 206, type: !82)
!93 = !DILocation(line: 206, column: 7, scope: !79)
!94 = !DILocalVariable(name: "nk", scope: !79, file: !2, line: 207, type: !82)
!95 = !DILocation(line: 207, column: 7, scope: !79)
!96 = !DILocalVariable(name: "alpha", scope: !79, file: !2, line: 210, type: !55)
!97 = !DILocation(line: 210, column: 13, scope: !79)
!98 = !DILocalVariable(name: "beta", scope: !79, file: !2, line: 211, type: !55)
!99 = !DILocation(line: 211, column: 13, scope: !79)
!100 = !DILocalVariable(name: "alpha_double", scope: !79, file: !2, line: 212, type: !66)
!101 = !DILocation(line: 212, column: 10, scope: !79)
!102 = !DILocalVariable(name: "beta_double", scope: !79, file: !2, line: 213, type: !66)
!103 = !DILocation(line: 213, column: 10, scope: !79)
!104 = !DILocalVariable(name: "C", scope: !79, file: !2, line: 215, type: !53)
!105 = !DILocation(line: 215, column: 3, scope: !79)
!106 = !DILocalVariable(name: "A", scope: !79, file: !2, line: 216, type: !58)
!107 = !DILocation(line: 216, column: 3, scope: !79)
!108 = !DILocalVariable(name: "B", scope: !79, file: !2, line: 217, type: !61)
!109 = !DILocation(line: 217, column: 3, scope: !79)
!110 = !DILocalVariable(name: "C_double", scope: !79, file: !2, line: 219, type: !64)
!111 = !DILocation(line: 219, column: 3, scope: !79)
!112 = !DILocalVariable(name: "A_double", scope: !79, file: !2, line: 220, type: !67)
!113 = !DILocation(line: 220, column: 3, scope: !79)
!114 = !DILocalVariable(name: "B_double", scope: !79, file: !2, line: 221, type: !69)
!115 = !DILocation(line: 221, column: 3, scope: !79)
!116 = !DILocation(line: 224, column: 15, scope: !79)
!117 = !DILocation(line: 224, column: 19, scope: !79)
!118 = !DILocation(line: 224, column: 23, scope: !79)
!119 = !DILocation(line: 225, column: 8, scope: !79)
!120 = !DILocation(line: 226, column: 8, scope: !79)
!121 = !DILocation(line: 227, column: 8, scope: !79)
!122 = !DILocation(line: 224, column: 3, scope: !79)
!123 = !DILocation(line: 229, column: 22, scope: !79)
!124 = !DILocation(line: 229, column: 26, scope: !79)
!125 = !DILocation(line: 229, column: 30, scope: !79)
!126 = !DILocation(line: 230, column: 8, scope: !79)
!127 = !DILocation(line: 231, column: 8, scope: !79)
!128 = !DILocation(line: 232, column: 8, scope: !79)
!129 = !DILocation(line: 229, column: 3, scope: !79)
!130 = !DILocation(line: 237, column: 16, scope: !79)
!131 = !DILocation(line: 237, column: 20, scope: !79)
!132 = !DILocation(line: 237, column: 24, scope: !79)
!133 = !DILocation(line: 238, column: 9, scope: !79)
!134 = !DILocation(line: 238, column: 16, scope: !79)
!135 = !DILocation(line: 239, column: 9, scope: !79)
!136 = !DILocation(line: 240, column: 9, scope: !79)
!137 = !DILocation(line: 241, column: 9, scope: !79)
!138 = !DILocation(line: 237, column: 3, scope: !79)
!139 = !DILocation(line: 243, column: 23, scope: !79)
!140 = !DILocation(line: 243, column: 27, scope: !79)
!141 = !DILocation(line: 243, column: 31, scope: !79)
!142 = !DILocation(line: 244, column: 9, scope: !79)
!143 = !DILocation(line: 244, column: 23, scope: !79)
!144 = !DILocation(line: 245, column: 9, scope: !79)
!145 = !DILocation(line: 246, column: 9, scope: !79)
!146 = !DILocation(line: 247, column: 9, scope: !79)
!147 = !DILocation(line: 243, column: 3, scope: !79)
!148 = !DILocation(line: 255, column: 3, scope: !79)
!149 = !DILocation(line: 258, column: 3, scope: !79)
!150 = !DILocation(line: 259, column: 3, scope: !79)
!151 = !DILocation(line: 260, column: 3, scope: !79)
!152 = !DILocation(line: 261, column: 3, scope: !79)
!153 = !DILocation(line: 262, column: 3, scope: !79)
!154 = !DILocation(line: 263, column: 3, scope: !79)
!155 = !DILocation(line: 265, column: 3, scope: !79)
!156 = distinct !DISubprogram(name: "init_array", scope: !2, file: !2, line: 26, type: !157, scopeLine: 32, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !85)
!157 = !DISubroutineType(types: !158)
!158 = !{null, !82, !82, !82, !159, !159, !160, !162, !160}
!159 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !55, size: 64)
!160 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !161, size: 64)
!161 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 800, elements: !35)
!162 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !163, size: 64)
!163 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 960, elements: !30)
!164 = !DILocalVariable(name: "ni", arg: 1, scope: !156, file: !2, line: 26, type: !82)
!165 = !DILocation(line: 26, column: 21, scope: !156)
!166 = !DILocalVariable(name: "nj", arg: 2, scope: !156, file: !2, line: 26, type: !82)
!167 = !DILocation(line: 26, column: 29, scope: !156)
!168 = !DILocalVariable(name: "nk", arg: 3, scope: !156, file: !2, line: 26, type: !82)
!169 = !DILocation(line: 26, column: 37, scope: !156)
!170 = !DILocalVariable(name: "alpha", arg: 4, scope: !156, file: !2, line: 27, type: !159)
!171 = !DILocation(line: 27, column: 14, scope: !156)
!172 = !DILocalVariable(name: "beta", arg: 5, scope: !156, file: !2, line: 28, type: !159)
!173 = !DILocation(line: 28, column: 14, scope: !156)
!174 = !DILocalVariable(name: "C", arg: 6, scope: !156, file: !2, line: 29, type: !160)
!175 = !DILocation(line: 29, column: 13, scope: !156)
!176 = !DILocalVariable(name: "A", arg: 7, scope: !156, file: !2, line: 30, type: !162)
!177 = !DILocation(line: 30, column: 13, scope: !156)
!178 = !DILocalVariable(name: "B", arg: 8, scope: !156, file: !2, line: 31, type: !160)
!179 = !DILocation(line: 31, column: 13, scope: !156)
!180 = !DILocalVariable(name: "i", scope: !156, file: !2, line: 33, type: !82)
!181 = !DILocation(line: 33, column: 7, scope: !156)
!182 = !DILocalVariable(name: "j", scope: !156, file: !2, line: 33, type: !82)
!183 = !DILocation(line: 33, column: 10, scope: !156)
!184 = !DILocation(line: 35, column: 4, scope: !156)
!185 = !DILocation(line: 35, column: 10, scope: !156)
!186 = !DILocation(line: 36, column: 4, scope: !156)
!187 = !DILocation(line: 36, column: 9, scope: !156)
!188 = !DILocation(line: 37, column: 10, scope: !189)
!189 = distinct !DILexicalBlock(scope: !156, file: !2, line: 37, column: 3)
!190 = !DILocation(line: 37, column: 8, scope: !189)
!191 = !DILocation(line: 37, column: 15, scope: !192)
!192 = distinct !DILexicalBlock(scope: !189, file: !2, line: 37, column: 3)
!193 = !DILocation(line: 37, column: 19, scope: !192)
!194 = !DILocation(line: 37, column: 17, scope: !192)
!195 = !DILocation(line: 37, column: 3, scope: !189)
!196 = !DILocation(line: 38, column: 12, scope: !197)
!197 = distinct !DILexicalBlock(scope: !192, file: !2, line: 38, column: 5)
!198 = !DILocation(line: 38, column: 10, scope: !197)
!199 = !DILocation(line: 38, column: 17, scope: !200)
!200 = distinct !DILexicalBlock(scope: !197, file: !2, line: 38, column: 5)
!201 = !DILocation(line: 38, column: 21, scope: !200)
!202 = !DILocation(line: 38, column: 19, scope: !200)
!203 = !DILocation(line: 38, column: 5, scope: !197)
!204 = !DILocation(line: 39, column: 31, scope: !200)
!205 = !DILocation(line: 39, column: 33, scope: !200)
!206 = !DILocation(line: 39, column: 32, scope: !200)
!207 = !DILocation(line: 39, column: 34, scope: !200)
!208 = !DILocation(line: 39, column: 40, scope: !200)
!209 = !DILocation(line: 39, column: 38, scope: !200)
!210 = !DILocation(line: 39, column: 17, scope: !200)
!211 = !DILocation(line: 39, column: 46, scope: !200)
!212 = !DILocation(line: 39, column: 44, scope: !200)
!213 = !DILocation(line: 39, column: 7, scope: !200)
!214 = !DILocation(line: 39, column: 9, scope: !200)
!215 = !DILocation(line: 39, column: 12, scope: !200)
!216 = !DILocation(line: 39, column: 15, scope: !200)
!217 = !DILocation(line: 38, column: 26, scope: !200)
!218 = !DILocation(line: 38, column: 5, scope: !200)
!219 = distinct !{!219, !203, !220, !221}
!220 = !DILocation(line: 39, column: 46, scope: !197)
!221 = !{!"llvm.loop.mustprogress"}
!222 = !DILocation(line: 37, column: 24, scope: !192)
!223 = !DILocation(line: 37, column: 3, scope: !192)
!224 = distinct !{!224, !195, !225, !221}
!225 = !DILocation(line: 39, column: 46, scope: !189)
!226 = !DILocation(line: 40, column: 10, scope: !227)
!227 = distinct !DILexicalBlock(scope: !156, file: !2, line: 40, column: 3)
!228 = !DILocation(line: 40, column: 8, scope: !227)
!229 = !DILocation(line: 40, column: 15, scope: !230)
!230 = distinct !DILexicalBlock(scope: !227, file: !2, line: 40, column: 3)
!231 = !DILocation(line: 40, column: 19, scope: !230)
!232 = !DILocation(line: 40, column: 17, scope: !230)
!233 = !DILocation(line: 40, column: 3, scope: !227)
!234 = !DILocation(line: 41, column: 12, scope: !235)
!235 = distinct !DILexicalBlock(scope: !230, file: !2, line: 41, column: 5)
!236 = !DILocation(line: 41, column: 10, scope: !235)
!237 = !DILocation(line: 41, column: 17, scope: !238)
!238 = distinct !DILexicalBlock(scope: !235, file: !2, line: 41, column: 5)
!239 = !DILocation(line: 41, column: 21, scope: !238)
!240 = !DILocation(line: 41, column: 19, scope: !238)
!241 = !DILocation(line: 41, column: 5, scope: !235)
!242 = !DILocation(line: 42, column: 30, scope: !238)
!243 = !DILocation(line: 42, column: 33, scope: !238)
!244 = !DILocation(line: 42, column: 34, scope: !238)
!245 = !DILocation(line: 42, column: 31, scope: !238)
!246 = !DILocation(line: 42, column: 40, scope: !238)
!247 = !DILocation(line: 42, column: 38, scope: !238)
!248 = !DILocation(line: 42, column: 17, scope: !238)
!249 = !DILocation(line: 42, column: 46, scope: !238)
!250 = !DILocation(line: 42, column: 44, scope: !238)
!251 = !DILocation(line: 42, column: 7, scope: !238)
!252 = !DILocation(line: 42, column: 9, scope: !238)
!253 = !DILocation(line: 42, column: 12, scope: !238)
!254 = !DILocation(line: 42, column: 15, scope: !238)
!255 = !DILocation(line: 41, column: 26, scope: !238)
!256 = !DILocation(line: 41, column: 5, scope: !238)
!257 = distinct !{!257, !241, !258, !221}
!258 = !DILocation(line: 42, column: 46, scope: !235)
!259 = !DILocation(line: 40, column: 24, scope: !230)
!260 = !DILocation(line: 40, column: 3, scope: !230)
!261 = distinct !{!261, !233, !262, !221}
!262 = !DILocation(line: 42, column: 46, scope: !227)
!263 = !DILocation(line: 43, column: 10, scope: !264)
!264 = distinct !DILexicalBlock(scope: !156, file: !2, line: 43, column: 3)
!265 = !DILocation(line: 43, column: 8, scope: !264)
!266 = !DILocation(line: 43, column: 15, scope: !267)
!267 = distinct !DILexicalBlock(scope: !264, file: !2, line: 43, column: 3)
!268 = !DILocation(line: 43, column: 19, scope: !267)
!269 = !DILocation(line: 43, column: 17, scope: !267)
!270 = !DILocation(line: 43, column: 3, scope: !264)
!271 = !DILocation(line: 44, column: 12, scope: !272)
!272 = distinct !DILexicalBlock(scope: !267, file: !2, line: 44, column: 5)
!273 = !DILocation(line: 44, column: 10, scope: !272)
!274 = !DILocation(line: 44, column: 17, scope: !275)
!275 = distinct !DILexicalBlock(scope: !272, file: !2, line: 44, column: 5)
!276 = !DILocation(line: 44, column: 21, scope: !275)
!277 = !DILocation(line: 44, column: 19, scope: !275)
!278 = !DILocation(line: 44, column: 5, scope: !272)
!279 = !DILocation(line: 45, column: 30, scope: !275)
!280 = !DILocation(line: 45, column: 33, scope: !275)
!281 = !DILocation(line: 45, column: 34, scope: !275)
!282 = !DILocation(line: 45, column: 31, scope: !275)
!283 = !DILocation(line: 45, column: 40, scope: !275)
!284 = !DILocation(line: 45, column: 38, scope: !275)
!285 = !DILocation(line: 45, column: 17, scope: !275)
!286 = !DILocation(line: 45, column: 46, scope: !275)
!287 = !DILocation(line: 45, column: 44, scope: !275)
!288 = !DILocation(line: 45, column: 7, scope: !275)
!289 = !DILocation(line: 45, column: 9, scope: !275)
!290 = !DILocation(line: 45, column: 12, scope: !275)
!291 = !DILocation(line: 45, column: 15, scope: !275)
!292 = !DILocation(line: 44, column: 26, scope: !275)
!293 = !DILocation(line: 44, column: 5, scope: !275)
!294 = distinct !{!294, !278, !295, !221}
!295 = !DILocation(line: 45, column: 46, scope: !272)
!296 = !DILocation(line: 43, column: 24, scope: !267)
!297 = !DILocation(line: 43, column: 3, scope: !267)
!298 = distinct !{!298, !270, !299, !221}
!299 = !DILocation(line: 45, column: 46, scope: !264)
!300 = !DILocation(line: 46, column: 1, scope: !156)
!301 = distinct !DISubprogram(name: "init_array_double", scope: !2, file: !2, line: 49, type: !302, scopeLine: 55, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !85)
!302 = !DISubroutineType(types: !303)
!303 = !{null, !82, !82, !82, !304, !304, !305, !307, !305}
!304 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !66, size: 64)
!305 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !306, size: 64)
!306 = !DICompositeType(tag: DW_TAG_array_type, baseType: !66, size: 1600, elements: !35)
!307 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !308, size: 64)
!308 = !DICompositeType(tag: DW_TAG_array_type, baseType: !66, size: 1920, elements: !30)
!309 = !DILocalVariable(name: "ni", arg: 1, scope: !301, file: !2, line: 49, type: !82)
!310 = !DILocation(line: 49, column: 28, scope: !301)
!311 = !DILocalVariable(name: "nj", arg: 2, scope: !301, file: !2, line: 49, type: !82)
!312 = !DILocation(line: 49, column: 36, scope: !301)
!313 = !DILocalVariable(name: "nk", arg: 3, scope: !301, file: !2, line: 49, type: !82)
!314 = !DILocation(line: 49, column: 44, scope: !301)
!315 = !DILocalVariable(name: "alpha", arg: 4, scope: !301, file: !2, line: 50, type: !304)
!316 = !DILocation(line: 50, column: 11, scope: !301)
!317 = !DILocalVariable(name: "beta", arg: 5, scope: !301, file: !2, line: 51, type: !304)
!318 = !DILocation(line: 51, column: 11, scope: !301)
!319 = !DILocalVariable(name: "C", arg: 6, scope: !301, file: !2, line: 52, type: !305)
!320 = !DILocation(line: 52, column: 10, scope: !301)
!321 = !DILocalVariable(name: "A", arg: 7, scope: !301, file: !2, line: 53, type: !307)
!322 = !DILocation(line: 53, column: 10, scope: !301)
!323 = !DILocalVariable(name: "B", arg: 8, scope: !301, file: !2, line: 54, type: !305)
!324 = !DILocation(line: 54, column: 10, scope: !301)
!325 = !DILocalVariable(name: "i", scope: !301, file: !2, line: 56, type: !82)
!326 = !DILocation(line: 56, column: 7, scope: !301)
!327 = !DILocalVariable(name: "j", scope: !301, file: !2, line: 56, type: !82)
!328 = !DILocation(line: 56, column: 10, scope: !301)
!329 = !DILocation(line: 58, column: 4, scope: !301)
!330 = !DILocation(line: 58, column: 10, scope: !301)
!331 = !DILocation(line: 59, column: 4, scope: !301)
!332 = !DILocation(line: 59, column: 9, scope: !301)
!333 = !DILocation(line: 60, column: 10, scope: !334)
!334 = distinct !DILexicalBlock(scope: !301, file: !2, line: 60, column: 3)
!335 = !DILocation(line: 60, column: 8, scope: !334)
!336 = !DILocation(line: 60, column: 15, scope: !337)
!337 = distinct !DILexicalBlock(scope: !334, file: !2, line: 60, column: 3)
!338 = !DILocation(line: 60, column: 19, scope: !337)
!339 = !DILocation(line: 60, column: 17, scope: !337)
!340 = !DILocation(line: 60, column: 3, scope: !334)
!341 = !DILocation(line: 61, column: 12, scope: !342)
!342 = distinct !DILexicalBlock(scope: !337, file: !2, line: 61, column: 5)
!343 = !DILocation(line: 61, column: 10, scope: !342)
!344 = !DILocation(line: 61, column: 17, scope: !345)
!345 = distinct !DILexicalBlock(scope: !342, file: !2, line: 61, column: 5)
!346 = !DILocation(line: 61, column: 21, scope: !345)
!347 = !DILocation(line: 61, column: 19, scope: !345)
!348 = !DILocation(line: 61, column: 5, scope: !342)
!349 = !DILocation(line: 62, column: 28, scope: !345)
!350 = !DILocation(line: 62, column: 30, scope: !345)
!351 = !DILocation(line: 62, column: 29, scope: !345)
!352 = !DILocation(line: 62, column: 31, scope: !345)
!353 = !DILocation(line: 62, column: 37, scope: !345)
!354 = !DILocation(line: 62, column: 35, scope: !345)
!355 = !DILocation(line: 62, column: 17, scope: !345)
!356 = !DILocation(line: 62, column: 43, scope: !345)
!357 = !DILocation(line: 62, column: 41, scope: !345)
!358 = !DILocation(line: 62, column: 7, scope: !345)
!359 = !DILocation(line: 62, column: 9, scope: !345)
!360 = !DILocation(line: 62, column: 12, scope: !345)
!361 = !DILocation(line: 62, column: 15, scope: !345)
!362 = !DILocation(line: 61, column: 26, scope: !345)
!363 = !DILocation(line: 61, column: 5, scope: !345)
!364 = distinct !{!364, !348, !365, !221}
!365 = !DILocation(line: 62, column: 43, scope: !342)
!366 = !DILocation(line: 60, column: 24, scope: !337)
!367 = !DILocation(line: 60, column: 3, scope: !337)
!368 = distinct !{!368, !340, !369, !221}
!369 = !DILocation(line: 62, column: 43, scope: !334)
!370 = !DILocation(line: 63, column: 10, scope: !371)
!371 = distinct !DILexicalBlock(scope: !301, file: !2, line: 63, column: 3)
!372 = !DILocation(line: 63, column: 8, scope: !371)
!373 = !DILocation(line: 63, column: 15, scope: !374)
!374 = distinct !DILexicalBlock(scope: !371, file: !2, line: 63, column: 3)
!375 = !DILocation(line: 63, column: 19, scope: !374)
!376 = !DILocation(line: 63, column: 17, scope: !374)
!377 = !DILocation(line: 63, column: 3, scope: !371)
!378 = !DILocation(line: 64, column: 12, scope: !379)
!379 = distinct !DILexicalBlock(scope: !374, file: !2, line: 64, column: 5)
!380 = !DILocation(line: 64, column: 10, scope: !379)
!381 = !DILocation(line: 64, column: 17, scope: !382)
!382 = distinct !DILexicalBlock(scope: !379, file: !2, line: 64, column: 5)
!383 = !DILocation(line: 64, column: 21, scope: !382)
!384 = !DILocation(line: 64, column: 19, scope: !382)
!385 = !DILocation(line: 64, column: 5, scope: !379)
!386 = !DILocation(line: 65, column: 27, scope: !382)
!387 = !DILocation(line: 65, column: 30, scope: !382)
!388 = !DILocation(line: 65, column: 31, scope: !382)
!389 = !DILocation(line: 65, column: 28, scope: !382)
!390 = !DILocation(line: 65, column: 37, scope: !382)
!391 = !DILocation(line: 65, column: 35, scope: !382)
!392 = !DILocation(line: 65, column: 17, scope: !382)
!393 = !DILocation(line: 65, column: 43, scope: !382)
!394 = !DILocation(line: 65, column: 41, scope: !382)
!395 = !DILocation(line: 65, column: 7, scope: !382)
!396 = !DILocation(line: 65, column: 9, scope: !382)
!397 = !DILocation(line: 65, column: 12, scope: !382)
!398 = !DILocation(line: 65, column: 15, scope: !382)
!399 = !DILocation(line: 64, column: 26, scope: !382)
!400 = !DILocation(line: 64, column: 5, scope: !382)
!401 = distinct !{!401, !385, !402, !221}
!402 = !DILocation(line: 65, column: 43, scope: !379)
!403 = !DILocation(line: 63, column: 24, scope: !374)
!404 = !DILocation(line: 63, column: 3, scope: !374)
!405 = distinct !{!405, !377, !406, !221}
!406 = !DILocation(line: 65, column: 43, scope: !371)
!407 = !DILocation(line: 66, column: 10, scope: !408)
!408 = distinct !DILexicalBlock(scope: !301, file: !2, line: 66, column: 3)
!409 = !DILocation(line: 66, column: 8, scope: !408)
!410 = !DILocation(line: 66, column: 15, scope: !411)
!411 = distinct !DILexicalBlock(scope: !408, file: !2, line: 66, column: 3)
!412 = !DILocation(line: 66, column: 19, scope: !411)
!413 = !DILocation(line: 66, column: 17, scope: !411)
!414 = !DILocation(line: 66, column: 3, scope: !408)
!415 = !DILocation(line: 67, column: 12, scope: !416)
!416 = distinct !DILexicalBlock(scope: !411, file: !2, line: 67, column: 5)
!417 = !DILocation(line: 67, column: 10, scope: !416)
!418 = !DILocation(line: 67, column: 17, scope: !419)
!419 = distinct !DILexicalBlock(scope: !416, file: !2, line: 67, column: 5)
!420 = !DILocation(line: 67, column: 21, scope: !419)
!421 = !DILocation(line: 67, column: 19, scope: !419)
!422 = !DILocation(line: 67, column: 5, scope: !416)
!423 = !DILocation(line: 68, column: 27, scope: !419)
!424 = !DILocation(line: 68, column: 30, scope: !419)
!425 = !DILocation(line: 68, column: 31, scope: !419)
!426 = !DILocation(line: 68, column: 28, scope: !419)
!427 = !DILocation(line: 68, column: 37, scope: !419)
!428 = !DILocation(line: 68, column: 35, scope: !419)
!429 = !DILocation(line: 68, column: 17, scope: !419)
!430 = !DILocation(line: 68, column: 43, scope: !419)
!431 = !DILocation(line: 68, column: 41, scope: !419)
!432 = !DILocation(line: 68, column: 7, scope: !419)
!433 = !DILocation(line: 68, column: 9, scope: !419)
!434 = !DILocation(line: 68, column: 12, scope: !419)
!435 = !DILocation(line: 68, column: 15, scope: !419)
!436 = !DILocation(line: 67, column: 26, scope: !419)
!437 = !DILocation(line: 67, column: 5, scope: !419)
!438 = distinct !{!438, !422, !439, !221}
!439 = !DILocation(line: 68, column: 43, scope: !416)
!440 = !DILocation(line: 66, column: 24, scope: !411)
!441 = !DILocation(line: 66, column: 3, scope: !411)
!442 = distinct !{!442, !414, !443, !221}
!443 = !DILocation(line: 68, column: 43, scope: !408)
!444 = !DILocation(line: 69, column: 1, scope: !301)
!445 = distinct !DISubprogram(name: "kernel_gemm", scope: !2, file: !2, line: 150, type: !446, scopeLine: 156, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !85)
!446 = !DISubroutineType(types: !447)
!447 = !{null, !82, !82, !82, !55, !55, !160, !162, !160}
!448 = !DILocalVariable(name: "ni", arg: 1, scope: !445, file: !2, line: 150, type: !82)
!449 = !DILocation(line: 150, column: 22, scope: !445)
!450 = !DILocalVariable(name: "nj", arg: 2, scope: !445, file: !2, line: 150, type: !82)
!451 = !DILocation(line: 150, column: 30, scope: !445)
!452 = !DILocalVariable(name: "nk", arg: 3, scope: !445, file: !2, line: 150, type: !82)
!453 = !DILocation(line: 150, column: 38, scope: !445)
!454 = !DILocalVariable(name: "alpha", arg: 4, scope: !445, file: !2, line: 151, type: !55)
!455 = !DILocation(line: 151, column: 14, scope: !445)
!456 = !DILocalVariable(name: "beta", arg: 5, scope: !445, file: !2, line: 152, type: !55)
!457 = !DILocation(line: 152, column: 14, scope: !445)
!458 = !DILocalVariable(name: "C", arg: 6, scope: !445, file: !2, line: 153, type: !160)
!459 = !DILocation(line: 153, column: 14, scope: !445)
!460 = !DILocalVariable(name: "A", arg: 7, scope: !445, file: !2, line: 154, type: !162)
!461 = !DILocation(line: 154, column: 14, scope: !445)
!462 = !DILocalVariable(name: "B", arg: 8, scope: !445, file: !2, line: 155, type: !160)
!463 = !DILocation(line: 155, column: 14, scope: !445)
!464 = !DILocalVariable(name: "i", scope: !445, file: !2, line: 157, type: !82)
!465 = !DILocation(line: 157, column: 7, scope: !445)
!466 = !DILocalVariable(name: "j", scope: !445, file: !2, line: 157, type: !82)
!467 = !DILocation(line: 157, column: 10, scope: !445)
!468 = !DILocalVariable(name: "k", scope: !445, file: !2, line: 157, type: !82)
!469 = !DILocation(line: 157, column: 13, scope: !445)
!470 = !DILocation(line: 167, column: 10, scope: !471)
!471 = distinct !DILexicalBlock(scope: !445, file: !2, line: 167, column: 3)
!472 = !DILocation(line: 167, column: 8, scope: !471)
!473 = !DILocation(line: 167, column: 15, scope: !474)
!474 = distinct !DILexicalBlock(scope: !471, file: !2, line: 167, column: 3)
!475 = !DILocation(line: 167, column: 19, scope: !474)
!476 = !DILocation(line: 167, column: 17, scope: !474)
!477 = !DILocation(line: 167, column: 3, scope: !471)
!478 = !DILocation(line: 168, column: 12, scope: !479)
!479 = distinct !DILexicalBlock(scope: !480, file: !2, line: 168, column: 5)
!480 = distinct !DILexicalBlock(scope: !474, file: !2, line: 167, column: 32)
!481 = !DILocation(line: 168, column: 10, scope: !479)
!482 = !DILocation(line: 168, column: 17, scope: !483)
!483 = distinct !DILexicalBlock(scope: !479, file: !2, line: 168, column: 5)
!484 = !DILocation(line: 168, column: 21, scope: !483)
!485 = !DILocation(line: 168, column: 19, scope: !483)
!486 = !DILocation(line: 168, column: 5, scope: !479)
!487 = !DILocation(line: 169, column: 13, scope: !483)
!488 = !DILocation(line: 169, column: 2, scope: !483)
!489 = !DILocation(line: 169, column: 4, scope: !483)
!490 = !DILocation(line: 169, column: 7, scope: !483)
!491 = !DILocation(line: 169, column: 10, scope: !483)
!492 = !DILocation(line: 168, column: 30, scope: !483)
!493 = !DILocation(line: 168, column: 5, scope: !483)
!494 = distinct !{!494, !486, !495, !221}
!495 = !DILocation(line: 169, column: 13, scope: !479)
!496 = !DILocation(line: 170, column: 12, scope: !497)
!497 = distinct !DILexicalBlock(scope: !480, file: !2, line: 170, column: 5)
!498 = !DILocation(line: 170, column: 10, scope: !497)
!499 = !DILocation(line: 170, column: 17, scope: !500)
!500 = distinct !DILexicalBlock(scope: !497, file: !2, line: 170, column: 5)
!501 = !DILocation(line: 170, column: 21, scope: !500)
!502 = !DILocation(line: 170, column: 19, scope: !500)
!503 = !DILocation(line: 170, column: 5, scope: !497)
!504 = !DILocation(line: 171, column: 15, scope: !505)
!505 = distinct !DILexicalBlock(scope: !506, file: !2, line: 171, column: 8)
!506 = distinct !DILexicalBlock(scope: !500, file: !2, line: 170, column: 34)
!507 = !DILocation(line: 171, column: 13, scope: !505)
!508 = !DILocation(line: 171, column: 20, scope: !509)
!509 = distinct !DILexicalBlock(scope: !505, file: !2, line: 171, column: 8)
!510 = !DILocation(line: 171, column: 24, scope: !509)
!511 = !DILocation(line: 171, column: 22, scope: !509)
!512 = !DILocation(line: 171, column: 8, scope: !505)
!513 = !DILocation(line: 172, column: 15, scope: !509)
!514 = !DILocation(line: 172, column: 23, scope: !509)
!515 = !DILocation(line: 172, column: 25, scope: !509)
!516 = !DILocation(line: 172, column: 28, scope: !509)
!517 = !DILocation(line: 172, column: 21, scope: !509)
!518 = !DILocation(line: 172, column: 33, scope: !509)
!519 = !DILocation(line: 172, column: 35, scope: !509)
!520 = !DILocation(line: 172, column: 38, scope: !509)
!521 = !DILocation(line: 172, column: 4, scope: !509)
!522 = !DILocation(line: 172, column: 6, scope: !509)
!523 = !DILocation(line: 172, column: 9, scope: !509)
!524 = !DILocation(line: 172, column: 12, scope: !509)
!525 = !DILocation(line: 171, column: 33, scope: !509)
!526 = !DILocation(line: 171, column: 8, scope: !509)
!527 = distinct !{!527, !512, !528, !221}
!528 = !DILocation(line: 172, column: 39, scope: !505)
!529 = !DILocation(line: 173, column: 5, scope: !506)
!530 = !DILocation(line: 170, column: 30, scope: !500)
!531 = !DILocation(line: 170, column: 5, scope: !500)
!532 = distinct !{!532, !503, !533, !221}
!533 = !DILocation(line: 173, column: 5, scope: !497)
!534 = !DILocation(line: 174, column: 3, scope: !480)
!535 = !DILocation(line: 167, column: 28, scope: !474)
!536 = !DILocation(line: 167, column: 3, scope: !474)
!537 = distinct !{!537, !477, !538, !221}
!538 = !DILocation(line: 174, column: 3, scope: !471)
!539 = !DILocation(line: 177, column: 1, scope: !445)
!540 = distinct !DISubprogram(name: "kernel_gemm_double", scope: !2, file: !2, line: 180, type: !541, scopeLine: 186, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !85)
!541 = !DISubroutineType(types: !542)
!542 = !{null, !82, !82, !82, !66, !66, !305, !307, !305}
!543 = !DILocalVariable(name: "ni", arg: 1, scope: !540, file: !2, line: 180, type: !82)
!544 = !DILocation(line: 180, column: 29, scope: !540)
!545 = !DILocalVariable(name: "nj", arg: 2, scope: !540, file: !2, line: 180, type: !82)
!546 = !DILocation(line: 180, column: 37, scope: !540)
!547 = !DILocalVariable(name: "nk", arg: 3, scope: !540, file: !2, line: 180, type: !82)
!548 = !DILocation(line: 180, column: 45, scope: !540)
!549 = !DILocalVariable(name: "alpha", arg: 4, scope: !540, file: !2, line: 181, type: !66)
!550 = !DILocation(line: 181, column: 11, scope: !540)
!551 = !DILocalVariable(name: "beta", arg: 5, scope: !540, file: !2, line: 182, type: !66)
!552 = !DILocation(line: 182, column: 11, scope: !540)
!553 = !DILocalVariable(name: "C", arg: 6, scope: !540, file: !2, line: 183, type: !305)
!554 = !DILocation(line: 183, column: 11, scope: !540)
!555 = !DILocalVariable(name: "A", arg: 7, scope: !540, file: !2, line: 184, type: !307)
!556 = !DILocation(line: 184, column: 11, scope: !540)
!557 = !DILocalVariable(name: "B", arg: 8, scope: !540, file: !2, line: 185, type: !305)
!558 = !DILocation(line: 185, column: 11, scope: !540)
!559 = !DILocalVariable(name: "i", scope: !540, file: !2, line: 187, type: !82)
!560 = !DILocation(line: 187, column: 7, scope: !540)
!561 = !DILocalVariable(name: "j", scope: !540, file: !2, line: 187, type: !82)
!562 = !DILocation(line: 187, column: 10, scope: !540)
!563 = !DILocalVariable(name: "k", scope: !540, file: !2, line: 187, type: !82)
!564 = !DILocation(line: 187, column: 13, scope: !540)
!565 = !DILocation(line: 190, column: 10, scope: !566)
!566 = distinct !DILexicalBlock(scope: !540, file: !2, line: 190, column: 3)
!567 = !DILocation(line: 190, column: 8, scope: !566)
!568 = !DILocation(line: 190, column: 15, scope: !569)
!569 = distinct !DILexicalBlock(scope: !566, file: !2, line: 190, column: 3)
!570 = !DILocation(line: 190, column: 19, scope: !569)
!571 = !DILocation(line: 190, column: 17, scope: !569)
!572 = !DILocation(line: 190, column: 3, scope: !566)
!573 = !DILocation(line: 191, column: 12, scope: !574)
!574 = distinct !DILexicalBlock(scope: !575, file: !2, line: 191, column: 5)
!575 = distinct !DILexicalBlock(scope: !569, file: !2, line: 190, column: 32)
!576 = !DILocation(line: 191, column: 10, scope: !574)
!577 = !DILocation(line: 191, column: 17, scope: !578)
!578 = distinct !DILexicalBlock(scope: !574, file: !2, line: 191, column: 5)
!579 = !DILocation(line: 191, column: 21, scope: !578)
!580 = !DILocation(line: 191, column: 19, scope: !578)
!581 = !DILocation(line: 191, column: 5, scope: !574)
!582 = !DILocation(line: 192, column: 13, scope: !578)
!583 = !DILocation(line: 192, column: 2, scope: !578)
!584 = !DILocation(line: 192, column: 4, scope: !578)
!585 = !DILocation(line: 192, column: 7, scope: !578)
!586 = !DILocation(line: 192, column: 10, scope: !578)
!587 = !DILocation(line: 191, column: 30, scope: !578)
!588 = !DILocation(line: 191, column: 5, scope: !578)
!589 = distinct !{!589, !581, !590, !221}
!590 = !DILocation(line: 192, column: 13, scope: !574)
!591 = !DILocation(line: 193, column: 12, scope: !592)
!592 = distinct !DILexicalBlock(scope: !575, file: !2, line: 193, column: 5)
!593 = !DILocation(line: 193, column: 10, scope: !592)
!594 = !DILocation(line: 193, column: 17, scope: !595)
!595 = distinct !DILexicalBlock(scope: !592, file: !2, line: 193, column: 5)
!596 = !DILocation(line: 193, column: 21, scope: !595)
!597 = !DILocation(line: 193, column: 19, scope: !595)
!598 = !DILocation(line: 193, column: 5, scope: !592)
!599 = !DILocation(line: 194, column: 15, scope: !600)
!600 = distinct !DILexicalBlock(scope: !601, file: !2, line: 194, column: 8)
!601 = distinct !DILexicalBlock(scope: !595, file: !2, line: 193, column: 34)
!602 = !DILocation(line: 194, column: 13, scope: !600)
!603 = !DILocation(line: 194, column: 20, scope: !604)
!604 = distinct !DILexicalBlock(scope: !600, file: !2, line: 194, column: 8)
!605 = !DILocation(line: 194, column: 24, scope: !604)
!606 = !DILocation(line: 194, column: 22, scope: !604)
!607 = !DILocation(line: 194, column: 8, scope: !600)
!608 = !DILocation(line: 195, column: 15, scope: !604)
!609 = !DILocation(line: 195, column: 23, scope: !604)
!610 = !DILocation(line: 195, column: 25, scope: !604)
!611 = !DILocation(line: 195, column: 28, scope: !604)
!612 = !DILocation(line: 195, column: 21, scope: !604)
!613 = !DILocation(line: 195, column: 33, scope: !604)
!614 = !DILocation(line: 195, column: 35, scope: !604)
!615 = !DILocation(line: 195, column: 38, scope: !604)
!616 = !DILocation(line: 195, column: 4, scope: !604)
!617 = !DILocation(line: 195, column: 6, scope: !604)
!618 = !DILocation(line: 195, column: 9, scope: !604)
!619 = !DILocation(line: 195, column: 12, scope: !604)
!620 = !DILocation(line: 194, column: 33, scope: !604)
!621 = !DILocation(line: 194, column: 8, scope: !604)
!622 = distinct !{!622, !607, !623, !221}
!623 = !DILocation(line: 195, column: 39, scope: !600)
!624 = !DILocation(line: 196, column: 5, scope: !601)
!625 = !DILocation(line: 193, column: 30, scope: !595)
!626 = !DILocation(line: 193, column: 5, scope: !595)
!627 = distinct !{!627, !598, !628, !221}
!628 = !DILocation(line: 196, column: 5, scope: !592)
!629 = !DILocation(line: 197, column: 3, scope: !575)
!630 = !DILocation(line: 190, column: 28, scope: !569)
!631 = !DILocation(line: 190, column: 3, scope: !569)
!632 = distinct !{!632, !572, !633, !221}
!633 = !DILocation(line: 197, column: 3, scope: !566)
!634 = !DILocation(line: 200, column: 1, scope: !540)
!635 = distinct !DISubprogram(name: "print_array", scope: !2, file: !2, line: 74, type: !636, scopeLine: 76, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !85)
!636 = !DISubroutineType(types: !637)
!637 = !{null, !82, !82, !160, !305}
!638 = !DILocalVariable(name: "ni", arg: 1, scope: !635, file: !2, line: 74, type: !82)
!639 = !DILocation(line: 74, column: 22, scope: !635)
!640 = !DILocalVariable(name: "nj", arg: 2, scope: !635, file: !2, line: 74, type: !82)
!641 = !DILocation(line: 74, column: 30, scope: !635)
!642 = !DILocalVariable(name: "C", arg: 3, scope: !635, file: !2, line: 75, type: !160)
!643 = !DILocation(line: 75, column: 14, scope: !635)
!644 = !DILocalVariable(name: "C_double", arg: 4, scope: !635, file: !2, line: 75, type: !305)
!645 = !DILocation(line: 75, column: 50, scope: !635)
!646 = !DILocalVariable(name: "i", scope: !635, file: !2, line: 77, type: !82)
!647 = !DILocation(line: 77, column: 7, scope: !635)
!648 = !DILocalVariable(name: "j", scope: !635, file: !2, line: 77, type: !82)
!649 = !DILocation(line: 77, column: 10, scope: !635)
!650 = !DILocalVariable(name: "max_value", scope: !635, file: !2, line: 79, type: !55)
!651 = !DILocation(line: 79, column: 13, scope: !635)
!652 = !DILocalVariable(name: "sum", scope: !635, file: !2, line: 80, type: !55)
!653 = !DILocation(line: 80, column: 13, scope: !635)
!654 = !DILocalVariable(name: "norm", scope: !635, file: !2, line: 81, type: !55)
!655 = !DILocation(line: 81, column: 13, scope: !635)
!656 = !DILocalVariable(name: "max_value_double", scope: !635, file: !2, line: 83, type: !66)
!657 = !DILocation(line: 83, column: 10, scope: !635)
!658 = !DILocalVariable(name: "sum_double", scope: !635, file: !2, line: 84, type: !66)
!659 = !DILocation(line: 84, column: 10, scope: !635)
!660 = !DILocalVariable(name: "norm_double", scope: !635, file: !2, line: 85, type: !66)
!661 = !DILocation(line: 85, column: 10, scope: !635)
!662 = !DILocation(line: 87, column: 3, scope: !635)
!663 = !DILocation(line: 88, column: 3, scope: !635)
!664 = !DILocation(line: 95, column: 10, scope: !665)
!665 = distinct !DILexicalBlock(scope: !635, file: !2, line: 95, column: 3)
!666 = !DILocation(line: 95, column: 8, scope: !665)
!667 = !DILocation(line: 95, column: 15, scope: !668)
!668 = distinct !DILexicalBlock(scope: !665, file: !2, line: 95, column: 3)
!669 = !DILocation(line: 95, column: 19, scope: !668)
!670 = !DILocation(line: 95, column: 17, scope: !668)
!671 = !DILocation(line: 95, column: 3, scope: !665)
!672 = !DILocation(line: 96, column: 12, scope: !673)
!673 = distinct !DILexicalBlock(scope: !668, file: !2, line: 96, column: 5)
!674 = !DILocation(line: 96, column: 10, scope: !673)
!675 = !DILocation(line: 96, column: 17, scope: !676)
!676 = distinct !DILexicalBlock(scope: !673, file: !2, line: 96, column: 5)
!677 = !DILocation(line: 96, column: 21, scope: !676)
!678 = !DILocation(line: 96, column: 19, scope: !676)
!679 = !DILocation(line: 96, column: 5, scope: !673)
!680 = !DILocalVariable(name: "value", scope: !681, file: !2, line: 97, type: !55)
!681 = distinct !DILexicalBlock(scope: !676, file: !2, line: 96, column: 30)
!682 = !DILocation(line: 97, column: 17, scope: !681)
!683 = !DILocation(line: 97, column: 25, scope: !681)
!684 = !DILocation(line: 97, column: 27, scope: !681)
!685 = !DILocation(line: 97, column: 30, scope: !681)
!686 = !DILocalVariable(name: "value_double", scope: !681, file: !2, line: 98, type: !66)
!687 = !DILocation(line: 98, column: 14, scope: !681)
!688 = !DILocation(line: 98, column: 29, scope: !681)
!689 = !DILocation(line: 98, column: 38, scope: !681)
!690 = !DILocation(line: 98, column: 41, scope: !681)
!691 = !DILocation(line: 100, column: 9, scope: !692)
!692 = distinct !DILexicalBlock(scope: !681, file: !2, line: 100, column: 9)
!693 = !DILocation(line: 100, column: 15, scope: !692)
!694 = !DILocation(line: 101, column: 16, scope: !692)
!695 = !DILocation(line: 101, column: 15, scope: !692)
!696 = !DILocation(line: 101, column: 13, scope: !692)
!697 = !DILocation(line: 101, column: 7, scope: !692)
!698 = !DILocation(line: 103, column: 9, scope: !699)
!699 = distinct !DILexicalBlock(scope: !681, file: !2, line: 103, column: 9)
!700 = !DILocation(line: 103, column: 22, scope: !699)
!701 = !DILocation(line: 104, column: 23, scope: !699)
!702 = !DILocation(line: 104, column: 22, scope: !699)
!703 = !DILocation(line: 104, column: 20, scope: !699)
!704 = !DILocation(line: 104, column: 7, scope: !699)
!705 = !DILocation(line: 106, column: 9, scope: !706)
!706 = distinct !DILexicalBlock(scope: !681, file: !2, line: 106, column: 9)
!707 = !DILocation(line: 106, column: 17, scope: !706)
!708 = !DILocation(line: 106, column: 15, scope: !706)
!709 = !DILocation(line: 107, column: 19, scope: !710)
!710 = distinct !DILexicalBlock(scope: !706, file: !2, line: 106, column: 28)
!711 = !DILocation(line: 107, column: 17, scope: !710)
!712 = !DILocation(line: 108, column: 5, scope: !710)
!713 = !DILocation(line: 109, column: 9, scope: !714)
!714 = distinct !DILexicalBlock(scope: !681, file: !2, line: 109, column: 9)
!715 = !DILocation(line: 109, column: 24, scope: !714)
!716 = !DILocation(line: 109, column: 22, scope: !714)
!717 = !DILocation(line: 110, column: 26, scope: !718)
!718 = distinct !DILexicalBlock(scope: !714, file: !2, line: 109, column: 42)
!719 = !DILocation(line: 110, column: 24, scope: !718)
!720 = !DILocation(line: 111, column: 5, scope: !718)
!721 = !DILocation(line: 112, column: 3, scope: !681)
!722 = !DILocation(line: 96, column: 26, scope: !676)
!723 = !DILocation(line: 96, column: 5, scope: !676)
!724 = distinct !{!724, !679, !725, !221}
!725 = !DILocation(line: 112, column: 3, scope: !673)
!726 = !DILocation(line: 95, column: 24, scope: !668)
!727 = !DILocation(line: 95, column: 3, scope: !668)
!728 = distinct !{!728, !671, !729, !221}
!729 = !DILocation(line: 112, column: 3, scope: !665)
!730 = !DILocation(line: 114, column: 7, scope: !731)
!731 = distinct !DILexicalBlock(scope: !635, file: !2, line: 114, column: 7)
!732 = !DILocation(line: 114, column: 17, scope: !731)
!733 = !DILocation(line: 115, column: 12, scope: !734)
!734 = distinct !DILexicalBlock(scope: !735, file: !2, line: 115, column: 5)
!735 = distinct !DILexicalBlock(scope: !731, file: !2, line: 114, column: 23)
!736 = !DILocation(line: 115, column: 10, scope: !734)
!737 = !DILocation(line: 115, column: 17, scope: !738)
!738 = distinct !DILexicalBlock(scope: !734, file: !2, line: 115, column: 5)
!739 = !DILocation(line: 115, column: 21, scope: !738)
!740 = !DILocation(line: 115, column: 19, scope: !738)
!741 = !DILocation(line: 115, column: 5, scope: !734)
!742 = !DILocation(line: 116, column: 14, scope: !743)
!743 = distinct !DILexicalBlock(scope: !744, file: !2, line: 116, column: 7)
!744 = distinct !DILexicalBlock(scope: !738, file: !2, line: 115, column: 30)
!745 = !DILocation(line: 116, column: 12, scope: !743)
!746 = !DILocation(line: 116, column: 19, scope: !747)
!747 = distinct !DILexicalBlock(scope: !743, file: !2, line: 116, column: 7)
!748 = !DILocation(line: 116, column: 23, scope: !747)
!749 = !DILocation(line: 116, column: 21, scope: !747)
!750 = !DILocation(line: 116, column: 7, scope: !743)
!751 = !DILocalVariable(name: "scaled", scope: !752, file: !2, line: 117, type: !55)
!752 = distinct !DILexicalBlock(scope: !747, file: !2, line: 116, column: 32)
!753 = !DILocation(line: 117, column: 19, scope: !752)
!754 = !DILocation(line: 117, column: 28, scope: !752)
!755 = !DILocation(line: 117, column: 30, scope: !752)
!756 = !DILocation(line: 117, column: 33, scope: !752)
!757 = !DILocation(line: 117, column: 38, scope: !752)
!758 = !DILocation(line: 117, column: 36, scope: !752)
!759 = !DILocation(line: 118, column: 16, scope: !752)
!760 = !DILocation(line: 118, column: 25, scope: !752)
!761 = !DILocation(line: 118, column: 13, scope: !752)
!762 = !DILocation(line: 119, column: 7, scope: !752)
!763 = !DILocation(line: 116, column: 28, scope: !747)
!764 = !DILocation(line: 116, column: 7, scope: !747)
!765 = distinct !{!765, !750, !766, !221}
!766 = !DILocation(line: 119, column: 7, scope: !743)
!767 = !DILocation(line: 120, column: 5, scope: !744)
!768 = !DILocation(line: 115, column: 26, scope: !738)
!769 = !DILocation(line: 115, column: 5, scope: !738)
!770 = distinct !{!770, !741, !771, !221}
!771 = !DILocation(line: 120, column: 5, scope: !734)
!772 = !DILocation(line: 121, column: 12, scope: !735)
!773 = !DILocation(line: 121, column: 10, scope: !735)
!774 = !DILocation(line: 122, column: 3, scope: !735)
!775 = !DILocation(line: 124, column: 7, scope: !776)
!776 = distinct !DILexicalBlock(scope: !635, file: !2, line: 124, column: 7)
!777 = !DILocation(line: 124, column: 24, scope: !776)
!778 = !DILocation(line: 125, column: 12, scope: !779)
!779 = distinct !DILexicalBlock(scope: !780, file: !2, line: 125, column: 5)
!780 = distinct !DILexicalBlock(scope: !776, file: !2, line: 124, column: 30)
!781 = !DILocation(line: 125, column: 10, scope: !779)
!782 = !DILocation(line: 125, column: 17, scope: !783)
!783 = distinct !DILexicalBlock(scope: !779, file: !2, line: 125, column: 5)
!784 = !DILocation(line: 125, column: 21, scope: !783)
!785 = !DILocation(line: 125, column: 19, scope: !783)
!786 = !DILocation(line: 125, column: 5, scope: !779)
!787 = !DILocation(line: 126, column: 14, scope: !788)
!788 = distinct !DILexicalBlock(scope: !789, file: !2, line: 126, column: 7)
!789 = distinct !DILexicalBlock(scope: !783, file: !2, line: 125, column: 30)
!790 = !DILocation(line: 126, column: 12, scope: !788)
!791 = !DILocation(line: 126, column: 19, scope: !792)
!792 = distinct !DILexicalBlock(scope: !788, file: !2, line: 126, column: 7)
!793 = !DILocation(line: 126, column: 23, scope: !792)
!794 = !DILocation(line: 126, column: 21, scope: !792)
!795 = !DILocation(line: 126, column: 7, scope: !788)
!796 = !DILocalVariable(name: "scaled", scope: !797, file: !2, line: 127, type: !66)
!797 = distinct !DILexicalBlock(scope: !792, file: !2, line: 126, column: 32)
!798 = !DILocation(line: 127, column: 16, scope: !797)
!799 = !DILocation(line: 127, column: 25, scope: !797)
!800 = !DILocation(line: 127, column: 34, scope: !797)
!801 = !DILocation(line: 127, column: 37, scope: !797)
!802 = !DILocation(line: 127, column: 42, scope: !797)
!803 = !DILocation(line: 127, column: 40, scope: !797)
!804 = !DILocation(line: 128, column: 23, scope: !797)
!805 = !DILocation(line: 128, column: 32, scope: !797)
!806 = !DILocation(line: 128, column: 20, scope: !797)
!807 = !DILocation(line: 129, column: 7, scope: !797)
!808 = !DILocation(line: 126, column: 28, scope: !792)
!809 = !DILocation(line: 126, column: 7, scope: !792)
!810 = distinct !{!810, !795, !811, !221}
!811 = !DILocation(line: 129, column: 7, scope: !788)
!812 = !DILocation(line: 130, column: 5, scope: !789)
!813 = !DILocation(line: 125, column: 26, scope: !783)
!814 = !DILocation(line: 125, column: 5, scope: !783)
!815 = distinct !{!815, !786, !816, !221}
!816 = !DILocation(line: 130, column: 5, scope: !779)
!817 = !DILocation(line: 131, column: 24, scope: !780)
!818 = !DILocation(line: 131, column: 19, scope: !780)
!819 = !DILocation(line: 131, column: 17, scope: !780)
!820 = !DILocation(line: 132, column: 3, scope: !780)
!821 = !DILocation(line: 134, column: 12, scope: !635)
!822 = !DILocation(line: 134, column: 61, scope: !635)
!823 = !DILocation(line: 134, column: 3, scope: !635)
!824 = !DILocation(line: 135, column: 12, scope: !635)
!825 = !DILocation(line: 135, column: 56, scope: !635)
!826 = !DILocation(line: 135, column: 3, scope: !635)
!827 = !DILocation(line: 136, column: 12, scope: !635)
!828 = !DILocation(line: 136, column: 69, scope: !635)
!829 = !DILocation(line: 136, column: 3, scope: !635)
!830 = !DILocation(line: 137, column: 12, scope: !635)
!831 = !DILocation(line: 137, column: 64, scope: !635)
!832 = !DILocation(line: 137, column: 3, scope: !635)
!833 = !DILocalVariable(name: "norm_error", scope: !635, file: !2, line: 139, type: !66)
!834 = !DILocation(line: 139, column: 10, scope: !635)
!835 = !DILocation(line: 139, column: 23, scope: !635)
!836 = !DILocation(line: 139, column: 45, scope: !635)
!837 = !DILocation(line: 139, column: 37, scope: !635)
!838 = !DILocation(line: 139, column: 35, scope: !635)
!839 = !DILocation(line: 140, column: 12, scope: !635)
!840 = !DILocation(line: 140, column: 58, scope: !635)
!841 = !DILocation(line: 140, column: 3, scope: !635)
!842 = !DILocation(line: 142, column: 3, scope: !635)
!843 = !DILocation(line: 143, column: 3, scope: !635)
!844 = !DILocation(line: 144, column: 1, scope: !635)

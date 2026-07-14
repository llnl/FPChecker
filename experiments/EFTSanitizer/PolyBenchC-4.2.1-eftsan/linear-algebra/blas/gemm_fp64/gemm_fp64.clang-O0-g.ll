; ModuleID = 'gemm.c'
source_filename = "gemm.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@stderr = external dso_local global ptr, align 8
@.str = private unnamed_addr constant [23 x i8] c"==BEGIN DUMP_ARRAYS==\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [15 x i8] c"begin dump: %s\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [3 x i8] c"C\0A\00", align 1, !dbg !12
@.str.3 = private unnamed_addr constant [23 x i8] c"Max value in C: %.17e\0A\00", align 1, !dbg !17
@.str.4 = private unnamed_addr constant [18 x i8] c"Norm of C: %.17e\0A\00", align 1, !dbg !19
@.str.5 = private unnamed_addr constant [34 x i8] c"Max value in long_double: %.21Le\0A\00", align 1, !dbg !24
@.str.6 = private unnamed_addr constant [29 x i8] c"Norm of long_double: %.21Le\0A\00", align 1, !dbg !29
@.str.7 = private unnamed_addr constant [19 x i8] c"Norm error: %.17e\0A\00", align 1, !dbg !34
@.str.8 = private unnamed_addr constant [17 x i8] c"\0Aend   dump: %s\0A\00", align 1, !dbg !39
@.str.9 = private unnamed_addr constant [2 x i8] c"C\00", align 1, !dbg !44
@.str.10 = private unnamed_addr constant [23 x i8] c"==END   DUMP_ARRAYS==\0A\00", align 1, !dbg !49

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 noundef %0, ptr noundef %1) #0 !dbg !81 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca ptr, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca double, align 8
  %10 = alloca double, align 8
  %11 = alloca x86_fp80, align 16
  %12 = alloca x86_fp80, align 16
  %13 = alloca ptr, align 8
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca ptr, align 8
  %18 = alloca ptr, align 8
  store i32 0, ptr %3, align 4
  store i32 %0, ptr %4, align 4
    #dbg_declare(ptr %4, !88, !DIExpression(), !89)
  store ptr %1, ptr %5, align 8
    #dbg_declare(ptr %5, !90, !DIExpression(), !91)
    #dbg_declare(ptr %6, !92, !DIExpression(), !93)
  store i32 20, ptr %6, align 4, !dbg !93
    #dbg_declare(ptr %7, !94, !DIExpression(), !95)
  store i32 25, ptr %7, align 4, !dbg !95
    #dbg_declare(ptr %8, !96, !DIExpression(), !97)
  store i32 30, ptr %8, align 4, !dbg !97
    #dbg_declare(ptr %9, !98, !DIExpression(), !99)
    #dbg_declare(ptr %10, !100, !DIExpression(), !101)
    #dbg_declare(ptr %11, !102, !DIExpression(), !103)
    #dbg_declare(ptr %12, !104, !DIExpression(), !105)
    #dbg_declare(ptr %13, !106, !DIExpression(), !107)
  %19 = call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 8), !dbg !107
  store ptr %19, ptr %13, align 8, !dbg !107
    #dbg_declare(ptr %14, !108, !DIExpression(), !109)
  %20 = call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 8), !dbg !109
  store ptr %20, ptr %14, align 8, !dbg !109
    #dbg_declare(ptr %15, !110, !DIExpression(), !111)
  %21 = call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 8), !dbg !111
  store ptr %21, ptr %15, align 8, !dbg !111
    #dbg_declare(ptr %16, !112, !DIExpression(), !113)
  %22 = call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 16), !dbg !113
  store ptr %22, ptr %16, align 8, !dbg !113
    #dbg_declare(ptr %17, !114, !DIExpression(), !115)
  %23 = call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 16), !dbg !115
  store ptr %23, ptr %17, align 8, !dbg !115
    #dbg_declare(ptr %18, !116, !DIExpression(), !117)
  %24 = call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 16), !dbg !117
  store ptr %24, ptr %18, align 8, !dbg !117
  %25 = load i32, ptr %6, align 4, !dbg !118
  %26 = load i32, ptr %7, align 4, !dbg !119
  %27 = load i32, ptr %8, align 4, !dbg !120
  %28 = load ptr, ptr %13, align 8, !dbg !121
  %29 = getelementptr inbounds [20 x [25 x double]], ptr %28, i64 0, i64 0, !dbg !121
  %30 = load ptr, ptr %14, align 8, !dbg !122
  %31 = getelementptr inbounds [20 x [30 x double]], ptr %30, i64 0, i64 0, !dbg !122
  %32 = load ptr, ptr %15, align 8, !dbg !123
  %33 = getelementptr inbounds [30 x [25 x double]], ptr %32, i64 0, i64 0, !dbg !123
  call void @init_array(i32 noundef %25, i32 noundef %26, i32 noundef %27, ptr noundef %9, ptr noundef %10, ptr noundef %29, ptr noundef %31, ptr noundef %33), !dbg !124
  %34 = load i32, ptr %6, align 4, !dbg !125
  %35 = load i32, ptr %7, align 4, !dbg !126
  %36 = load i32, ptr %8, align 4, !dbg !127
  %37 = load ptr, ptr %16, align 8, !dbg !128
  %38 = getelementptr inbounds [20 x [25 x x86_fp80]], ptr %37, i64 0, i64 0, !dbg !128
  %39 = load ptr, ptr %17, align 8, !dbg !129
  %40 = getelementptr inbounds [20 x [30 x x86_fp80]], ptr %39, i64 0, i64 0, !dbg !129
  %41 = load ptr, ptr %18, align 8, !dbg !130
  %42 = getelementptr inbounds [30 x [25 x x86_fp80]], ptr %41, i64 0, i64 0, !dbg !130
  call void @init_array_long_double(i32 noundef %34, i32 noundef %35, i32 noundef %36, ptr noundef %11, ptr noundef %12, ptr noundef %38, ptr noundef %40, ptr noundef %42), !dbg !131
  %43 = load i32, ptr %6, align 4, !dbg !132
  %44 = load i32, ptr %7, align 4, !dbg !133
  %45 = load i32, ptr %8, align 4, !dbg !134
  %46 = load double, ptr %9, align 8, !dbg !135
  %47 = load double, ptr %10, align 8, !dbg !136
  %48 = load ptr, ptr %13, align 8, !dbg !137
  %49 = getelementptr inbounds [20 x [25 x double]], ptr %48, i64 0, i64 0, !dbg !137
  %50 = load ptr, ptr %14, align 8, !dbg !138
  %51 = getelementptr inbounds [20 x [30 x double]], ptr %50, i64 0, i64 0, !dbg !138
  %52 = load ptr, ptr %15, align 8, !dbg !139
  %53 = getelementptr inbounds [30 x [25 x double]], ptr %52, i64 0, i64 0, !dbg !139
  call void @kernel_gemm(i32 noundef %43, i32 noundef %44, i32 noundef %45, double noundef %46, double noundef %47, ptr noundef %49, ptr noundef %51, ptr noundef %53), !dbg !140
  %54 = load i32, ptr %6, align 4, !dbg !141
  %55 = load i32, ptr %7, align 4, !dbg !142
  %56 = load i32, ptr %8, align 4, !dbg !143
  %57 = load x86_fp80, ptr %11, align 16, !dbg !144
  %58 = load x86_fp80, ptr %12, align 16, !dbg !145
  %59 = load ptr, ptr %16, align 8, !dbg !146
  %60 = getelementptr inbounds [20 x [25 x x86_fp80]], ptr %59, i64 0, i64 0, !dbg !146
  %61 = load ptr, ptr %17, align 8, !dbg !147
  %62 = getelementptr inbounds [20 x [30 x x86_fp80]], ptr %61, i64 0, i64 0, !dbg !147
  %63 = load ptr, ptr %18, align 8, !dbg !148
  %64 = getelementptr inbounds [30 x [25 x x86_fp80]], ptr %63, i64 0, i64 0, !dbg !148
  call void @kernel_gemm_long_double(i32 noundef %54, i32 noundef %55, i32 noundef %56, x86_fp80 noundef %57, x86_fp80 noundef %58, ptr noundef %60, ptr noundef %62, ptr noundef %64), !dbg !149
  %65 = load i32, ptr %6, align 4, !dbg !150
  %66 = load i32, ptr %7, align 4, !dbg !150
  %67 = load ptr, ptr %13, align 8, !dbg !150
  %68 = getelementptr inbounds [20 x [25 x double]], ptr %67, i64 0, i64 0, !dbg !150
  %69 = load ptr, ptr %16, align 8, !dbg !150
  %70 = getelementptr inbounds [20 x [25 x x86_fp80]], ptr %69, i64 0, i64 0, !dbg !150
  call void @print_array(i32 noundef %65, i32 noundef %66, ptr noundef %68, ptr noundef %70), !dbg !150
  %71 = load ptr, ptr %13, align 8, !dbg !151
  call void @free(ptr noundef %71) #4, !dbg !151
  %72 = load ptr, ptr %14, align 8, !dbg !152
  call void @free(ptr noundef %72) #4, !dbg !152
  %73 = load ptr, ptr %15, align 8, !dbg !153
  call void @free(ptr noundef %73) #4, !dbg !153
  %74 = load ptr, ptr %16, align 8, !dbg !154
  call void @free(ptr noundef %74) #4, !dbg !154
  %75 = load ptr, ptr %17, align 8, !dbg !155
  call void @free(ptr noundef %75) #4, !dbg !155
  %76 = load ptr, ptr %18, align 8, !dbg !156
  call void @free(ptr noundef %76) #4, !dbg !156
  ret i32 0, !dbg !157
}

declare dso_local ptr @polybench_alloc_data(i64 noundef, i32 noundef) #1

; Function Attrs: noinline nounwind uwtable
define internal void @init_array(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr noundef %3, ptr noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !158 {
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
    #dbg_declare(ptr %9, !168, !DIExpression(), !169)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !170, !DIExpression(), !171)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !172, !DIExpression(), !173)
  store ptr %3, ptr %12, align 8
    #dbg_declare(ptr %12, !174, !DIExpression(), !175)
  store ptr %4, ptr %13, align 8
    #dbg_declare(ptr %13, !176, !DIExpression(), !177)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !178, !DIExpression(), !179)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !180, !DIExpression(), !181)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !182, !DIExpression(), !183)
    #dbg_declare(ptr %17, !184, !DIExpression(), !185)
    #dbg_declare(ptr %18, !186, !DIExpression(), !187)
  %19 = load ptr, ptr %12, align 8, !dbg !188
  store double 1.500000e+00, ptr %19, align 8, !dbg !189
  %20 = load ptr, ptr %13, align 8, !dbg !190
  store double 1.200000e+00, ptr %20, align 8, !dbg !191
  store i32 0, ptr %17, align 4, !dbg !192
  br label %21, !dbg !194

21:                                               ; preds = %52, %8
  %22 = load i32, ptr %17, align 4, !dbg !195
  %23 = load i32, ptr %9, align 4, !dbg !197
  %24 = icmp slt i32 %22, %23, !dbg !198
  br i1 %24, label %25, label %55, !dbg !199

25:                                               ; preds = %21
  store i32 0, ptr %18, align 4, !dbg !200
  br label %26, !dbg !202

26:                                               ; preds = %48, %25
  %27 = load i32, ptr %18, align 4, !dbg !203
  %28 = load i32, ptr %10, align 4, !dbg !205
  %29 = icmp slt i32 %27, %28, !dbg !206
  br i1 %29, label %30, label %51, !dbg !207

30:                                               ; preds = %26
  %31 = load i32, ptr %17, align 4, !dbg !208
  %32 = load i32, ptr %18, align 4, !dbg !209
  %33 = mul nsw i32 %31, %32, !dbg !210
  %34 = add nsw i32 %33, 1, !dbg !211
  %35 = load i32, ptr %9, align 4, !dbg !212
  %36 = srem i32 %34, %35, !dbg !213
  %37 = sitofp i32 %36 to double, !dbg !214
  %38 = load i32, ptr %9, align 4, !dbg !215
  %39 = sitofp i32 %38 to double, !dbg !215
  %40 = fdiv double %37, %39, !dbg !216
  %41 = load ptr, ptr %14, align 8, !dbg !217
  %42 = load i32, ptr %17, align 4, !dbg !218
  %43 = sext i32 %42 to i64, !dbg !217
  %44 = getelementptr inbounds [25 x double], ptr %41, i64 %43, !dbg !217
  %45 = load i32, ptr %18, align 4, !dbg !219
  %46 = sext i32 %45 to i64, !dbg !217
  %47 = getelementptr inbounds [25 x double], ptr %44, i64 0, i64 %46, !dbg !217
  store double %40, ptr %47, align 8, !dbg !220
  br label %48, !dbg !217

48:                                               ; preds = %30
  %49 = load i32, ptr %18, align 4, !dbg !221
  %50 = add nsw i32 %49, 1, !dbg !221
  store i32 %50, ptr %18, align 4, !dbg !221
  br label %26, !dbg !222, !llvm.loop !223

51:                                               ; preds = %26
  br label %52, !dbg !224

52:                                               ; preds = %51
  %53 = load i32, ptr %17, align 4, !dbg !226
  %54 = add nsw i32 %53, 1, !dbg !226
  store i32 %54, ptr %17, align 4, !dbg !226
  br label %21, !dbg !227, !llvm.loop !228

55:                                               ; preds = %21
  store i32 0, ptr %17, align 4, !dbg !230
  br label %56, !dbg !232

56:                                               ; preds = %87, %55
  %57 = load i32, ptr %17, align 4, !dbg !233
  %58 = load i32, ptr %9, align 4, !dbg !235
  %59 = icmp slt i32 %57, %58, !dbg !236
  br i1 %59, label %60, label %90, !dbg !237

60:                                               ; preds = %56
  store i32 0, ptr %18, align 4, !dbg !238
  br label %61, !dbg !240

61:                                               ; preds = %83, %60
  %62 = load i32, ptr %18, align 4, !dbg !241
  %63 = load i32, ptr %11, align 4, !dbg !243
  %64 = icmp slt i32 %62, %63, !dbg !244
  br i1 %64, label %65, label %86, !dbg !245

65:                                               ; preds = %61
  %66 = load i32, ptr %17, align 4, !dbg !246
  %67 = load i32, ptr %18, align 4, !dbg !247
  %68 = add nsw i32 %67, 1, !dbg !248
  %69 = mul nsw i32 %66, %68, !dbg !249
  %70 = load i32, ptr %11, align 4, !dbg !250
  %71 = srem i32 %69, %70, !dbg !251
  %72 = sitofp i32 %71 to double, !dbg !252
  %73 = load i32, ptr %11, align 4, !dbg !253
  %74 = sitofp i32 %73 to double, !dbg !253
  %75 = fdiv double %72, %74, !dbg !254
  %76 = load ptr, ptr %15, align 8, !dbg !255
  %77 = load i32, ptr %17, align 4, !dbg !256
  %78 = sext i32 %77 to i64, !dbg !255
  %79 = getelementptr inbounds [30 x double], ptr %76, i64 %78, !dbg !255
  %80 = load i32, ptr %18, align 4, !dbg !257
  %81 = sext i32 %80 to i64, !dbg !255
  %82 = getelementptr inbounds [30 x double], ptr %79, i64 0, i64 %81, !dbg !255
  store double %75, ptr %82, align 8, !dbg !258
  br label %83, !dbg !255

83:                                               ; preds = %65
  %84 = load i32, ptr %18, align 4, !dbg !259
  %85 = add nsw i32 %84, 1, !dbg !259
  store i32 %85, ptr %18, align 4, !dbg !259
  br label %61, !dbg !260, !llvm.loop !261

86:                                               ; preds = %61
  br label %87, !dbg !262

87:                                               ; preds = %86
  %88 = load i32, ptr %17, align 4, !dbg !263
  %89 = add nsw i32 %88, 1, !dbg !263
  store i32 %89, ptr %17, align 4, !dbg !263
  br label %56, !dbg !264, !llvm.loop !265

90:                                               ; preds = %56
  store i32 0, ptr %17, align 4, !dbg !267
  br label %91, !dbg !269

91:                                               ; preds = %122, %90
  %92 = load i32, ptr %17, align 4, !dbg !270
  %93 = load i32, ptr %11, align 4, !dbg !272
  %94 = icmp slt i32 %92, %93, !dbg !273
  br i1 %94, label %95, label %125, !dbg !274

95:                                               ; preds = %91
  store i32 0, ptr %18, align 4, !dbg !275
  br label %96, !dbg !277

96:                                               ; preds = %118, %95
  %97 = load i32, ptr %18, align 4, !dbg !278
  %98 = load i32, ptr %10, align 4, !dbg !280
  %99 = icmp slt i32 %97, %98, !dbg !281
  br i1 %99, label %100, label %121, !dbg !282

100:                                              ; preds = %96
  %101 = load i32, ptr %17, align 4, !dbg !283
  %102 = load i32, ptr %18, align 4, !dbg !284
  %103 = add nsw i32 %102, 2, !dbg !285
  %104 = mul nsw i32 %101, %103, !dbg !286
  %105 = load i32, ptr %10, align 4, !dbg !287
  %106 = srem i32 %104, %105, !dbg !288
  %107 = sitofp i32 %106 to double, !dbg !289
  %108 = load i32, ptr %10, align 4, !dbg !290
  %109 = sitofp i32 %108 to double, !dbg !290
  %110 = fdiv double %107, %109, !dbg !291
  %111 = load ptr, ptr %16, align 8, !dbg !292
  %112 = load i32, ptr %17, align 4, !dbg !293
  %113 = sext i32 %112 to i64, !dbg !292
  %114 = getelementptr inbounds [25 x double], ptr %111, i64 %113, !dbg !292
  %115 = load i32, ptr %18, align 4, !dbg !294
  %116 = sext i32 %115 to i64, !dbg !292
  %117 = getelementptr inbounds [25 x double], ptr %114, i64 0, i64 %116, !dbg !292
  store double %110, ptr %117, align 8, !dbg !295
  br label %118, !dbg !292

118:                                              ; preds = %100
  %119 = load i32, ptr %18, align 4, !dbg !296
  %120 = add nsw i32 %119, 1, !dbg !296
  store i32 %120, ptr %18, align 4, !dbg !296
  br label %96, !dbg !297, !llvm.loop !298

121:                                              ; preds = %96
  br label %122, !dbg !299

122:                                              ; preds = %121
  %123 = load i32, ptr %17, align 4, !dbg !300
  %124 = add nsw i32 %123, 1, !dbg !300
  store i32 %124, ptr %17, align 4, !dbg !300
  br label %91, !dbg !301, !llvm.loop !302

125:                                              ; preds = %91
  ret void, !dbg !304
}

; Function Attrs: noinline nounwind uwtable
define internal void @init_array_long_double(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr noundef %3, ptr noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !305 {
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
    #dbg_declare(ptr %9, !313, !DIExpression(), !314)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !315, !DIExpression(), !316)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !317, !DIExpression(), !318)
  store ptr %3, ptr %12, align 8
    #dbg_declare(ptr %12, !319, !DIExpression(), !320)
  store ptr %4, ptr %13, align 8
    #dbg_declare(ptr %13, !321, !DIExpression(), !322)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !323, !DIExpression(), !324)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !325, !DIExpression(), !326)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !327, !DIExpression(), !328)
    #dbg_declare(ptr %17, !329, !DIExpression(), !330)
    #dbg_declare(ptr %18, !331, !DIExpression(), !332)
  %19 = load ptr, ptr %12, align 8, !dbg !333
  store x86_fp80 0xK3FFFC000000000000000, ptr %19, align 16, !dbg !334
  %20 = load ptr, ptr %13, align 8, !dbg !335
  store x86_fp80 0xK3FFF999999999999999A, ptr %20, align 16, !dbg !336
  store i32 0, ptr %17, align 4, !dbg !337
  br label %21, !dbg !339

21:                                               ; preds = %52, %8
  %22 = load i32, ptr %17, align 4, !dbg !340
  %23 = load i32, ptr %9, align 4, !dbg !342
  %24 = icmp slt i32 %22, %23, !dbg !343
  br i1 %24, label %25, label %55, !dbg !344

25:                                               ; preds = %21
  store i32 0, ptr %18, align 4, !dbg !345
  br label %26, !dbg !347

26:                                               ; preds = %48, %25
  %27 = load i32, ptr %18, align 4, !dbg !348
  %28 = load i32, ptr %10, align 4, !dbg !350
  %29 = icmp slt i32 %27, %28, !dbg !351
  br i1 %29, label %30, label %51, !dbg !352

30:                                               ; preds = %26
  %31 = load i32, ptr %17, align 4, !dbg !353
  %32 = load i32, ptr %18, align 4, !dbg !354
  %33 = mul nsw i32 %31, %32, !dbg !355
  %34 = add nsw i32 %33, 1, !dbg !356
  %35 = load i32, ptr %9, align 4, !dbg !357
  %36 = srem i32 %34, %35, !dbg !358
  %37 = sitofp i32 %36 to x86_fp80, !dbg !359
  %38 = load i32, ptr %9, align 4, !dbg !360
  %39 = sitofp i32 %38 to x86_fp80, !dbg !360
  %40 = fdiv x86_fp80 %37, %39, !dbg !361
  %41 = load ptr, ptr %14, align 8, !dbg !362
  %42 = load i32, ptr %17, align 4, !dbg !363
  %43 = sext i32 %42 to i64, !dbg !362
  %44 = getelementptr inbounds [25 x x86_fp80], ptr %41, i64 %43, !dbg !362
  %45 = load i32, ptr %18, align 4, !dbg !364
  %46 = sext i32 %45 to i64, !dbg !362
  %47 = getelementptr inbounds [25 x x86_fp80], ptr %44, i64 0, i64 %46, !dbg !362
  store x86_fp80 %40, ptr %47, align 16, !dbg !365
  br label %48, !dbg !362

48:                                               ; preds = %30
  %49 = load i32, ptr %18, align 4, !dbg !366
  %50 = add nsw i32 %49, 1, !dbg !366
  store i32 %50, ptr %18, align 4, !dbg !366
  br label %26, !dbg !367, !llvm.loop !368

51:                                               ; preds = %26
  br label %52, !dbg !369

52:                                               ; preds = %51
  %53 = load i32, ptr %17, align 4, !dbg !370
  %54 = add nsw i32 %53, 1, !dbg !370
  store i32 %54, ptr %17, align 4, !dbg !370
  br label %21, !dbg !371, !llvm.loop !372

55:                                               ; preds = %21
  store i32 0, ptr %17, align 4, !dbg !374
  br label %56, !dbg !376

56:                                               ; preds = %87, %55
  %57 = load i32, ptr %17, align 4, !dbg !377
  %58 = load i32, ptr %9, align 4, !dbg !379
  %59 = icmp slt i32 %57, %58, !dbg !380
  br i1 %59, label %60, label %90, !dbg !381

60:                                               ; preds = %56
  store i32 0, ptr %18, align 4, !dbg !382
  br label %61, !dbg !384

61:                                               ; preds = %83, %60
  %62 = load i32, ptr %18, align 4, !dbg !385
  %63 = load i32, ptr %11, align 4, !dbg !387
  %64 = icmp slt i32 %62, %63, !dbg !388
  br i1 %64, label %65, label %86, !dbg !389

65:                                               ; preds = %61
  %66 = load i32, ptr %17, align 4, !dbg !390
  %67 = load i32, ptr %18, align 4, !dbg !391
  %68 = add nsw i32 %67, 1, !dbg !392
  %69 = mul nsw i32 %66, %68, !dbg !393
  %70 = load i32, ptr %11, align 4, !dbg !394
  %71 = srem i32 %69, %70, !dbg !395
  %72 = sitofp i32 %71 to x86_fp80, !dbg !396
  %73 = load i32, ptr %11, align 4, !dbg !397
  %74 = sitofp i32 %73 to x86_fp80, !dbg !397
  %75 = fdiv x86_fp80 %72, %74, !dbg !398
  %76 = load ptr, ptr %15, align 8, !dbg !399
  %77 = load i32, ptr %17, align 4, !dbg !400
  %78 = sext i32 %77 to i64, !dbg !399
  %79 = getelementptr inbounds [30 x x86_fp80], ptr %76, i64 %78, !dbg !399
  %80 = load i32, ptr %18, align 4, !dbg !401
  %81 = sext i32 %80 to i64, !dbg !399
  %82 = getelementptr inbounds [30 x x86_fp80], ptr %79, i64 0, i64 %81, !dbg !399
  store x86_fp80 %75, ptr %82, align 16, !dbg !402
  br label %83, !dbg !399

83:                                               ; preds = %65
  %84 = load i32, ptr %18, align 4, !dbg !403
  %85 = add nsw i32 %84, 1, !dbg !403
  store i32 %85, ptr %18, align 4, !dbg !403
  br label %61, !dbg !404, !llvm.loop !405

86:                                               ; preds = %61
  br label %87, !dbg !406

87:                                               ; preds = %86
  %88 = load i32, ptr %17, align 4, !dbg !407
  %89 = add nsw i32 %88, 1, !dbg !407
  store i32 %89, ptr %17, align 4, !dbg !407
  br label %56, !dbg !408, !llvm.loop !409

90:                                               ; preds = %56
  store i32 0, ptr %17, align 4, !dbg !411
  br label %91, !dbg !413

91:                                               ; preds = %122, %90
  %92 = load i32, ptr %17, align 4, !dbg !414
  %93 = load i32, ptr %11, align 4, !dbg !416
  %94 = icmp slt i32 %92, %93, !dbg !417
  br i1 %94, label %95, label %125, !dbg !418

95:                                               ; preds = %91
  store i32 0, ptr %18, align 4, !dbg !419
  br label %96, !dbg !421

96:                                               ; preds = %118, %95
  %97 = load i32, ptr %18, align 4, !dbg !422
  %98 = load i32, ptr %10, align 4, !dbg !424
  %99 = icmp slt i32 %97, %98, !dbg !425
  br i1 %99, label %100, label %121, !dbg !426

100:                                              ; preds = %96
  %101 = load i32, ptr %17, align 4, !dbg !427
  %102 = load i32, ptr %18, align 4, !dbg !428
  %103 = add nsw i32 %102, 2, !dbg !429
  %104 = mul nsw i32 %101, %103, !dbg !430
  %105 = load i32, ptr %10, align 4, !dbg !431
  %106 = srem i32 %104, %105, !dbg !432
  %107 = sitofp i32 %106 to x86_fp80, !dbg !433
  %108 = load i32, ptr %10, align 4, !dbg !434
  %109 = sitofp i32 %108 to x86_fp80, !dbg !434
  %110 = fdiv x86_fp80 %107, %109, !dbg !435
  %111 = load ptr, ptr %16, align 8, !dbg !436
  %112 = load i32, ptr %17, align 4, !dbg !437
  %113 = sext i32 %112 to i64, !dbg !436
  %114 = getelementptr inbounds [25 x x86_fp80], ptr %111, i64 %113, !dbg !436
  %115 = load i32, ptr %18, align 4, !dbg !438
  %116 = sext i32 %115 to i64, !dbg !436
  %117 = getelementptr inbounds [25 x x86_fp80], ptr %114, i64 0, i64 %116, !dbg !436
  store x86_fp80 %110, ptr %117, align 16, !dbg !439
  br label %118, !dbg !436

118:                                              ; preds = %100
  %119 = load i32, ptr %18, align 4, !dbg !440
  %120 = add nsw i32 %119, 1, !dbg !440
  store i32 %120, ptr %18, align 4, !dbg !440
  br label %96, !dbg !441, !llvm.loop !442

121:                                              ; preds = %96
  br label %122, !dbg !443

122:                                              ; preds = %121
  %123 = load i32, ptr %17, align 4, !dbg !444
  %124 = add nsw i32 %123, 1, !dbg !444
  store i32 %124, ptr %17, align 4, !dbg !444
  br label %91, !dbg !445, !llvm.loop !446

125:                                              ; preds = %91
  ret void, !dbg !448
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemm(i32 noundef %0, i32 noundef %1, i32 noundef %2, double noundef %3, double noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !449 {
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
    #dbg_declare(ptr %9, !452, !DIExpression(), !453)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !454, !DIExpression(), !455)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !456, !DIExpression(), !457)
  store double %3, ptr %12, align 8
    #dbg_declare(ptr %12, !458, !DIExpression(), !459)
  store double %4, ptr %13, align 8
    #dbg_declare(ptr %13, !460, !DIExpression(), !461)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !462, !DIExpression(), !463)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !464, !DIExpression(), !465)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !466, !DIExpression(), !467)
    #dbg_declare(ptr %17, !468, !DIExpression(), !469)
    #dbg_declare(ptr %18, !470, !DIExpression(), !471)
    #dbg_declare(ptr %19, !472, !DIExpression(), !473)
  store i32 0, ptr %17, align 4, !dbg !474
  br label %20, !dbg !476

20:                                               ; preds = %89, %8
  %21 = load i32, ptr %17, align 4, !dbg !477
  %22 = load i32, ptr %9, align 4, !dbg !479
  %23 = icmp slt i32 %21, %22, !dbg !480
  br i1 %23, label %24, label %92, !dbg !481

24:                                               ; preds = %20
  store i32 0, ptr %18, align 4, !dbg !482
  br label %25, !dbg !485

25:                                               ; preds = %40, %24
  %26 = load i32, ptr %18, align 4, !dbg !486
  %27 = load i32, ptr %10, align 4, !dbg !488
  %28 = icmp slt i32 %26, %27, !dbg !489
  br i1 %28, label %29, label %43, !dbg !490

29:                                               ; preds = %25
  %30 = load double, ptr %13, align 8, !dbg !491
  %31 = load ptr, ptr %14, align 8, !dbg !492
  %32 = load i32, ptr %17, align 4, !dbg !493
  %33 = sext i32 %32 to i64, !dbg !492
  %34 = getelementptr inbounds [25 x double], ptr %31, i64 %33, !dbg !492
  %35 = load i32, ptr %18, align 4, !dbg !494
  %36 = sext i32 %35 to i64, !dbg !492
  %37 = getelementptr inbounds [25 x double], ptr %34, i64 0, i64 %36, !dbg !492
  %38 = load double, ptr %37, align 8, !dbg !495
  %39 = fmul double %38, %30, !dbg !495
  store double %39, ptr %37, align 8, !dbg !495
  br label %40, !dbg !492

40:                                               ; preds = %29
  %41 = load i32, ptr %18, align 4, !dbg !496
  %42 = add nsw i32 %41, 1, !dbg !496
  store i32 %42, ptr %18, align 4, !dbg !496
  br label %25, !dbg !497, !llvm.loop !498

43:                                               ; preds = %25
  store i32 0, ptr %19, align 4, !dbg !500
  br label %44, !dbg !502

44:                                               ; preds = %85, %43
  %45 = load i32, ptr %19, align 4, !dbg !503
  %46 = load i32, ptr %11, align 4, !dbg !505
  %47 = icmp slt i32 %45, %46, !dbg !506
  br i1 %47, label %48, label %88, !dbg !507

48:                                               ; preds = %44
  store i32 0, ptr %18, align 4, !dbg !508
  br label %49, !dbg !511

49:                                               ; preds = %81, %48
  %50 = load i32, ptr %18, align 4, !dbg !512
  %51 = load i32, ptr %10, align 4, !dbg !514
  %52 = icmp slt i32 %50, %51, !dbg !515
  br i1 %52, label %53, label %84, !dbg !516

53:                                               ; preds = %49
  %54 = load double, ptr %12, align 8, !dbg !517
  %55 = load ptr, ptr %15, align 8, !dbg !518
  %56 = load i32, ptr %17, align 4, !dbg !519
  %57 = sext i32 %56 to i64, !dbg !518
  %58 = getelementptr inbounds [30 x double], ptr %55, i64 %57, !dbg !518
  %59 = load i32, ptr %19, align 4, !dbg !520
  %60 = sext i32 %59 to i64, !dbg !518
  %61 = getelementptr inbounds [30 x double], ptr %58, i64 0, i64 %60, !dbg !518
  %62 = load double, ptr %61, align 8, !dbg !518
  %63 = fmul double %54, %62, !dbg !521
  %64 = load ptr, ptr %16, align 8, !dbg !522
  %65 = load i32, ptr %19, align 4, !dbg !523
  %66 = sext i32 %65 to i64, !dbg !522
  %67 = getelementptr inbounds [25 x double], ptr %64, i64 %66, !dbg !522
  %68 = load i32, ptr %18, align 4, !dbg !524
  %69 = sext i32 %68 to i64, !dbg !522
  %70 = getelementptr inbounds [25 x double], ptr %67, i64 0, i64 %69, !dbg !522
  %71 = load double, ptr %70, align 8, !dbg !522
  %72 = load ptr, ptr %14, align 8, !dbg !525
  %73 = load i32, ptr %17, align 4, !dbg !526
  %74 = sext i32 %73 to i64, !dbg !525
  %75 = getelementptr inbounds [25 x double], ptr %72, i64 %74, !dbg !525
  %76 = load i32, ptr %18, align 4, !dbg !527
  %77 = sext i32 %76 to i64, !dbg !525
  %78 = getelementptr inbounds [25 x double], ptr %75, i64 0, i64 %77, !dbg !525
  %79 = load double, ptr %78, align 8, !dbg !528
  %80 = call double @llvm.fmuladd.f64(double %63, double %71, double %79), !dbg !528
  store double %80, ptr %78, align 8, !dbg !528
  br label %81, !dbg !525

81:                                               ; preds = %53
  %82 = load i32, ptr %18, align 4, !dbg !529
  %83 = add nsw i32 %82, 1, !dbg !529
  store i32 %83, ptr %18, align 4, !dbg !529
  br label %49, !dbg !530, !llvm.loop !531

84:                                               ; preds = %49
  br label %85, !dbg !533

85:                                               ; preds = %84
  %86 = load i32, ptr %19, align 4, !dbg !534
  %87 = add nsw i32 %86, 1, !dbg !534
  store i32 %87, ptr %19, align 4, !dbg !534
  br label %44, !dbg !535, !llvm.loop !536

88:                                               ; preds = %44
  br label %89, !dbg !538

89:                                               ; preds = %88
  %90 = load i32, ptr %17, align 4, !dbg !539
  %91 = add nsw i32 %90, 1, !dbg !539
  store i32 %91, ptr %17, align 4, !dbg !539
  br label %20, !dbg !540, !llvm.loop !541

92:                                               ; preds = %20
  ret void, !dbg !543
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemm_long_double(i32 noundef %0, i32 noundef %1, i32 noundef %2, x86_fp80 noundef %3, x86_fp80 noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 !dbg !544 {
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca x86_fp80, align 16
  %13 = alloca x86_fp80, align 16
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca i32, align 4
  %18 = alloca i32, align 4
  %19 = alloca i32, align 4
  store i32 %0, ptr %9, align 4
    #dbg_declare(ptr %9, !547, !DIExpression(), !548)
  store i32 %1, ptr %10, align 4
    #dbg_declare(ptr %10, !549, !DIExpression(), !550)
  store i32 %2, ptr %11, align 4
    #dbg_declare(ptr %11, !551, !DIExpression(), !552)
  store x86_fp80 %3, ptr %12, align 16
    #dbg_declare(ptr %12, !553, !DIExpression(), !554)
  store x86_fp80 %4, ptr %13, align 16
    #dbg_declare(ptr %13, !555, !DIExpression(), !556)
  store ptr %5, ptr %14, align 8
    #dbg_declare(ptr %14, !557, !DIExpression(), !558)
  store ptr %6, ptr %15, align 8
    #dbg_declare(ptr %15, !559, !DIExpression(), !560)
  store ptr %7, ptr %16, align 8
    #dbg_declare(ptr %16, !561, !DIExpression(), !562)
    #dbg_declare(ptr %17, !563, !DIExpression(), !564)
    #dbg_declare(ptr %18, !565, !DIExpression(), !566)
    #dbg_declare(ptr %19, !567, !DIExpression(), !568)
  store i32 0, ptr %17, align 4, !dbg !569
  br label %20, !dbg !571

20:                                               ; preds = %89, %8
  %21 = load i32, ptr %17, align 4, !dbg !572
  %22 = load i32, ptr %9, align 4, !dbg !574
  %23 = icmp slt i32 %21, %22, !dbg !575
  br i1 %23, label %24, label %92, !dbg !576

24:                                               ; preds = %20
  store i32 0, ptr %18, align 4, !dbg !577
  br label %25, !dbg !580

25:                                               ; preds = %40, %24
  %26 = load i32, ptr %18, align 4, !dbg !581
  %27 = load i32, ptr %10, align 4, !dbg !583
  %28 = icmp slt i32 %26, %27, !dbg !584
  br i1 %28, label %29, label %43, !dbg !585

29:                                               ; preds = %25
  %30 = load x86_fp80, ptr %13, align 16, !dbg !586
  %31 = load ptr, ptr %14, align 8, !dbg !587
  %32 = load i32, ptr %17, align 4, !dbg !588
  %33 = sext i32 %32 to i64, !dbg !587
  %34 = getelementptr inbounds [25 x x86_fp80], ptr %31, i64 %33, !dbg !587
  %35 = load i32, ptr %18, align 4, !dbg !589
  %36 = sext i32 %35 to i64, !dbg !587
  %37 = getelementptr inbounds [25 x x86_fp80], ptr %34, i64 0, i64 %36, !dbg !587
  %38 = load x86_fp80, ptr %37, align 16, !dbg !590
  %39 = fmul x86_fp80 %38, %30, !dbg !590
  store x86_fp80 %39, ptr %37, align 16, !dbg !590
  br label %40, !dbg !587

40:                                               ; preds = %29
  %41 = load i32, ptr %18, align 4, !dbg !591
  %42 = add nsw i32 %41, 1, !dbg !591
  store i32 %42, ptr %18, align 4, !dbg !591
  br label %25, !dbg !592, !llvm.loop !593

43:                                               ; preds = %25
  store i32 0, ptr %19, align 4, !dbg !595
  br label %44, !dbg !597

44:                                               ; preds = %85, %43
  %45 = load i32, ptr %19, align 4, !dbg !598
  %46 = load i32, ptr %11, align 4, !dbg !600
  %47 = icmp slt i32 %45, %46, !dbg !601
  br i1 %47, label %48, label %88, !dbg !602

48:                                               ; preds = %44
  store i32 0, ptr %18, align 4, !dbg !603
  br label %49, !dbg !606

49:                                               ; preds = %81, %48
  %50 = load i32, ptr %18, align 4, !dbg !607
  %51 = load i32, ptr %10, align 4, !dbg !609
  %52 = icmp slt i32 %50, %51, !dbg !610
  br i1 %52, label %53, label %84, !dbg !611

53:                                               ; preds = %49
  %54 = load x86_fp80, ptr %12, align 16, !dbg !612
  %55 = load ptr, ptr %15, align 8, !dbg !613
  %56 = load i32, ptr %17, align 4, !dbg !614
  %57 = sext i32 %56 to i64, !dbg !613
  %58 = getelementptr inbounds [30 x x86_fp80], ptr %55, i64 %57, !dbg !613
  %59 = load i32, ptr %19, align 4, !dbg !615
  %60 = sext i32 %59 to i64, !dbg !613
  %61 = getelementptr inbounds [30 x x86_fp80], ptr %58, i64 0, i64 %60, !dbg !613
  %62 = load x86_fp80, ptr %61, align 16, !dbg !613
  %63 = fmul x86_fp80 %54, %62, !dbg !616
  %64 = load ptr, ptr %16, align 8, !dbg !617
  %65 = load i32, ptr %19, align 4, !dbg !618
  %66 = sext i32 %65 to i64, !dbg !617
  %67 = getelementptr inbounds [25 x x86_fp80], ptr %64, i64 %66, !dbg !617
  %68 = load i32, ptr %18, align 4, !dbg !619
  %69 = sext i32 %68 to i64, !dbg !617
  %70 = getelementptr inbounds [25 x x86_fp80], ptr %67, i64 0, i64 %69, !dbg !617
  %71 = load x86_fp80, ptr %70, align 16, !dbg !617
  %72 = load ptr, ptr %14, align 8, !dbg !620
  %73 = load i32, ptr %17, align 4, !dbg !621
  %74 = sext i32 %73 to i64, !dbg !620
  %75 = getelementptr inbounds [25 x x86_fp80], ptr %72, i64 %74, !dbg !620
  %76 = load i32, ptr %18, align 4, !dbg !622
  %77 = sext i32 %76 to i64, !dbg !620
  %78 = getelementptr inbounds [25 x x86_fp80], ptr %75, i64 0, i64 %77, !dbg !620
  %79 = load x86_fp80, ptr %78, align 16, !dbg !623
  %80 = call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %63, x86_fp80 %71, x86_fp80 %79), !dbg !623
  store x86_fp80 %80, ptr %78, align 16, !dbg !623
  br label %81, !dbg !620

81:                                               ; preds = %53
  %82 = load i32, ptr %18, align 4, !dbg !624
  %83 = add nsw i32 %82, 1, !dbg !624
  store i32 %83, ptr %18, align 4, !dbg !624
  br label %49, !dbg !625, !llvm.loop !626

84:                                               ; preds = %49
  br label %85, !dbg !628

85:                                               ; preds = %84
  %86 = load i32, ptr %19, align 4, !dbg !629
  %87 = add nsw i32 %86, 1, !dbg !629
  store i32 %87, ptr %19, align 4, !dbg !629
  br label %44, !dbg !630, !llvm.loop !631

88:                                               ; preds = %44
  br label %89, !dbg !633

89:                                               ; preds = %88
  %90 = load i32, ptr %17, align 4, !dbg !634
  %91 = add nsw i32 %90, 1, !dbg !634
  store i32 %91, ptr %17, align 4, !dbg !634
  br label %20, !dbg !635, !llvm.loop !636

92:                                               ; preds = %20
  ret void, !dbg !638
}

; Function Attrs: noinline nounwind uwtable
define internal void @print_array(i32 noundef %0, i32 noundef %1, ptr noundef %2, ptr noundef %3) #0 !dbg !639 {
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca ptr, align 8
  %8 = alloca ptr, align 8
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca double, align 8
  %12 = alloca double, align 8
  %13 = alloca double, align 8
  %14 = alloca x86_fp80, align 16
  %15 = alloca x86_fp80, align 16
  %16 = alloca x86_fp80, align 16
  %17 = alloca double, align 8
  %18 = alloca x86_fp80, align 16
  %19 = alloca double, align 8
  %20 = alloca x86_fp80, align 16
  %21 = alloca double, align 8
  store i32 %0, ptr %5, align 4
    #dbg_declare(ptr %5, !642, !DIExpression(), !643)
  store i32 %1, ptr %6, align 4
    #dbg_declare(ptr %6, !644, !DIExpression(), !645)
  store ptr %2, ptr %7, align 8
    #dbg_declare(ptr %7, !646, !DIExpression(), !647)
  store ptr %3, ptr %8, align 8
    #dbg_declare(ptr %8, !648, !DIExpression(), !649)
    #dbg_declare(ptr %9, !650, !DIExpression(), !651)
    #dbg_declare(ptr %10, !652, !DIExpression(), !653)
    #dbg_declare(ptr %11, !654, !DIExpression(), !655)
  store double 0.000000e+00, ptr %11, align 8, !dbg !655
    #dbg_declare(ptr %12, !656, !DIExpression(), !657)
  store double 0.000000e+00, ptr %12, align 8, !dbg !657
    #dbg_declare(ptr %13, !658, !DIExpression(), !659)
  store double 0.000000e+00, ptr %13, align 8, !dbg !659
    #dbg_declare(ptr %14, !660, !DIExpression(), !661)
  store x86_fp80 0xK00000000000000000000, ptr %14, align 16, !dbg !661
    #dbg_declare(ptr %15, !662, !DIExpression(), !663)
  store x86_fp80 0xK00000000000000000000, ptr %15, align 16, !dbg !663
    #dbg_declare(ptr %16, !664, !DIExpression(), !665)
  store x86_fp80 0xK00000000000000000000, ptr %16, align 16, !dbg !665
  %22 = load ptr, ptr @stderr, align 8, !dbg !666
  %23 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %22, ptr noundef @.str) #4, !dbg !666
  %24 = load ptr, ptr @stderr, align 8, !dbg !667
  %25 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %24, ptr noundef @.str.1, ptr noundef @.str.2) #4, !dbg !667
  store i32 0, ptr %9, align 4, !dbg !668
  br label %26, !dbg !670

26:                                               ; preds = %80, %4
  %27 = load i32, ptr %9, align 4, !dbg !671
  %28 = load i32, ptr %5, align 4, !dbg !673
  %29 = icmp slt i32 %27, %28, !dbg !674
  br i1 %29, label %30, label %83, !dbg !675

30:                                               ; preds = %26
  store i32 0, ptr %10, align 4, !dbg !676
  br label %31, !dbg !678

31:                                               ; preds = %76, %30
  %32 = load i32, ptr %10, align 4, !dbg !679
  %33 = load i32, ptr %6, align 4, !dbg !681
  %34 = icmp slt i32 %32, %33, !dbg !682
  br i1 %34, label %35, label %79, !dbg !683

35:                                               ; preds = %31
    #dbg_declare(ptr %17, !684, !DIExpression(), !686)
  %36 = load ptr, ptr %7, align 8, !dbg !687
  %37 = load i32, ptr %9, align 4, !dbg !688
  %38 = sext i32 %37 to i64, !dbg !687
  %39 = getelementptr inbounds [25 x double], ptr %36, i64 %38, !dbg !687
  %40 = load i32, ptr %10, align 4, !dbg !689
  %41 = sext i32 %40 to i64, !dbg !687
  %42 = getelementptr inbounds [25 x double], ptr %39, i64 0, i64 %41, !dbg !687
  %43 = load double, ptr %42, align 8, !dbg !687
  store double %43, ptr %17, align 8, !dbg !686
    #dbg_declare(ptr %18, !690, !DIExpression(), !691)
  %44 = load ptr, ptr %8, align 8, !dbg !692
  %45 = load i32, ptr %9, align 4, !dbg !693
  %46 = sext i32 %45 to i64, !dbg !692
  %47 = getelementptr inbounds [25 x x86_fp80], ptr %44, i64 %46, !dbg !692
  %48 = load i32, ptr %10, align 4, !dbg !694
  %49 = sext i32 %48 to i64, !dbg !692
  %50 = getelementptr inbounds [25 x x86_fp80], ptr %47, i64 0, i64 %49, !dbg !692
  %51 = load x86_fp80, ptr %50, align 16, !dbg !692
  store x86_fp80 %51, ptr %18, align 16, !dbg !691
  %52 = load double, ptr %17, align 8, !dbg !695
  %53 = fcmp olt double %52, 0.000000e+00, !dbg !697
  br i1 %53, label %54, label %57, !dbg !697

54:                                               ; preds = %35
  %55 = load double, ptr %17, align 8, !dbg !698
  %56 = fneg double %55, !dbg !699
  store double %56, ptr %17, align 8, !dbg !700
  br label %57, !dbg !701

57:                                               ; preds = %54, %35
  %58 = load x86_fp80, ptr %18, align 16, !dbg !702
  %59 = fcmp olt x86_fp80 %58, 0xK00000000000000000000, !dbg !704
  br i1 %59, label %60, label %63, !dbg !704

60:                                               ; preds = %57
  %61 = load x86_fp80, ptr %18, align 16, !dbg !705
  %62 = fneg x86_fp80 %61, !dbg !706
  store x86_fp80 %62, ptr %18, align 16, !dbg !707
  br label %63, !dbg !708

63:                                               ; preds = %60, %57
  %64 = load double, ptr %17, align 8, !dbg !709
  %65 = load double, ptr %11, align 8, !dbg !711
  %66 = fcmp ogt double %64, %65, !dbg !712
  br i1 %66, label %67, label %69, !dbg !712

67:                                               ; preds = %63
  %68 = load double, ptr %17, align 8, !dbg !713
  store double %68, ptr %11, align 8, !dbg !715
  br label %69, !dbg !716

69:                                               ; preds = %67, %63
  %70 = load x86_fp80, ptr %18, align 16, !dbg !717
  %71 = load x86_fp80, ptr %14, align 16, !dbg !719
  %72 = fcmp ogt x86_fp80 %70, %71, !dbg !720
  br i1 %72, label %73, label %75, !dbg !720

73:                                               ; preds = %69
  %74 = load x86_fp80, ptr %18, align 16, !dbg !721
  store x86_fp80 %74, ptr %14, align 16, !dbg !723
  br label %75, !dbg !724

75:                                               ; preds = %73, %69
  br label %76, !dbg !725

76:                                               ; preds = %75
  %77 = load i32, ptr %10, align 4, !dbg !726
  %78 = add nsw i32 %77, 1, !dbg !726
  store i32 %78, ptr %10, align 4, !dbg !726
  br label %31, !dbg !727, !llvm.loop !728

79:                                               ; preds = %31
  br label %80, !dbg !729

80:                                               ; preds = %79
  %81 = load i32, ptr %9, align 4, !dbg !730
  %82 = add nsw i32 %81, 1, !dbg !730
  store i32 %82, ptr %9, align 4, !dbg !730
  br label %26, !dbg !731, !llvm.loop !732

83:                                               ; preds = %26
  %84 = load double, ptr %11, align 8, !dbg !734
  %85 = fcmp une double %84, 0.000000e+00, !dbg !736
  br i1 %85, label %86, label %121, !dbg !736

86:                                               ; preds = %83
  store i32 0, ptr %9, align 4, !dbg !737
  br label %87, !dbg !740

87:                                               ; preds = %115, %86
  %88 = load i32, ptr %9, align 4, !dbg !741
  %89 = load i32, ptr %5, align 4, !dbg !743
  %90 = icmp slt i32 %88, %89, !dbg !744
  br i1 %90, label %91, label %118, !dbg !745

91:                                               ; preds = %87
  store i32 0, ptr %10, align 4, !dbg !746
  br label %92, !dbg !749

92:                                               ; preds = %111, %91
  %93 = load i32, ptr %10, align 4, !dbg !750
  %94 = load i32, ptr %6, align 4, !dbg !752
  %95 = icmp slt i32 %93, %94, !dbg !753
  br i1 %95, label %96, label %114, !dbg !754

96:                                               ; preds = %92
    #dbg_declare(ptr %19, !755, !DIExpression(), !757)
  %97 = load ptr, ptr %7, align 8, !dbg !758
  %98 = load i32, ptr %9, align 4, !dbg !759
  %99 = sext i32 %98 to i64, !dbg !758
  %100 = getelementptr inbounds [25 x double], ptr %97, i64 %99, !dbg !758
  %101 = load i32, ptr %10, align 4, !dbg !760
  %102 = sext i32 %101 to i64, !dbg !758
  %103 = getelementptr inbounds [25 x double], ptr %100, i64 0, i64 %102, !dbg !758
  %104 = load double, ptr %103, align 8, !dbg !758
  %105 = load double, ptr %11, align 8, !dbg !761
  %106 = fdiv double %104, %105, !dbg !762
  store double %106, ptr %19, align 8, !dbg !757
  %107 = load double, ptr %19, align 8, !dbg !763
  %108 = load double, ptr %19, align 8, !dbg !764
  %109 = load double, ptr %12, align 8, !dbg !765
  %110 = call double @llvm.fmuladd.f64(double %107, double %108, double %109), !dbg !765
  store double %110, ptr %12, align 8, !dbg !765
  br label %111, !dbg !766

111:                                              ; preds = %96
  %112 = load i32, ptr %10, align 4, !dbg !767
  %113 = add nsw i32 %112, 1, !dbg !767
  store i32 %113, ptr %10, align 4, !dbg !767
  br label %92, !dbg !768, !llvm.loop !769

114:                                              ; preds = %92
  br label %115, !dbg !771

115:                                              ; preds = %114
  %116 = load i32, ptr %9, align 4, !dbg !772
  %117 = add nsw i32 %116, 1, !dbg !772
  store i32 %117, ptr %9, align 4, !dbg !772
  br label %87, !dbg !773, !llvm.loop !774

118:                                              ; preds = %87
  %119 = load double, ptr %12, align 8, !dbg !776
  %120 = call double @sqrt(double noundef %119) #4, !dbg !776
  store double %120, ptr %13, align 8, !dbg !777
  br label %121, !dbg !778

121:                                              ; preds = %118, %83
  %122 = load x86_fp80, ptr %14, align 16, !dbg !779
  %123 = fcmp une x86_fp80 %122, 0xK00000000000000000000, !dbg !781
  br i1 %123, label %124, label %159, !dbg !781

124:                                              ; preds = %121
  store i32 0, ptr %9, align 4, !dbg !782
  br label %125, !dbg !785

125:                                              ; preds = %153, %124
  %126 = load i32, ptr %9, align 4, !dbg !786
  %127 = load i32, ptr %5, align 4, !dbg !788
  %128 = icmp slt i32 %126, %127, !dbg !789
  br i1 %128, label %129, label %156, !dbg !790

129:                                              ; preds = %125
  store i32 0, ptr %10, align 4, !dbg !791
  br label %130, !dbg !794

130:                                              ; preds = %149, %129
  %131 = load i32, ptr %10, align 4, !dbg !795
  %132 = load i32, ptr %6, align 4, !dbg !797
  %133 = icmp slt i32 %131, %132, !dbg !798
  br i1 %133, label %134, label %152, !dbg !799

134:                                              ; preds = %130
    #dbg_declare(ptr %20, !800, !DIExpression(), !802)
  %135 = load ptr, ptr %8, align 8, !dbg !803
  %136 = load i32, ptr %9, align 4, !dbg !804
  %137 = sext i32 %136 to i64, !dbg !803
  %138 = getelementptr inbounds [25 x x86_fp80], ptr %135, i64 %137, !dbg !803
  %139 = load i32, ptr %10, align 4, !dbg !805
  %140 = sext i32 %139 to i64, !dbg !803
  %141 = getelementptr inbounds [25 x x86_fp80], ptr %138, i64 0, i64 %140, !dbg !803
  %142 = load x86_fp80, ptr %141, align 16, !dbg !803
  %143 = load x86_fp80, ptr %14, align 16, !dbg !806
  %144 = fdiv x86_fp80 %142, %143, !dbg !807
  store x86_fp80 %144, ptr %20, align 16, !dbg !802
  %145 = load x86_fp80, ptr %20, align 16, !dbg !808
  %146 = load x86_fp80, ptr %20, align 16, !dbg !809
  %147 = load x86_fp80, ptr %15, align 16, !dbg !810
  %148 = call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %145, x86_fp80 %146, x86_fp80 %147), !dbg !810
  store x86_fp80 %148, ptr %15, align 16, !dbg !810
  br label %149, !dbg !811

149:                                              ; preds = %134
  %150 = load i32, ptr %10, align 4, !dbg !812
  %151 = add nsw i32 %150, 1, !dbg !812
  store i32 %151, ptr %10, align 4, !dbg !812
  br label %130, !dbg !813, !llvm.loop !814

152:                                              ; preds = %130
  br label %153, !dbg !816

153:                                              ; preds = %152
  %154 = load i32, ptr %9, align 4, !dbg !817
  %155 = add nsw i32 %154, 1, !dbg !817
  store i32 %155, ptr %9, align 4, !dbg !817
  br label %125, !dbg !818, !llvm.loop !819

156:                                              ; preds = %125
  %157 = load x86_fp80, ptr %15, align 16, !dbg !821
  %158 = call x86_fp80 @sqrtl(x86_fp80 noundef %157) #4, !dbg !822
  store x86_fp80 %158, ptr %16, align 16, !dbg !823
  br label %159, !dbg !824

159:                                              ; preds = %156, %121
  %160 = load ptr, ptr @stderr, align 8, !dbg !825
  %161 = load double, ptr %11, align 8, !dbg !826
  %162 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %160, ptr noundef @.str.3, double noundef %161) #4, !dbg !827
  %163 = load ptr, ptr @stderr, align 8, !dbg !828
  %164 = load double, ptr %13, align 8, !dbg !829
  %165 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %163, ptr noundef @.str.4, double noundef %164) #4, !dbg !830
  %166 = load ptr, ptr @stderr, align 8, !dbg !831
  %167 = load x86_fp80, ptr %14, align 16, !dbg !832
  %168 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %166, ptr noundef @.str.5, x86_fp80 noundef %167) #4, !dbg !833
  %169 = load ptr, ptr @stderr, align 8, !dbg !834
  %170 = load x86_fp80, ptr %16, align 16, !dbg !835
  %171 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %169, ptr noundef @.str.6, x86_fp80 noundef %170) #4, !dbg !836
    #dbg_declare(ptr %21, !837, !DIExpression(), !838)
  %172 = load x86_fp80, ptr %16, align 16, !dbg !839
  %173 = load double, ptr %13, align 8, !dbg !840
  %174 = fpext double %173 to x86_fp80, !dbg !841
  %175 = fsub x86_fp80 %172, %174, !dbg !842
  %176 = fptrunc x86_fp80 %175 to double, !dbg !839
  store double %176, ptr %21, align 8, !dbg !838
  %177 = load ptr, ptr @stderr, align 8, !dbg !843
  %178 = load double, ptr %21, align 8, !dbg !844
  %179 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %177, ptr noundef @.str.7, double noundef %178) #4, !dbg !845
  %180 = load ptr, ptr @stderr, align 8, !dbg !846
  %181 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %180, ptr noundef @.str.8, ptr noundef @.str.9) #4, !dbg !846
  %182 = load ptr, ptr @stderr, align 8, !dbg !847
  %183 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %182, ptr noundef @.str.10) #4, !dbg !847
  ret void, !dbg !848
}

; Function Attrs: nounwind
declare dso_local void @free(ptr noundef) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.fmuladd.f64(double, double, double) #3

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare x86_fp80 @llvm.fmuladd.f80(x86_fp80, x86_fp80, x86_fp80) #3

; Function Attrs: nounwind
declare dso_local i32 @fprintf(ptr noundef, ptr noundef, ...) #2

; Function Attrs: nounwind
declare dso_local double @sqrt(double noundef) #2

; Function Attrs: nounwind
declare dso_local x86_fp80 @sqrtl(x86_fp80 noundef) #2

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!51}
!llvm.module.flags = !{!75, !76, !77, !78, !79}
!llvm.ident = !{!80}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 87, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "gemm.c", directory: "/g/g90/sharmin1/tutorial/FPChecker/experiments/benchmark/PolyBenchC-4.2.1/linear-algebra/blas/gemm_fp64")
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
!18 = distinct !DIGlobalVariable(scope: null, file: !2, line: 134, type: !3, isLocal: true, isDefinition: true)
!19 = !DIGlobalVariableExpression(var: !20, expr: !DIExpression())
!20 = distinct !DIGlobalVariable(scope: null, file: !2, line: 135, type: !21, isLocal: true, isDefinition: true)
!21 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 144, elements: !22)
!22 = !{!23}
!23 = !DISubrange(count: 18)
!24 = !DIGlobalVariableExpression(var: !25, expr: !DIExpression())
!25 = distinct !DIGlobalVariable(scope: null, file: !2, line: 136, type: !26, isLocal: true, isDefinition: true)
!26 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 272, elements: !27)
!27 = !{!28}
!28 = !DISubrange(count: 34)
!29 = !DIGlobalVariableExpression(var: !30, expr: !DIExpression())
!30 = distinct !DIGlobalVariable(scope: null, file: !2, line: 137, type: !31, isLocal: true, isDefinition: true)
!31 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 232, elements: !32)
!32 = !{!33}
!33 = !DISubrange(count: 29)
!34 = !DIGlobalVariableExpression(var: !35, expr: !DIExpression())
!35 = distinct !DIGlobalVariable(scope: null, file: !2, line: 140, type: !36, isLocal: true, isDefinition: true)
!36 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 152, elements: !37)
!37 = !{!38}
!38 = !DISubrange(count: 19)
!39 = !DIGlobalVariableExpression(var: !40, expr: !DIExpression())
!40 = distinct !DIGlobalVariable(scope: null, file: !2, line: 142, type: !41, isLocal: true, isDefinition: true)
!41 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 136, elements: !42)
!42 = !{!43}
!43 = !DISubrange(count: 17)
!44 = !DIGlobalVariableExpression(var: !45, expr: !DIExpression())
!45 = distinct !DIGlobalVariable(scope: null, file: !2, line: 142, type: !46, isLocal: true, isDefinition: true)
!46 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 16, elements: !47)
!47 = !{!48}
!48 = !DISubrange(count: 2)
!49 = !DIGlobalVariableExpression(var: !50, expr: !DIExpression())
!50 = distinct !DIGlobalVariable(scope: null, file: !2, line: 143, type: !3, isLocal: true, isDefinition: true)
!51 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !52, globals: !74, splitDebugInlining: false, nameTableKind: None)
!52 = !{!53, !59, !63, !66, !69, !71, !73, !55, !68}
!53 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !54, size: 64)
!54 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 32000, elements: !56)
!55 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!56 = !{!57, !58}
!57 = !DISubrange(count: 20)
!58 = !DISubrange(count: 25)
!59 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !60, size: 64)
!60 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 38400, elements: !61)
!61 = !{!57, !62}
!62 = !DISubrange(count: 30)
!63 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !64, size: 64)
!64 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 48000, elements: !65)
!65 = !{!62, !58}
!66 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !67, size: 64)
!67 = !DICompositeType(tag: DW_TAG_array_type, baseType: !68, size: 64000, elements: !56)
!68 = !DIBasicType(name: "long double", size: 128, encoding: DW_ATE_float)
!69 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !70, size: 64)
!70 = !DICompositeType(tag: DW_TAG_array_type, baseType: !68, size: 76800, elements: !61)
!71 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !72, size: 64)
!72 = !DICompositeType(tag: DW_TAG_array_type, baseType: !68, size: 96000, elements: !65)
!73 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!74 = !{!0, !7, !12, !17, !19, !24, !29, !34, !39, !44, !49}
!75 = !{i32 7, !"Dwarf Version", i32 4}
!76 = !{i32 2, !"Debug Info Version", i32 3}
!77 = !{i32 1, !"wchar_size", i32 4}
!78 = !{i32 7, !"uwtable", i32 2}
!79 = !{i32 7, !"frame-pointer", i32 2}
!80 = !{!"clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)"}
!81 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 202, type: !82, scopeLine: 203, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !51, retainedNodes: !87)
!82 = !DISubroutineType(types: !83)
!83 = !{!84, !84, !85}
!84 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!85 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !86, size: 64)
!86 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!87 = !{}
!88 = !DILocalVariable(name: "argc", arg: 1, scope: !81, file: !2, line: 202, type: !84)
!89 = !DILocation(line: 202, column: 14, scope: !81)
!90 = !DILocalVariable(name: "argv", arg: 2, scope: !81, file: !2, line: 202, type: !85)
!91 = !DILocation(line: 202, column: 27, scope: !81)
!92 = !DILocalVariable(name: "ni", scope: !81, file: !2, line: 205, type: !84)
!93 = !DILocation(line: 205, column: 7, scope: !81)
!94 = !DILocalVariable(name: "nj", scope: !81, file: !2, line: 206, type: !84)
!95 = !DILocation(line: 206, column: 7, scope: !81)
!96 = !DILocalVariable(name: "nk", scope: !81, file: !2, line: 207, type: !84)
!97 = !DILocation(line: 207, column: 7, scope: !81)
!98 = !DILocalVariable(name: "alpha", scope: !81, file: !2, line: 210, type: !55)
!99 = !DILocation(line: 210, column: 13, scope: !81)
!100 = !DILocalVariable(name: "beta", scope: !81, file: !2, line: 211, type: !55)
!101 = !DILocation(line: 211, column: 13, scope: !81)
!102 = !DILocalVariable(name: "alpha_long_double", scope: !81, file: !2, line: 212, type: !68)
!103 = !DILocation(line: 212, column: 15, scope: !81)
!104 = !DILocalVariable(name: "beta_long_double", scope: !81, file: !2, line: 213, type: !68)
!105 = !DILocation(line: 213, column: 15, scope: !81)
!106 = !DILocalVariable(name: "C", scope: !81, file: !2, line: 215, type: !53)
!107 = !DILocation(line: 215, column: 3, scope: !81)
!108 = !DILocalVariable(name: "A", scope: !81, file: !2, line: 216, type: !59)
!109 = !DILocation(line: 216, column: 3, scope: !81)
!110 = !DILocalVariable(name: "B", scope: !81, file: !2, line: 217, type: !63)
!111 = !DILocation(line: 217, column: 3, scope: !81)
!112 = !DILocalVariable(name: "C_long_double", scope: !81, file: !2, line: 219, type: !66)
!113 = !DILocation(line: 219, column: 3, scope: !81)
!114 = !DILocalVariable(name: "A_long_double", scope: !81, file: !2, line: 220, type: !69)
!115 = !DILocation(line: 220, column: 3, scope: !81)
!116 = !DILocalVariable(name: "B_long_double", scope: !81, file: !2, line: 221, type: !71)
!117 = !DILocation(line: 221, column: 3, scope: !81)
!118 = !DILocation(line: 224, column: 15, scope: !81)
!119 = !DILocation(line: 224, column: 19, scope: !81)
!120 = !DILocation(line: 224, column: 23, scope: !81)
!121 = !DILocation(line: 225, column: 8, scope: !81)
!122 = !DILocation(line: 226, column: 8, scope: !81)
!123 = !DILocation(line: 227, column: 8, scope: !81)
!124 = !DILocation(line: 224, column: 3, scope: !81)
!125 = !DILocation(line: 229, column: 27, scope: !81)
!126 = !DILocation(line: 229, column: 31, scope: !81)
!127 = !DILocation(line: 229, column: 35, scope: !81)
!128 = !DILocation(line: 230, column: 8, scope: !81)
!129 = !DILocation(line: 231, column: 8, scope: !81)
!130 = !DILocation(line: 232, column: 8, scope: !81)
!131 = !DILocation(line: 229, column: 3, scope: !81)
!132 = !DILocation(line: 237, column: 16, scope: !81)
!133 = !DILocation(line: 237, column: 20, scope: !81)
!134 = !DILocation(line: 237, column: 24, scope: !81)
!135 = !DILocation(line: 238, column: 9, scope: !81)
!136 = !DILocation(line: 238, column: 16, scope: !81)
!137 = !DILocation(line: 239, column: 9, scope: !81)
!138 = !DILocation(line: 240, column: 9, scope: !81)
!139 = !DILocation(line: 241, column: 9, scope: !81)
!140 = !DILocation(line: 237, column: 3, scope: !81)
!141 = !DILocation(line: 243, column: 28, scope: !81)
!142 = !DILocation(line: 243, column: 32, scope: !81)
!143 = !DILocation(line: 243, column: 36, scope: !81)
!144 = !DILocation(line: 244, column: 9, scope: !81)
!145 = !DILocation(line: 244, column: 28, scope: !81)
!146 = !DILocation(line: 245, column: 9, scope: !81)
!147 = !DILocation(line: 246, column: 9, scope: !81)
!148 = !DILocation(line: 247, column: 9, scope: !81)
!149 = !DILocation(line: 243, column: 3, scope: !81)
!150 = !DILocation(line: 255, column: 3, scope: !81)
!151 = !DILocation(line: 258, column: 3, scope: !81)
!152 = !DILocation(line: 259, column: 3, scope: !81)
!153 = !DILocation(line: 260, column: 3, scope: !81)
!154 = !DILocation(line: 261, column: 3, scope: !81)
!155 = !DILocation(line: 262, column: 3, scope: !81)
!156 = !DILocation(line: 263, column: 3, scope: !81)
!157 = !DILocation(line: 265, column: 3, scope: !81)
!158 = distinct !DISubprogram(name: "init_array", scope: !2, file: !2, line: 26, type: !159, scopeLine: 32, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !87)
!159 = !DISubroutineType(types: !160)
!160 = !{null, !84, !84, !84, !161, !161, !162, !165, !162}
!161 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !55, size: 64)
!162 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !163, size: 64)
!163 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 1600, elements: !164)
!164 = !{!58}
!165 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !166, size: 64)
!166 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 1920, elements: !167)
!167 = !{!62}
!168 = !DILocalVariable(name: "ni", arg: 1, scope: !158, file: !2, line: 26, type: !84)
!169 = !DILocation(line: 26, column: 21, scope: !158)
!170 = !DILocalVariable(name: "nj", arg: 2, scope: !158, file: !2, line: 26, type: !84)
!171 = !DILocation(line: 26, column: 29, scope: !158)
!172 = !DILocalVariable(name: "nk", arg: 3, scope: !158, file: !2, line: 26, type: !84)
!173 = !DILocation(line: 26, column: 37, scope: !158)
!174 = !DILocalVariable(name: "alpha", arg: 4, scope: !158, file: !2, line: 27, type: !161)
!175 = !DILocation(line: 27, column: 14, scope: !158)
!176 = !DILocalVariable(name: "beta", arg: 5, scope: !158, file: !2, line: 28, type: !161)
!177 = !DILocation(line: 28, column: 14, scope: !158)
!178 = !DILocalVariable(name: "C", arg: 6, scope: !158, file: !2, line: 29, type: !162)
!179 = !DILocation(line: 29, column: 13, scope: !158)
!180 = !DILocalVariable(name: "A", arg: 7, scope: !158, file: !2, line: 30, type: !165)
!181 = !DILocation(line: 30, column: 13, scope: !158)
!182 = !DILocalVariable(name: "B", arg: 8, scope: !158, file: !2, line: 31, type: !162)
!183 = !DILocation(line: 31, column: 13, scope: !158)
!184 = !DILocalVariable(name: "i", scope: !158, file: !2, line: 33, type: !84)
!185 = !DILocation(line: 33, column: 7, scope: !158)
!186 = !DILocalVariable(name: "j", scope: !158, file: !2, line: 33, type: !84)
!187 = !DILocation(line: 33, column: 10, scope: !158)
!188 = !DILocation(line: 35, column: 4, scope: !158)
!189 = !DILocation(line: 35, column: 10, scope: !158)
!190 = !DILocation(line: 36, column: 4, scope: !158)
!191 = !DILocation(line: 36, column: 9, scope: !158)
!192 = !DILocation(line: 37, column: 10, scope: !193)
!193 = distinct !DILexicalBlock(scope: !158, file: !2, line: 37, column: 3)
!194 = !DILocation(line: 37, column: 8, scope: !193)
!195 = !DILocation(line: 37, column: 15, scope: !196)
!196 = distinct !DILexicalBlock(scope: !193, file: !2, line: 37, column: 3)
!197 = !DILocation(line: 37, column: 19, scope: !196)
!198 = !DILocation(line: 37, column: 17, scope: !196)
!199 = !DILocation(line: 37, column: 3, scope: !193)
!200 = !DILocation(line: 38, column: 12, scope: !201)
!201 = distinct !DILexicalBlock(scope: !196, file: !2, line: 38, column: 5)
!202 = !DILocation(line: 38, column: 10, scope: !201)
!203 = !DILocation(line: 38, column: 17, scope: !204)
!204 = distinct !DILexicalBlock(scope: !201, file: !2, line: 38, column: 5)
!205 = !DILocation(line: 38, column: 21, scope: !204)
!206 = !DILocation(line: 38, column: 19, scope: !204)
!207 = !DILocation(line: 38, column: 5, scope: !201)
!208 = !DILocation(line: 39, column: 31, scope: !204)
!209 = !DILocation(line: 39, column: 33, scope: !204)
!210 = !DILocation(line: 39, column: 32, scope: !204)
!211 = !DILocation(line: 39, column: 34, scope: !204)
!212 = !DILocation(line: 39, column: 40, scope: !204)
!213 = !DILocation(line: 39, column: 38, scope: !204)
!214 = !DILocation(line: 39, column: 17, scope: !204)
!215 = !DILocation(line: 39, column: 46, scope: !204)
!216 = !DILocation(line: 39, column: 44, scope: !204)
!217 = !DILocation(line: 39, column: 7, scope: !204)
!218 = !DILocation(line: 39, column: 9, scope: !204)
!219 = !DILocation(line: 39, column: 12, scope: !204)
!220 = !DILocation(line: 39, column: 15, scope: !204)
!221 = !DILocation(line: 38, column: 26, scope: !204)
!222 = !DILocation(line: 38, column: 5, scope: !204)
!223 = distinct !{!223, !207, !224, !225}
!224 = !DILocation(line: 39, column: 46, scope: !201)
!225 = !{!"llvm.loop.mustprogress"}
!226 = !DILocation(line: 37, column: 24, scope: !196)
!227 = !DILocation(line: 37, column: 3, scope: !196)
!228 = distinct !{!228, !199, !229, !225}
!229 = !DILocation(line: 39, column: 46, scope: !193)
!230 = !DILocation(line: 40, column: 10, scope: !231)
!231 = distinct !DILexicalBlock(scope: !158, file: !2, line: 40, column: 3)
!232 = !DILocation(line: 40, column: 8, scope: !231)
!233 = !DILocation(line: 40, column: 15, scope: !234)
!234 = distinct !DILexicalBlock(scope: !231, file: !2, line: 40, column: 3)
!235 = !DILocation(line: 40, column: 19, scope: !234)
!236 = !DILocation(line: 40, column: 17, scope: !234)
!237 = !DILocation(line: 40, column: 3, scope: !231)
!238 = !DILocation(line: 41, column: 12, scope: !239)
!239 = distinct !DILexicalBlock(scope: !234, file: !2, line: 41, column: 5)
!240 = !DILocation(line: 41, column: 10, scope: !239)
!241 = !DILocation(line: 41, column: 17, scope: !242)
!242 = distinct !DILexicalBlock(scope: !239, file: !2, line: 41, column: 5)
!243 = !DILocation(line: 41, column: 21, scope: !242)
!244 = !DILocation(line: 41, column: 19, scope: !242)
!245 = !DILocation(line: 41, column: 5, scope: !239)
!246 = !DILocation(line: 42, column: 30, scope: !242)
!247 = !DILocation(line: 42, column: 33, scope: !242)
!248 = !DILocation(line: 42, column: 34, scope: !242)
!249 = !DILocation(line: 42, column: 31, scope: !242)
!250 = !DILocation(line: 42, column: 40, scope: !242)
!251 = !DILocation(line: 42, column: 38, scope: !242)
!252 = !DILocation(line: 42, column: 17, scope: !242)
!253 = !DILocation(line: 42, column: 46, scope: !242)
!254 = !DILocation(line: 42, column: 44, scope: !242)
!255 = !DILocation(line: 42, column: 7, scope: !242)
!256 = !DILocation(line: 42, column: 9, scope: !242)
!257 = !DILocation(line: 42, column: 12, scope: !242)
!258 = !DILocation(line: 42, column: 15, scope: !242)
!259 = !DILocation(line: 41, column: 26, scope: !242)
!260 = !DILocation(line: 41, column: 5, scope: !242)
!261 = distinct !{!261, !245, !262, !225}
!262 = !DILocation(line: 42, column: 46, scope: !239)
!263 = !DILocation(line: 40, column: 24, scope: !234)
!264 = !DILocation(line: 40, column: 3, scope: !234)
!265 = distinct !{!265, !237, !266, !225}
!266 = !DILocation(line: 42, column: 46, scope: !231)
!267 = !DILocation(line: 43, column: 10, scope: !268)
!268 = distinct !DILexicalBlock(scope: !158, file: !2, line: 43, column: 3)
!269 = !DILocation(line: 43, column: 8, scope: !268)
!270 = !DILocation(line: 43, column: 15, scope: !271)
!271 = distinct !DILexicalBlock(scope: !268, file: !2, line: 43, column: 3)
!272 = !DILocation(line: 43, column: 19, scope: !271)
!273 = !DILocation(line: 43, column: 17, scope: !271)
!274 = !DILocation(line: 43, column: 3, scope: !268)
!275 = !DILocation(line: 44, column: 12, scope: !276)
!276 = distinct !DILexicalBlock(scope: !271, file: !2, line: 44, column: 5)
!277 = !DILocation(line: 44, column: 10, scope: !276)
!278 = !DILocation(line: 44, column: 17, scope: !279)
!279 = distinct !DILexicalBlock(scope: !276, file: !2, line: 44, column: 5)
!280 = !DILocation(line: 44, column: 21, scope: !279)
!281 = !DILocation(line: 44, column: 19, scope: !279)
!282 = !DILocation(line: 44, column: 5, scope: !276)
!283 = !DILocation(line: 45, column: 30, scope: !279)
!284 = !DILocation(line: 45, column: 33, scope: !279)
!285 = !DILocation(line: 45, column: 34, scope: !279)
!286 = !DILocation(line: 45, column: 31, scope: !279)
!287 = !DILocation(line: 45, column: 40, scope: !279)
!288 = !DILocation(line: 45, column: 38, scope: !279)
!289 = !DILocation(line: 45, column: 17, scope: !279)
!290 = !DILocation(line: 45, column: 46, scope: !279)
!291 = !DILocation(line: 45, column: 44, scope: !279)
!292 = !DILocation(line: 45, column: 7, scope: !279)
!293 = !DILocation(line: 45, column: 9, scope: !279)
!294 = !DILocation(line: 45, column: 12, scope: !279)
!295 = !DILocation(line: 45, column: 15, scope: !279)
!296 = !DILocation(line: 44, column: 26, scope: !279)
!297 = !DILocation(line: 44, column: 5, scope: !279)
!298 = distinct !{!298, !282, !299, !225}
!299 = !DILocation(line: 45, column: 46, scope: !276)
!300 = !DILocation(line: 43, column: 24, scope: !271)
!301 = !DILocation(line: 43, column: 3, scope: !271)
!302 = distinct !{!302, !274, !303, !225}
!303 = !DILocation(line: 45, column: 46, scope: !268)
!304 = !DILocation(line: 46, column: 1, scope: !158)
!305 = distinct !DISubprogram(name: "init_array_long_double", scope: !2, file: !2, line: 49, type: !306, scopeLine: 55, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !87)
!306 = !DISubroutineType(types: !307)
!307 = !{null, !84, !84, !84, !308, !308, !309, !311, !309}
!308 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !68, size: 64)
!309 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !310, size: 64)
!310 = !DICompositeType(tag: DW_TAG_array_type, baseType: !68, size: 3200, elements: !164)
!311 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !312, size: 64)
!312 = !DICompositeType(tag: DW_TAG_array_type, baseType: !68, size: 3840, elements: !167)
!313 = !DILocalVariable(name: "ni", arg: 1, scope: !305, file: !2, line: 49, type: !84)
!314 = !DILocation(line: 49, column: 33, scope: !305)
!315 = !DILocalVariable(name: "nj", arg: 2, scope: !305, file: !2, line: 49, type: !84)
!316 = !DILocation(line: 49, column: 41, scope: !305)
!317 = !DILocalVariable(name: "nk", arg: 3, scope: !305, file: !2, line: 49, type: !84)
!318 = !DILocation(line: 49, column: 49, scope: !305)
!319 = !DILocalVariable(name: "alpha", arg: 4, scope: !305, file: !2, line: 50, type: !308)
!320 = !DILocation(line: 50, column: 16, scope: !305)
!321 = !DILocalVariable(name: "beta", arg: 5, scope: !305, file: !2, line: 51, type: !308)
!322 = !DILocation(line: 51, column: 16, scope: !305)
!323 = !DILocalVariable(name: "C", arg: 6, scope: !305, file: !2, line: 52, type: !309)
!324 = !DILocation(line: 52, column: 15, scope: !305)
!325 = !DILocalVariable(name: "A", arg: 7, scope: !305, file: !2, line: 53, type: !311)
!326 = !DILocation(line: 53, column: 15, scope: !305)
!327 = !DILocalVariable(name: "B", arg: 8, scope: !305, file: !2, line: 54, type: !309)
!328 = !DILocation(line: 54, column: 15, scope: !305)
!329 = !DILocalVariable(name: "i", scope: !305, file: !2, line: 56, type: !84)
!330 = !DILocation(line: 56, column: 7, scope: !305)
!331 = !DILocalVariable(name: "j", scope: !305, file: !2, line: 56, type: !84)
!332 = !DILocation(line: 56, column: 10, scope: !305)
!333 = !DILocation(line: 58, column: 4, scope: !305)
!334 = !DILocation(line: 58, column: 10, scope: !305)
!335 = !DILocation(line: 59, column: 4, scope: !305)
!336 = !DILocation(line: 59, column: 9, scope: !305)
!337 = !DILocation(line: 60, column: 10, scope: !338)
!338 = distinct !DILexicalBlock(scope: !305, file: !2, line: 60, column: 3)
!339 = !DILocation(line: 60, column: 8, scope: !338)
!340 = !DILocation(line: 60, column: 15, scope: !341)
!341 = distinct !DILexicalBlock(scope: !338, file: !2, line: 60, column: 3)
!342 = !DILocation(line: 60, column: 19, scope: !341)
!343 = !DILocation(line: 60, column: 17, scope: !341)
!344 = !DILocation(line: 60, column: 3, scope: !338)
!345 = !DILocation(line: 61, column: 12, scope: !346)
!346 = distinct !DILexicalBlock(scope: !341, file: !2, line: 61, column: 5)
!347 = !DILocation(line: 61, column: 10, scope: !346)
!348 = !DILocation(line: 61, column: 17, scope: !349)
!349 = distinct !DILexicalBlock(scope: !346, file: !2, line: 61, column: 5)
!350 = !DILocation(line: 61, column: 21, scope: !349)
!351 = !DILocation(line: 61, column: 19, scope: !349)
!352 = !DILocation(line: 61, column: 5, scope: !346)
!353 = !DILocation(line: 62, column: 33, scope: !349)
!354 = !DILocation(line: 62, column: 35, scope: !349)
!355 = !DILocation(line: 62, column: 34, scope: !349)
!356 = !DILocation(line: 62, column: 36, scope: !349)
!357 = !DILocation(line: 62, column: 42, scope: !349)
!358 = !DILocation(line: 62, column: 40, scope: !349)
!359 = !DILocation(line: 62, column: 17, scope: !349)
!360 = !DILocation(line: 62, column: 48, scope: !349)
!361 = !DILocation(line: 62, column: 46, scope: !349)
!362 = !DILocation(line: 62, column: 7, scope: !349)
!363 = !DILocation(line: 62, column: 9, scope: !349)
!364 = !DILocation(line: 62, column: 12, scope: !349)
!365 = !DILocation(line: 62, column: 15, scope: !349)
!366 = !DILocation(line: 61, column: 26, scope: !349)
!367 = !DILocation(line: 61, column: 5, scope: !349)
!368 = distinct !{!368, !352, !369, !225}
!369 = !DILocation(line: 62, column: 48, scope: !346)
!370 = !DILocation(line: 60, column: 24, scope: !341)
!371 = !DILocation(line: 60, column: 3, scope: !341)
!372 = distinct !{!372, !344, !373, !225}
!373 = !DILocation(line: 62, column: 48, scope: !338)
!374 = !DILocation(line: 63, column: 10, scope: !375)
!375 = distinct !DILexicalBlock(scope: !305, file: !2, line: 63, column: 3)
!376 = !DILocation(line: 63, column: 8, scope: !375)
!377 = !DILocation(line: 63, column: 15, scope: !378)
!378 = distinct !DILexicalBlock(scope: !375, file: !2, line: 63, column: 3)
!379 = !DILocation(line: 63, column: 19, scope: !378)
!380 = !DILocation(line: 63, column: 17, scope: !378)
!381 = !DILocation(line: 63, column: 3, scope: !375)
!382 = !DILocation(line: 64, column: 12, scope: !383)
!383 = distinct !DILexicalBlock(scope: !378, file: !2, line: 64, column: 5)
!384 = !DILocation(line: 64, column: 10, scope: !383)
!385 = !DILocation(line: 64, column: 17, scope: !386)
!386 = distinct !DILexicalBlock(scope: !383, file: !2, line: 64, column: 5)
!387 = !DILocation(line: 64, column: 21, scope: !386)
!388 = !DILocation(line: 64, column: 19, scope: !386)
!389 = !DILocation(line: 64, column: 5, scope: !383)
!390 = !DILocation(line: 65, column: 32, scope: !386)
!391 = !DILocation(line: 65, column: 35, scope: !386)
!392 = !DILocation(line: 65, column: 36, scope: !386)
!393 = !DILocation(line: 65, column: 33, scope: !386)
!394 = !DILocation(line: 65, column: 42, scope: !386)
!395 = !DILocation(line: 65, column: 40, scope: !386)
!396 = !DILocation(line: 65, column: 17, scope: !386)
!397 = !DILocation(line: 65, column: 48, scope: !386)
!398 = !DILocation(line: 65, column: 46, scope: !386)
!399 = !DILocation(line: 65, column: 7, scope: !386)
!400 = !DILocation(line: 65, column: 9, scope: !386)
!401 = !DILocation(line: 65, column: 12, scope: !386)
!402 = !DILocation(line: 65, column: 15, scope: !386)
!403 = !DILocation(line: 64, column: 26, scope: !386)
!404 = !DILocation(line: 64, column: 5, scope: !386)
!405 = distinct !{!405, !389, !406, !225}
!406 = !DILocation(line: 65, column: 48, scope: !383)
!407 = !DILocation(line: 63, column: 24, scope: !378)
!408 = !DILocation(line: 63, column: 3, scope: !378)
!409 = distinct !{!409, !381, !410, !225}
!410 = !DILocation(line: 65, column: 48, scope: !375)
!411 = !DILocation(line: 66, column: 10, scope: !412)
!412 = distinct !DILexicalBlock(scope: !305, file: !2, line: 66, column: 3)
!413 = !DILocation(line: 66, column: 8, scope: !412)
!414 = !DILocation(line: 66, column: 15, scope: !415)
!415 = distinct !DILexicalBlock(scope: !412, file: !2, line: 66, column: 3)
!416 = !DILocation(line: 66, column: 19, scope: !415)
!417 = !DILocation(line: 66, column: 17, scope: !415)
!418 = !DILocation(line: 66, column: 3, scope: !412)
!419 = !DILocation(line: 67, column: 12, scope: !420)
!420 = distinct !DILexicalBlock(scope: !415, file: !2, line: 67, column: 5)
!421 = !DILocation(line: 67, column: 10, scope: !420)
!422 = !DILocation(line: 67, column: 17, scope: !423)
!423 = distinct !DILexicalBlock(scope: !420, file: !2, line: 67, column: 5)
!424 = !DILocation(line: 67, column: 21, scope: !423)
!425 = !DILocation(line: 67, column: 19, scope: !423)
!426 = !DILocation(line: 67, column: 5, scope: !420)
!427 = !DILocation(line: 68, column: 32, scope: !423)
!428 = !DILocation(line: 68, column: 35, scope: !423)
!429 = !DILocation(line: 68, column: 36, scope: !423)
!430 = !DILocation(line: 68, column: 33, scope: !423)
!431 = !DILocation(line: 68, column: 42, scope: !423)
!432 = !DILocation(line: 68, column: 40, scope: !423)
!433 = !DILocation(line: 68, column: 17, scope: !423)
!434 = !DILocation(line: 68, column: 48, scope: !423)
!435 = !DILocation(line: 68, column: 46, scope: !423)
!436 = !DILocation(line: 68, column: 7, scope: !423)
!437 = !DILocation(line: 68, column: 9, scope: !423)
!438 = !DILocation(line: 68, column: 12, scope: !423)
!439 = !DILocation(line: 68, column: 15, scope: !423)
!440 = !DILocation(line: 67, column: 26, scope: !423)
!441 = !DILocation(line: 67, column: 5, scope: !423)
!442 = distinct !{!442, !426, !443, !225}
!443 = !DILocation(line: 68, column: 48, scope: !420)
!444 = !DILocation(line: 66, column: 24, scope: !415)
!445 = !DILocation(line: 66, column: 3, scope: !415)
!446 = distinct !{!446, !418, !447, !225}
!447 = !DILocation(line: 68, column: 48, scope: !412)
!448 = !DILocation(line: 69, column: 1, scope: !305)
!449 = distinct !DISubprogram(name: "kernel_gemm", scope: !2, file: !2, line: 150, type: !450, scopeLine: 156, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !87)
!450 = !DISubroutineType(types: !451)
!451 = !{null, !84, !84, !84, !55, !55, !162, !165, !162}
!452 = !DILocalVariable(name: "ni", arg: 1, scope: !449, file: !2, line: 150, type: !84)
!453 = !DILocation(line: 150, column: 22, scope: !449)
!454 = !DILocalVariable(name: "nj", arg: 2, scope: !449, file: !2, line: 150, type: !84)
!455 = !DILocation(line: 150, column: 30, scope: !449)
!456 = !DILocalVariable(name: "nk", arg: 3, scope: !449, file: !2, line: 150, type: !84)
!457 = !DILocation(line: 150, column: 38, scope: !449)
!458 = !DILocalVariable(name: "alpha", arg: 4, scope: !449, file: !2, line: 151, type: !55)
!459 = !DILocation(line: 151, column: 14, scope: !449)
!460 = !DILocalVariable(name: "beta", arg: 5, scope: !449, file: !2, line: 152, type: !55)
!461 = !DILocation(line: 152, column: 14, scope: !449)
!462 = !DILocalVariable(name: "C", arg: 6, scope: !449, file: !2, line: 153, type: !162)
!463 = !DILocation(line: 153, column: 14, scope: !449)
!464 = !DILocalVariable(name: "A", arg: 7, scope: !449, file: !2, line: 154, type: !165)
!465 = !DILocation(line: 154, column: 14, scope: !449)
!466 = !DILocalVariable(name: "B", arg: 8, scope: !449, file: !2, line: 155, type: !162)
!467 = !DILocation(line: 155, column: 14, scope: !449)
!468 = !DILocalVariable(name: "i", scope: !449, file: !2, line: 157, type: !84)
!469 = !DILocation(line: 157, column: 7, scope: !449)
!470 = !DILocalVariable(name: "j", scope: !449, file: !2, line: 157, type: !84)
!471 = !DILocation(line: 157, column: 10, scope: !449)
!472 = !DILocalVariable(name: "k", scope: !449, file: !2, line: 157, type: !84)
!473 = !DILocation(line: 157, column: 13, scope: !449)
!474 = !DILocation(line: 167, column: 10, scope: !475)
!475 = distinct !DILexicalBlock(scope: !449, file: !2, line: 167, column: 3)
!476 = !DILocation(line: 167, column: 8, scope: !475)
!477 = !DILocation(line: 167, column: 15, scope: !478)
!478 = distinct !DILexicalBlock(scope: !475, file: !2, line: 167, column: 3)
!479 = !DILocation(line: 167, column: 19, scope: !478)
!480 = !DILocation(line: 167, column: 17, scope: !478)
!481 = !DILocation(line: 167, column: 3, scope: !475)
!482 = !DILocation(line: 168, column: 12, scope: !483)
!483 = distinct !DILexicalBlock(scope: !484, file: !2, line: 168, column: 5)
!484 = distinct !DILexicalBlock(scope: !478, file: !2, line: 167, column: 32)
!485 = !DILocation(line: 168, column: 10, scope: !483)
!486 = !DILocation(line: 168, column: 17, scope: !487)
!487 = distinct !DILexicalBlock(scope: !483, file: !2, line: 168, column: 5)
!488 = !DILocation(line: 168, column: 21, scope: !487)
!489 = !DILocation(line: 168, column: 19, scope: !487)
!490 = !DILocation(line: 168, column: 5, scope: !483)
!491 = !DILocation(line: 169, column: 13, scope: !487)
!492 = !DILocation(line: 169, column: 2, scope: !487)
!493 = !DILocation(line: 169, column: 4, scope: !487)
!494 = !DILocation(line: 169, column: 7, scope: !487)
!495 = !DILocation(line: 169, column: 10, scope: !487)
!496 = !DILocation(line: 168, column: 30, scope: !487)
!497 = !DILocation(line: 168, column: 5, scope: !487)
!498 = distinct !{!498, !490, !499, !225}
!499 = !DILocation(line: 169, column: 13, scope: !483)
!500 = !DILocation(line: 170, column: 12, scope: !501)
!501 = distinct !DILexicalBlock(scope: !484, file: !2, line: 170, column: 5)
!502 = !DILocation(line: 170, column: 10, scope: !501)
!503 = !DILocation(line: 170, column: 17, scope: !504)
!504 = distinct !DILexicalBlock(scope: !501, file: !2, line: 170, column: 5)
!505 = !DILocation(line: 170, column: 21, scope: !504)
!506 = !DILocation(line: 170, column: 19, scope: !504)
!507 = !DILocation(line: 170, column: 5, scope: !501)
!508 = !DILocation(line: 171, column: 15, scope: !509)
!509 = distinct !DILexicalBlock(scope: !510, file: !2, line: 171, column: 8)
!510 = distinct !DILexicalBlock(scope: !504, file: !2, line: 170, column: 34)
!511 = !DILocation(line: 171, column: 13, scope: !509)
!512 = !DILocation(line: 171, column: 20, scope: !513)
!513 = distinct !DILexicalBlock(scope: !509, file: !2, line: 171, column: 8)
!514 = !DILocation(line: 171, column: 24, scope: !513)
!515 = !DILocation(line: 171, column: 22, scope: !513)
!516 = !DILocation(line: 171, column: 8, scope: !509)
!517 = !DILocation(line: 172, column: 15, scope: !513)
!518 = !DILocation(line: 172, column: 23, scope: !513)
!519 = !DILocation(line: 172, column: 25, scope: !513)
!520 = !DILocation(line: 172, column: 28, scope: !513)
!521 = !DILocation(line: 172, column: 21, scope: !513)
!522 = !DILocation(line: 172, column: 33, scope: !513)
!523 = !DILocation(line: 172, column: 35, scope: !513)
!524 = !DILocation(line: 172, column: 38, scope: !513)
!525 = !DILocation(line: 172, column: 4, scope: !513)
!526 = !DILocation(line: 172, column: 6, scope: !513)
!527 = !DILocation(line: 172, column: 9, scope: !513)
!528 = !DILocation(line: 172, column: 12, scope: !513)
!529 = !DILocation(line: 171, column: 33, scope: !513)
!530 = !DILocation(line: 171, column: 8, scope: !513)
!531 = distinct !{!531, !516, !532, !225}
!532 = !DILocation(line: 172, column: 39, scope: !509)
!533 = !DILocation(line: 173, column: 5, scope: !510)
!534 = !DILocation(line: 170, column: 30, scope: !504)
!535 = !DILocation(line: 170, column: 5, scope: !504)
!536 = distinct !{!536, !507, !537, !225}
!537 = !DILocation(line: 173, column: 5, scope: !501)
!538 = !DILocation(line: 174, column: 3, scope: !484)
!539 = !DILocation(line: 167, column: 28, scope: !478)
!540 = !DILocation(line: 167, column: 3, scope: !478)
!541 = distinct !{!541, !481, !542, !225}
!542 = !DILocation(line: 174, column: 3, scope: !475)
!543 = !DILocation(line: 177, column: 1, scope: !449)
!544 = distinct !DISubprogram(name: "kernel_gemm_long_double", scope: !2, file: !2, line: 180, type: !545, scopeLine: 186, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !87)
!545 = !DISubroutineType(types: !546)
!546 = !{null, !84, !84, !84, !68, !68, !309, !311, !309}
!547 = !DILocalVariable(name: "ni", arg: 1, scope: !544, file: !2, line: 180, type: !84)
!548 = !DILocation(line: 180, column: 34, scope: !544)
!549 = !DILocalVariable(name: "nj", arg: 2, scope: !544, file: !2, line: 180, type: !84)
!550 = !DILocation(line: 180, column: 42, scope: !544)
!551 = !DILocalVariable(name: "nk", arg: 3, scope: !544, file: !2, line: 180, type: !84)
!552 = !DILocation(line: 180, column: 50, scope: !544)
!553 = !DILocalVariable(name: "alpha", arg: 4, scope: !544, file: !2, line: 181, type: !68)
!554 = !DILocation(line: 181, column: 16, scope: !544)
!555 = !DILocalVariable(name: "beta", arg: 5, scope: !544, file: !2, line: 182, type: !68)
!556 = !DILocation(line: 182, column: 16, scope: !544)
!557 = !DILocalVariable(name: "C", arg: 6, scope: !544, file: !2, line: 183, type: !309)
!558 = !DILocation(line: 183, column: 16, scope: !544)
!559 = !DILocalVariable(name: "A", arg: 7, scope: !544, file: !2, line: 184, type: !311)
!560 = !DILocation(line: 184, column: 16, scope: !544)
!561 = !DILocalVariable(name: "B", arg: 8, scope: !544, file: !2, line: 185, type: !309)
!562 = !DILocation(line: 185, column: 16, scope: !544)
!563 = !DILocalVariable(name: "i", scope: !544, file: !2, line: 187, type: !84)
!564 = !DILocation(line: 187, column: 7, scope: !544)
!565 = !DILocalVariable(name: "j", scope: !544, file: !2, line: 187, type: !84)
!566 = !DILocation(line: 187, column: 10, scope: !544)
!567 = !DILocalVariable(name: "k", scope: !544, file: !2, line: 187, type: !84)
!568 = !DILocation(line: 187, column: 13, scope: !544)
!569 = !DILocation(line: 190, column: 10, scope: !570)
!570 = distinct !DILexicalBlock(scope: !544, file: !2, line: 190, column: 3)
!571 = !DILocation(line: 190, column: 8, scope: !570)
!572 = !DILocation(line: 190, column: 15, scope: !573)
!573 = distinct !DILexicalBlock(scope: !570, file: !2, line: 190, column: 3)
!574 = !DILocation(line: 190, column: 19, scope: !573)
!575 = !DILocation(line: 190, column: 17, scope: !573)
!576 = !DILocation(line: 190, column: 3, scope: !570)
!577 = !DILocation(line: 191, column: 12, scope: !578)
!578 = distinct !DILexicalBlock(scope: !579, file: !2, line: 191, column: 5)
!579 = distinct !DILexicalBlock(scope: !573, file: !2, line: 190, column: 32)
!580 = !DILocation(line: 191, column: 10, scope: !578)
!581 = !DILocation(line: 191, column: 17, scope: !582)
!582 = distinct !DILexicalBlock(scope: !578, file: !2, line: 191, column: 5)
!583 = !DILocation(line: 191, column: 21, scope: !582)
!584 = !DILocation(line: 191, column: 19, scope: !582)
!585 = !DILocation(line: 191, column: 5, scope: !578)
!586 = !DILocation(line: 192, column: 13, scope: !582)
!587 = !DILocation(line: 192, column: 2, scope: !582)
!588 = !DILocation(line: 192, column: 4, scope: !582)
!589 = !DILocation(line: 192, column: 7, scope: !582)
!590 = !DILocation(line: 192, column: 10, scope: !582)
!591 = !DILocation(line: 191, column: 30, scope: !582)
!592 = !DILocation(line: 191, column: 5, scope: !582)
!593 = distinct !{!593, !585, !594, !225}
!594 = !DILocation(line: 192, column: 13, scope: !578)
!595 = !DILocation(line: 193, column: 12, scope: !596)
!596 = distinct !DILexicalBlock(scope: !579, file: !2, line: 193, column: 5)
!597 = !DILocation(line: 193, column: 10, scope: !596)
!598 = !DILocation(line: 193, column: 17, scope: !599)
!599 = distinct !DILexicalBlock(scope: !596, file: !2, line: 193, column: 5)
!600 = !DILocation(line: 193, column: 21, scope: !599)
!601 = !DILocation(line: 193, column: 19, scope: !599)
!602 = !DILocation(line: 193, column: 5, scope: !596)
!603 = !DILocation(line: 194, column: 15, scope: !604)
!604 = distinct !DILexicalBlock(scope: !605, file: !2, line: 194, column: 8)
!605 = distinct !DILexicalBlock(scope: !599, file: !2, line: 193, column: 34)
!606 = !DILocation(line: 194, column: 13, scope: !604)
!607 = !DILocation(line: 194, column: 20, scope: !608)
!608 = distinct !DILexicalBlock(scope: !604, file: !2, line: 194, column: 8)
!609 = !DILocation(line: 194, column: 24, scope: !608)
!610 = !DILocation(line: 194, column: 22, scope: !608)
!611 = !DILocation(line: 194, column: 8, scope: !604)
!612 = !DILocation(line: 195, column: 15, scope: !608)
!613 = !DILocation(line: 195, column: 23, scope: !608)
!614 = !DILocation(line: 195, column: 25, scope: !608)
!615 = !DILocation(line: 195, column: 28, scope: !608)
!616 = !DILocation(line: 195, column: 21, scope: !608)
!617 = !DILocation(line: 195, column: 33, scope: !608)
!618 = !DILocation(line: 195, column: 35, scope: !608)
!619 = !DILocation(line: 195, column: 38, scope: !608)
!620 = !DILocation(line: 195, column: 4, scope: !608)
!621 = !DILocation(line: 195, column: 6, scope: !608)
!622 = !DILocation(line: 195, column: 9, scope: !608)
!623 = !DILocation(line: 195, column: 12, scope: !608)
!624 = !DILocation(line: 194, column: 33, scope: !608)
!625 = !DILocation(line: 194, column: 8, scope: !608)
!626 = distinct !{!626, !611, !627, !225}
!627 = !DILocation(line: 195, column: 39, scope: !604)
!628 = !DILocation(line: 196, column: 5, scope: !605)
!629 = !DILocation(line: 193, column: 30, scope: !599)
!630 = !DILocation(line: 193, column: 5, scope: !599)
!631 = distinct !{!631, !602, !632, !225}
!632 = !DILocation(line: 196, column: 5, scope: !596)
!633 = !DILocation(line: 197, column: 3, scope: !579)
!634 = !DILocation(line: 190, column: 28, scope: !573)
!635 = !DILocation(line: 190, column: 3, scope: !573)
!636 = distinct !{!636, !576, !637, !225}
!637 = !DILocation(line: 197, column: 3, scope: !570)
!638 = !DILocation(line: 200, column: 1, scope: !544)
!639 = distinct !DISubprogram(name: "print_array", scope: !2, file: !2, line: 74, type: !640, scopeLine: 76, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !51, retainedNodes: !87)
!640 = !DISubroutineType(types: !641)
!641 = !{null, !84, !84, !162, !309}
!642 = !DILocalVariable(name: "ni", arg: 1, scope: !639, file: !2, line: 74, type: !84)
!643 = !DILocation(line: 74, column: 22, scope: !639)
!644 = !DILocalVariable(name: "nj", arg: 2, scope: !639, file: !2, line: 74, type: !84)
!645 = !DILocation(line: 74, column: 30, scope: !639)
!646 = !DILocalVariable(name: "C", arg: 3, scope: !639, file: !2, line: 75, type: !162)
!647 = !DILocation(line: 75, column: 14, scope: !639)
!648 = !DILocalVariable(name: "C_long_double", arg: 4, scope: !639, file: !2, line: 75, type: !309)
!649 = !DILocation(line: 75, column: 55, scope: !639)
!650 = !DILocalVariable(name: "i", scope: !639, file: !2, line: 77, type: !84)
!651 = !DILocation(line: 77, column: 7, scope: !639)
!652 = !DILocalVariable(name: "j", scope: !639, file: !2, line: 77, type: !84)
!653 = !DILocation(line: 77, column: 10, scope: !639)
!654 = !DILocalVariable(name: "max_value", scope: !639, file: !2, line: 79, type: !55)
!655 = !DILocation(line: 79, column: 13, scope: !639)
!656 = !DILocalVariable(name: "sum", scope: !639, file: !2, line: 80, type: !55)
!657 = !DILocation(line: 80, column: 13, scope: !639)
!658 = !DILocalVariable(name: "norm", scope: !639, file: !2, line: 81, type: !55)
!659 = !DILocation(line: 81, column: 13, scope: !639)
!660 = !DILocalVariable(name: "max_value_double", scope: !639, file: !2, line: 83, type: !68)
!661 = !DILocation(line: 83, column: 15, scope: !639)
!662 = !DILocalVariable(name: "sum_double", scope: !639, file: !2, line: 84, type: !68)
!663 = !DILocation(line: 84, column: 15, scope: !639)
!664 = !DILocalVariable(name: "norm_double", scope: !639, file: !2, line: 85, type: !68)
!665 = !DILocation(line: 85, column: 15, scope: !639)
!666 = !DILocation(line: 87, column: 3, scope: !639)
!667 = !DILocation(line: 88, column: 3, scope: !639)
!668 = !DILocation(line: 95, column: 10, scope: !669)
!669 = distinct !DILexicalBlock(scope: !639, file: !2, line: 95, column: 3)
!670 = !DILocation(line: 95, column: 8, scope: !669)
!671 = !DILocation(line: 95, column: 15, scope: !672)
!672 = distinct !DILexicalBlock(scope: !669, file: !2, line: 95, column: 3)
!673 = !DILocation(line: 95, column: 19, scope: !672)
!674 = !DILocation(line: 95, column: 17, scope: !672)
!675 = !DILocation(line: 95, column: 3, scope: !669)
!676 = !DILocation(line: 96, column: 12, scope: !677)
!677 = distinct !DILexicalBlock(scope: !672, file: !2, line: 96, column: 5)
!678 = !DILocation(line: 96, column: 10, scope: !677)
!679 = !DILocation(line: 96, column: 17, scope: !680)
!680 = distinct !DILexicalBlock(scope: !677, file: !2, line: 96, column: 5)
!681 = !DILocation(line: 96, column: 21, scope: !680)
!682 = !DILocation(line: 96, column: 19, scope: !680)
!683 = !DILocation(line: 96, column: 5, scope: !677)
!684 = !DILocalVariable(name: "value", scope: !685, file: !2, line: 97, type: !55)
!685 = distinct !DILexicalBlock(scope: !680, file: !2, line: 96, column: 30)
!686 = !DILocation(line: 97, column: 17, scope: !685)
!687 = !DILocation(line: 97, column: 25, scope: !685)
!688 = !DILocation(line: 97, column: 27, scope: !685)
!689 = !DILocation(line: 97, column: 30, scope: !685)
!690 = !DILocalVariable(name: "value_double", scope: !685, file: !2, line: 98, type: !68)
!691 = !DILocation(line: 98, column: 19, scope: !685)
!692 = !DILocation(line: 98, column: 34, scope: !685)
!693 = !DILocation(line: 98, column: 48, scope: !685)
!694 = !DILocation(line: 98, column: 51, scope: !685)
!695 = !DILocation(line: 100, column: 9, scope: !696)
!696 = distinct !DILexicalBlock(scope: !685, file: !2, line: 100, column: 9)
!697 = !DILocation(line: 100, column: 15, scope: !696)
!698 = !DILocation(line: 101, column: 16, scope: !696)
!699 = !DILocation(line: 101, column: 15, scope: !696)
!700 = !DILocation(line: 101, column: 13, scope: !696)
!701 = !DILocation(line: 101, column: 7, scope: !696)
!702 = !DILocation(line: 103, column: 9, scope: !703)
!703 = distinct !DILexicalBlock(scope: !685, file: !2, line: 103, column: 9)
!704 = !DILocation(line: 103, column: 22, scope: !703)
!705 = !DILocation(line: 104, column: 23, scope: !703)
!706 = !DILocation(line: 104, column: 22, scope: !703)
!707 = !DILocation(line: 104, column: 20, scope: !703)
!708 = !DILocation(line: 104, column: 7, scope: !703)
!709 = !DILocation(line: 106, column: 9, scope: !710)
!710 = distinct !DILexicalBlock(scope: !685, file: !2, line: 106, column: 9)
!711 = !DILocation(line: 106, column: 17, scope: !710)
!712 = !DILocation(line: 106, column: 15, scope: !710)
!713 = !DILocation(line: 107, column: 19, scope: !714)
!714 = distinct !DILexicalBlock(scope: !710, file: !2, line: 106, column: 28)
!715 = !DILocation(line: 107, column: 17, scope: !714)
!716 = !DILocation(line: 108, column: 5, scope: !714)
!717 = !DILocation(line: 109, column: 9, scope: !718)
!718 = distinct !DILexicalBlock(scope: !685, file: !2, line: 109, column: 9)
!719 = !DILocation(line: 109, column: 24, scope: !718)
!720 = !DILocation(line: 109, column: 22, scope: !718)
!721 = !DILocation(line: 110, column: 26, scope: !722)
!722 = distinct !DILexicalBlock(scope: !718, file: !2, line: 109, column: 42)
!723 = !DILocation(line: 110, column: 24, scope: !722)
!724 = !DILocation(line: 111, column: 5, scope: !722)
!725 = !DILocation(line: 112, column: 3, scope: !685)
!726 = !DILocation(line: 96, column: 26, scope: !680)
!727 = !DILocation(line: 96, column: 5, scope: !680)
!728 = distinct !{!728, !683, !729, !225}
!729 = !DILocation(line: 112, column: 3, scope: !677)
!730 = !DILocation(line: 95, column: 24, scope: !672)
!731 = !DILocation(line: 95, column: 3, scope: !672)
!732 = distinct !{!732, !675, !733, !225}
!733 = !DILocation(line: 112, column: 3, scope: !669)
!734 = !DILocation(line: 114, column: 7, scope: !735)
!735 = distinct !DILexicalBlock(scope: !639, file: !2, line: 114, column: 7)
!736 = !DILocation(line: 114, column: 17, scope: !735)
!737 = !DILocation(line: 115, column: 12, scope: !738)
!738 = distinct !DILexicalBlock(scope: !739, file: !2, line: 115, column: 5)
!739 = distinct !DILexicalBlock(scope: !735, file: !2, line: 114, column: 23)
!740 = !DILocation(line: 115, column: 10, scope: !738)
!741 = !DILocation(line: 115, column: 17, scope: !742)
!742 = distinct !DILexicalBlock(scope: !738, file: !2, line: 115, column: 5)
!743 = !DILocation(line: 115, column: 21, scope: !742)
!744 = !DILocation(line: 115, column: 19, scope: !742)
!745 = !DILocation(line: 115, column: 5, scope: !738)
!746 = !DILocation(line: 116, column: 14, scope: !747)
!747 = distinct !DILexicalBlock(scope: !748, file: !2, line: 116, column: 7)
!748 = distinct !DILexicalBlock(scope: !742, file: !2, line: 115, column: 30)
!749 = !DILocation(line: 116, column: 12, scope: !747)
!750 = !DILocation(line: 116, column: 19, scope: !751)
!751 = distinct !DILexicalBlock(scope: !747, file: !2, line: 116, column: 7)
!752 = !DILocation(line: 116, column: 23, scope: !751)
!753 = !DILocation(line: 116, column: 21, scope: !751)
!754 = !DILocation(line: 116, column: 7, scope: !747)
!755 = !DILocalVariable(name: "scaled", scope: !756, file: !2, line: 117, type: !55)
!756 = distinct !DILexicalBlock(scope: !751, file: !2, line: 116, column: 32)
!757 = !DILocation(line: 117, column: 19, scope: !756)
!758 = !DILocation(line: 117, column: 28, scope: !756)
!759 = !DILocation(line: 117, column: 30, scope: !756)
!760 = !DILocation(line: 117, column: 33, scope: !756)
!761 = !DILocation(line: 117, column: 38, scope: !756)
!762 = !DILocation(line: 117, column: 36, scope: !756)
!763 = !DILocation(line: 118, column: 16, scope: !756)
!764 = !DILocation(line: 118, column: 25, scope: !756)
!765 = !DILocation(line: 118, column: 13, scope: !756)
!766 = !DILocation(line: 119, column: 7, scope: !756)
!767 = !DILocation(line: 116, column: 28, scope: !751)
!768 = !DILocation(line: 116, column: 7, scope: !751)
!769 = distinct !{!769, !754, !770, !225}
!770 = !DILocation(line: 119, column: 7, scope: !747)
!771 = !DILocation(line: 120, column: 5, scope: !748)
!772 = !DILocation(line: 115, column: 26, scope: !742)
!773 = !DILocation(line: 115, column: 5, scope: !742)
!774 = distinct !{!774, !745, !775, !225}
!775 = !DILocation(line: 120, column: 5, scope: !738)
!776 = !DILocation(line: 121, column: 12, scope: !739)
!777 = !DILocation(line: 121, column: 10, scope: !739)
!778 = !DILocation(line: 122, column: 3, scope: !739)
!779 = !DILocation(line: 124, column: 7, scope: !780)
!780 = distinct !DILexicalBlock(scope: !639, file: !2, line: 124, column: 7)
!781 = !DILocation(line: 124, column: 24, scope: !780)
!782 = !DILocation(line: 125, column: 12, scope: !783)
!783 = distinct !DILexicalBlock(scope: !784, file: !2, line: 125, column: 5)
!784 = distinct !DILexicalBlock(scope: !780, file: !2, line: 124, column: 30)
!785 = !DILocation(line: 125, column: 10, scope: !783)
!786 = !DILocation(line: 125, column: 17, scope: !787)
!787 = distinct !DILexicalBlock(scope: !783, file: !2, line: 125, column: 5)
!788 = !DILocation(line: 125, column: 21, scope: !787)
!789 = !DILocation(line: 125, column: 19, scope: !787)
!790 = !DILocation(line: 125, column: 5, scope: !783)
!791 = !DILocation(line: 126, column: 14, scope: !792)
!792 = distinct !DILexicalBlock(scope: !793, file: !2, line: 126, column: 7)
!793 = distinct !DILexicalBlock(scope: !787, file: !2, line: 125, column: 30)
!794 = !DILocation(line: 126, column: 12, scope: !792)
!795 = !DILocation(line: 126, column: 19, scope: !796)
!796 = distinct !DILexicalBlock(scope: !792, file: !2, line: 126, column: 7)
!797 = !DILocation(line: 126, column: 23, scope: !796)
!798 = !DILocation(line: 126, column: 21, scope: !796)
!799 = !DILocation(line: 126, column: 7, scope: !792)
!800 = !DILocalVariable(name: "scaled", scope: !801, file: !2, line: 127, type: !68)
!801 = distinct !DILexicalBlock(scope: !796, file: !2, line: 126, column: 32)
!802 = !DILocation(line: 127, column: 21, scope: !801)
!803 = !DILocation(line: 127, column: 30, scope: !801)
!804 = !DILocation(line: 127, column: 44, scope: !801)
!805 = !DILocation(line: 127, column: 47, scope: !801)
!806 = !DILocation(line: 127, column: 52, scope: !801)
!807 = !DILocation(line: 127, column: 50, scope: !801)
!808 = !DILocation(line: 128, column: 23, scope: !801)
!809 = !DILocation(line: 128, column: 32, scope: !801)
!810 = !DILocation(line: 128, column: 20, scope: !801)
!811 = !DILocation(line: 129, column: 7, scope: !801)
!812 = !DILocation(line: 126, column: 28, scope: !796)
!813 = !DILocation(line: 126, column: 7, scope: !796)
!814 = distinct !{!814, !799, !815, !225}
!815 = !DILocation(line: 129, column: 7, scope: !792)
!816 = !DILocation(line: 130, column: 5, scope: !793)
!817 = !DILocation(line: 125, column: 26, scope: !787)
!818 = !DILocation(line: 125, column: 5, scope: !787)
!819 = distinct !{!819, !790, !820, !225}
!820 = !DILocation(line: 130, column: 5, scope: !783)
!821 = !DILocation(line: 131, column: 25, scope: !784)
!822 = !DILocation(line: 131, column: 19, scope: !784)
!823 = !DILocation(line: 131, column: 17, scope: !784)
!824 = !DILocation(line: 132, column: 3, scope: !784)
!825 = !DILocation(line: 134, column: 12, scope: !639)
!826 = !DILocation(line: 134, column: 62, scope: !639)
!827 = !DILocation(line: 134, column: 3, scope: !639)
!828 = !DILocation(line: 135, column: 12, scope: !639)
!829 = !DILocation(line: 135, column: 57, scope: !639)
!830 = !DILocation(line: 135, column: 3, scope: !639)
!831 = !DILocation(line: 136, column: 12, scope: !639)
!832 = !DILocation(line: 136, column: 73, scope: !639)
!833 = !DILocation(line: 136, column: 3, scope: !639)
!834 = !DILocation(line: 137, column: 12, scope: !639)
!835 = !DILocation(line: 137, column: 68, scope: !639)
!836 = !DILocation(line: 137, column: 3, scope: !639)
!837 = !DILocalVariable(name: "norm_error", scope: !639, file: !2, line: 139, type: !55)
!838 = !DILocation(line: 139, column: 10, scope: !639)
!839 = !DILocation(line: 139, column: 23, scope: !639)
!840 = !DILocation(line: 139, column: 50, scope: !639)
!841 = !DILocation(line: 139, column: 37, scope: !639)
!842 = !DILocation(line: 139, column: 35, scope: !639)
!843 = !DILocation(line: 140, column: 12, scope: !639)
!844 = !DILocation(line: 140, column: 58, scope: !639)
!845 = !DILocation(line: 140, column: 3, scope: !639)
!846 = !DILocation(line: 142, column: 3, scope: !639)
!847 = !DILocation(line: 143, column: 3, scope: !639)
!848 = !DILocation(line: 144, column: 1, scope: !639)

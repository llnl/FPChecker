; ModuleID = 'gemm.c'
source_filename = "gemm.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@stderr = external dso_local global ptr, align 8
@.str = private unnamed_addr constant [23 x i8] c"==BEGIN DUMP_ARRAYS==\0A\00", align 1
@.str.1 = private unnamed_addr constant [15 x i8] c"begin dump: %s\00", align 1
@.str.2 = private unnamed_addr constant [3 x i8] c"C\0A\00", align 1
@.str.3 = private unnamed_addr constant [22 x i8] c"Max value in C: %.7e\0A\00", align 1
@.str.4 = private unnamed_addr constant [17 x i8] c"Norm of C: %.7e\0A\00", align 1
@.str.5 = private unnamed_addr constant [30 x i8] c"Max value in C_double: %.17e\0A\00", align 1
@.str.6 = private unnamed_addr constant [25 x i8] c"Norm of C_double: %.17e\0A\00", align 1
@.str.7 = private unnamed_addr constant [19 x i8] c"Norm error: %.17e\0A\00", align 1
@.str.8 = private unnamed_addr constant [17 x i8] c"\0Aend   dump: %s\0A\00", align 1
@.str.9 = private unnamed_addr constant [2 x i8] c"C\00", align 1
@.str.10 = private unnamed_addr constant [23 x i8] c"==END   DUMP_ARRAYS==\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 noundef %0, ptr noundef %1) #0 {
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
  store ptr %1, ptr %5, align 8
  store i32 20, ptr %6, align 4
  store i32 25, ptr %7, align 4
  store i32 30, ptr %8, align 4
  %19 = call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 4)
  store ptr %19, ptr %13, align 8
  %20 = call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 4)
  store ptr %20, ptr %14, align 8
  %21 = call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 4)
  store ptr %21, ptr %15, align 8
  %22 = call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 8)
  store ptr %22, ptr %16, align 8
  %23 = call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 8)
  store ptr %23, ptr %17, align 8
  %24 = call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 8)
  store ptr %24, ptr %18, align 8
  %25 = load i32, ptr %6, align 4
  %26 = load i32, ptr %7, align 4
  %27 = load i32, ptr %8, align 4
  %28 = load ptr, ptr %13, align 8
  %29 = getelementptr inbounds [20 x [25 x float]], ptr %28, i64 0, i64 0
  %30 = load ptr, ptr %14, align 8
  %31 = getelementptr inbounds [20 x [30 x float]], ptr %30, i64 0, i64 0
  %32 = load ptr, ptr %15, align 8
  %33 = getelementptr inbounds [30 x [25 x float]], ptr %32, i64 0, i64 0
  call void @init_array(i32 noundef %25, i32 noundef %26, i32 noundef %27, ptr noundef %9, ptr noundef %10, ptr noundef %29, ptr noundef %31, ptr noundef %33)
  %34 = load i32, ptr %6, align 4
  %35 = load i32, ptr %7, align 4
  %36 = load i32, ptr %8, align 4
  %37 = load ptr, ptr %16, align 8
  %38 = getelementptr inbounds [20 x [25 x double]], ptr %37, i64 0, i64 0
  %39 = load ptr, ptr %17, align 8
  %40 = getelementptr inbounds [20 x [30 x double]], ptr %39, i64 0, i64 0
  %41 = load ptr, ptr %18, align 8
  %42 = getelementptr inbounds [30 x [25 x double]], ptr %41, i64 0, i64 0
  call void @init_array_double(i32 noundef %34, i32 noundef %35, i32 noundef %36, ptr noundef %11, ptr noundef %12, ptr noundef %38, ptr noundef %40, ptr noundef %42)
  %43 = load i32, ptr %6, align 4
  %44 = load i32, ptr %7, align 4
  %45 = load i32, ptr %8, align 4
  %46 = load float, ptr %9, align 4
  %47 = load float, ptr %10, align 4
  %48 = load ptr, ptr %13, align 8
  %49 = getelementptr inbounds [20 x [25 x float]], ptr %48, i64 0, i64 0
  %50 = load ptr, ptr %14, align 8
  %51 = getelementptr inbounds [20 x [30 x float]], ptr %50, i64 0, i64 0
  %52 = load ptr, ptr %15, align 8
  %53 = getelementptr inbounds [30 x [25 x float]], ptr %52, i64 0, i64 0
  call void @kernel_gemm(i32 noundef %43, i32 noundef %44, i32 noundef %45, float noundef %46, float noundef %47, ptr noundef %49, ptr noundef %51, ptr noundef %53)
  %54 = load i32, ptr %6, align 4
  %55 = load i32, ptr %7, align 4
  %56 = load i32, ptr %8, align 4
  %57 = load double, ptr %11, align 8
  %58 = load double, ptr %12, align 8
  %59 = load ptr, ptr %16, align 8
  %60 = getelementptr inbounds [20 x [25 x double]], ptr %59, i64 0, i64 0
  %61 = load ptr, ptr %17, align 8
  %62 = getelementptr inbounds [20 x [30 x double]], ptr %61, i64 0, i64 0
  %63 = load ptr, ptr %18, align 8
  %64 = getelementptr inbounds [30 x [25 x double]], ptr %63, i64 0, i64 0
  call void @kernel_gemm_double(i32 noundef %54, i32 noundef %55, i32 noundef %56, double noundef %57, double noundef %58, ptr noundef %60, ptr noundef %62, ptr noundef %64)
  %65 = load i32, ptr %6, align 4
  %66 = load i32, ptr %7, align 4
  %67 = load ptr, ptr %13, align 8
  %68 = getelementptr inbounds [20 x [25 x float]], ptr %67, i64 0, i64 0
  %69 = load ptr, ptr %16, align 8
  %70 = getelementptr inbounds [20 x [25 x double]], ptr %69, i64 0, i64 0
  call void @print_array(i32 noundef %65, i32 noundef %66, ptr noundef %68, ptr noundef %70)
  %71 = load ptr, ptr %13, align 8
  call void @free(ptr noundef %71) #4
  %72 = load ptr, ptr %14, align 8
  call void @free(ptr noundef %72) #4
  %73 = load ptr, ptr %15, align 8
  call void @free(ptr noundef %73) #4
  %74 = load ptr, ptr %16, align 8
  call void @free(ptr noundef %74) #4
  %75 = load ptr, ptr %17, align 8
  call void @free(ptr noundef %75) #4
  %76 = load ptr, ptr %18, align 8
  call void @free(ptr noundef %76) #4
  ret i32 0
}

declare dso_local ptr @polybench_alloc_data(i64 noundef, i32 noundef) #1

; Function Attrs: noinline nounwind uwtable
define internal void @init_array(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr noundef %3, ptr noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 {
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
  store i32 %1, ptr %10, align 4
  store i32 %2, ptr %11, align 4
  store ptr %3, ptr %12, align 8
  store ptr %4, ptr %13, align 8
  store ptr %5, ptr %14, align 8
  store ptr %6, ptr %15, align 8
  store ptr %7, ptr %16, align 8
  %19 = load ptr, ptr %12, align 8
  store float 1.500000e+00, ptr %19, align 4
  %20 = load ptr, ptr %13, align 8
  store float 0x3FF3333340000000, ptr %20, align 4
  store i32 0, ptr %17, align 4
  br label %21

21:                                               ; preds = %52, %8
  %22 = load i32, ptr %17, align 4
  %23 = load i32, ptr %9, align 4
  %24 = icmp slt i32 %22, %23
  br i1 %24, label %25, label %55

25:                                               ; preds = %21
  store i32 0, ptr %18, align 4
  br label %26

26:                                               ; preds = %48, %25
  %27 = load i32, ptr %18, align 4
  %28 = load i32, ptr %10, align 4
  %29 = icmp slt i32 %27, %28
  br i1 %29, label %30, label %51

30:                                               ; preds = %26
  %31 = load i32, ptr %17, align 4
  %32 = load i32, ptr %18, align 4
  %33 = mul nsw i32 %31, %32
  %34 = add nsw i32 %33, 1
  %35 = load i32, ptr %9, align 4
  %36 = srem i32 %34, %35
  %37 = sitofp i32 %36 to float
  %38 = load i32, ptr %9, align 4
  %39 = sitofp i32 %38 to float
  %40 = fdiv float %37, %39
  %41 = load ptr, ptr %14, align 8
  %42 = load i32, ptr %17, align 4
  %43 = sext i32 %42 to i64
  %44 = getelementptr inbounds [25 x float], ptr %41, i64 %43
  %45 = load i32, ptr %18, align 4
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds [25 x float], ptr %44, i64 0, i64 %46
  store float %40, ptr %47, align 4
  br label %48

48:                                               ; preds = %30
  %49 = load i32, ptr %18, align 4
  %50 = add nsw i32 %49, 1
  store i32 %50, ptr %18, align 4
  br label %26, !llvm.loop !5

51:                                               ; preds = %26
  br label %52

52:                                               ; preds = %51
  %53 = load i32, ptr %17, align 4
  %54 = add nsw i32 %53, 1
  store i32 %54, ptr %17, align 4
  br label %21, !llvm.loop !7

55:                                               ; preds = %21
  store i32 0, ptr %17, align 4
  br label %56

56:                                               ; preds = %87, %55
  %57 = load i32, ptr %17, align 4
  %58 = load i32, ptr %9, align 4
  %59 = icmp slt i32 %57, %58
  br i1 %59, label %60, label %90

60:                                               ; preds = %56
  store i32 0, ptr %18, align 4
  br label %61

61:                                               ; preds = %83, %60
  %62 = load i32, ptr %18, align 4
  %63 = load i32, ptr %11, align 4
  %64 = icmp slt i32 %62, %63
  br i1 %64, label %65, label %86

65:                                               ; preds = %61
  %66 = load i32, ptr %17, align 4
  %67 = load i32, ptr %18, align 4
  %68 = add nsw i32 %67, 1
  %69 = mul nsw i32 %66, %68
  %70 = load i32, ptr %11, align 4
  %71 = srem i32 %69, %70
  %72 = sitofp i32 %71 to float
  %73 = load i32, ptr %11, align 4
  %74 = sitofp i32 %73 to float
  %75 = fdiv float %72, %74
  %76 = load ptr, ptr %15, align 8
  %77 = load i32, ptr %17, align 4
  %78 = sext i32 %77 to i64
  %79 = getelementptr inbounds [30 x float], ptr %76, i64 %78
  %80 = load i32, ptr %18, align 4
  %81 = sext i32 %80 to i64
  %82 = getelementptr inbounds [30 x float], ptr %79, i64 0, i64 %81
  store float %75, ptr %82, align 4
  br label %83

83:                                               ; preds = %65
  %84 = load i32, ptr %18, align 4
  %85 = add nsw i32 %84, 1
  store i32 %85, ptr %18, align 4
  br label %61, !llvm.loop !8

86:                                               ; preds = %61
  br label %87

87:                                               ; preds = %86
  %88 = load i32, ptr %17, align 4
  %89 = add nsw i32 %88, 1
  store i32 %89, ptr %17, align 4
  br label %56, !llvm.loop !9

90:                                               ; preds = %56
  store i32 0, ptr %17, align 4
  br label %91

91:                                               ; preds = %122, %90
  %92 = load i32, ptr %17, align 4
  %93 = load i32, ptr %11, align 4
  %94 = icmp slt i32 %92, %93
  br i1 %94, label %95, label %125

95:                                               ; preds = %91
  store i32 0, ptr %18, align 4
  br label %96

96:                                               ; preds = %118, %95
  %97 = load i32, ptr %18, align 4
  %98 = load i32, ptr %10, align 4
  %99 = icmp slt i32 %97, %98
  br i1 %99, label %100, label %121

100:                                              ; preds = %96
  %101 = load i32, ptr %17, align 4
  %102 = load i32, ptr %18, align 4
  %103 = add nsw i32 %102, 2
  %104 = mul nsw i32 %101, %103
  %105 = load i32, ptr %10, align 4
  %106 = srem i32 %104, %105
  %107 = sitofp i32 %106 to float
  %108 = load i32, ptr %10, align 4
  %109 = sitofp i32 %108 to float
  %110 = fdiv float %107, %109
  %111 = load ptr, ptr %16, align 8
  %112 = load i32, ptr %17, align 4
  %113 = sext i32 %112 to i64
  %114 = getelementptr inbounds [25 x float], ptr %111, i64 %113
  %115 = load i32, ptr %18, align 4
  %116 = sext i32 %115 to i64
  %117 = getelementptr inbounds [25 x float], ptr %114, i64 0, i64 %116
  store float %110, ptr %117, align 4
  br label %118

118:                                              ; preds = %100
  %119 = load i32, ptr %18, align 4
  %120 = add nsw i32 %119, 1
  store i32 %120, ptr %18, align 4
  br label %96, !llvm.loop !10

121:                                              ; preds = %96
  br label %122

122:                                              ; preds = %121
  %123 = load i32, ptr %17, align 4
  %124 = add nsw i32 %123, 1
  store i32 %124, ptr %17, align 4
  br label %91, !llvm.loop !11

125:                                              ; preds = %91
  ret void
}

; Function Attrs: noinline nounwind uwtable
define internal void @init_array_double(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr noundef %3, ptr noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 {
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
  store i32 %1, ptr %10, align 4
  store i32 %2, ptr %11, align 4
  store ptr %3, ptr %12, align 8
  store ptr %4, ptr %13, align 8
  store ptr %5, ptr %14, align 8
  store ptr %6, ptr %15, align 8
  store ptr %7, ptr %16, align 8
  %19 = load ptr, ptr %12, align 8
  store double 1.500000e+00, ptr %19, align 8
  %20 = load ptr, ptr %13, align 8
  store double 1.200000e+00, ptr %20, align 8
  store i32 0, ptr %17, align 4
  br label %21

21:                                               ; preds = %52, %8
  %22 = load i32, ptr %17, align 4
  %23 = load i32, ptr %9, align 4
  %24 = icmp slt i32 %22, %23
  br i1 %24, label %25, label %55

25:                                               ; preds = %21
  store i32 0, ptr %18, align 4
  br label %26

26:                                               ; preds = %48, %25
  %27 = load i32, ptr %18, align 4
  %28 = load i32, ptr %10, align 4
  %29 = icmp slt i32 %27, %28
  br i1 %29, label %30, label %51

30:                                               ; preds = %26
  %31 = load i32, ptr %17, align 4
  %32 = load i32, ptr %18, align 4
  %33 = mul nsw i32 %31, %32
  %34 = add nsw i32 %33, 1
  %35 = load i32, ptr %9, align 4
  %36 = srem i32 %34, %35
  %37 = sitofp i32 %36 to double
  %38 = load i32, ptr %9, align 4
  %39 = sitofp i32 %38 to double
  %40 = fdiv double %37, %39
  %41 = load ptr, ptr %14, align 8
  %42 = load i32, ptr %17, align 4
  %43 = sext i32 %42 to i64
  %44 = getelementptr inbounds [25 x double], ptr %41, i64 %43
  %45 = load i32, ptr %18, align 4
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds [25 x double], ptr %44, i64 0, i64 %46
  store double %40, ptr %47, align 8
  br label %48

48:                                               ; preds = %30
  %49 = load i32, ptr %18, align 4
  %50 = add nsw i32 %49, 1
  store i32 %50, ptr %18, align 4
  br label %26, !llvm.loop !12

51:                                               ; preds = %26
  br label %52

52:                                               ; preds = %51
  %53 = load i32, ptr %17, align 4
  %54 = add nsw i32 %53, 1
  store i32 %54, ptr %17, align 4
  br label %21, !llvm.loop !13

55:                                               ; preds = %21
  store i32 0, ptr %17, align 4
  br label %56

56:                                               ; preds = %87, %55
  %57 = load i32, ptr %17, align 4
  %58 = load i32, ptr %9, align 4
  %59 = icmp slt i32 %57, %58
  br i1 %59, label %60, label %90

60:                                               ; preds = %56
  store i32 0, ptr %18, align 4
  br label %61

61:                                               ; preds = %83, %60
  %62 = load i32, ptr %18, align 4
  %63 = load i32, ptr %11, align 4
  %64 = icmp slt i32 %62, %63
  br i1 %64, label %65, label %86

65:                                               ; preds = %61
  %66 = load i32, ptr %17, align 4
  %67 = load i32, ptr %18, align 4
  %68 = add nsw i32 %67, 1
  %69 = mul nsw i32 %66, %68
  %70 = load i32, ptr %11, align 4
  %71 = srem i32 %69, %70
  %72 = sitofp i32 %71 to double
  %73 = load i32, ptr %11, align 4
  %74 = sitofp i32 %73 to double
  %75 = fdiv double %72, %74
  %76 = load ptr, ptr %15, align 8
  %77 = load i32, ptr %17, align 4
  %78 = sext i32 %77 to i64
  %79 = getelementptr inbounds [30 x double], ptr %76, i64 %78
  %80 = load i32, ptr %18, align 4
  %81 = sext i32 %80 to i64
  %82 = getelementptr inbounds [30 x double], ptr %79, i64 0, i64 %81
  store double %75, ptr %82, align 8
  br label %83

83:                                               ; preds = %65
  %84 = load i32, ptr %18, align 4
  %85 = add nsw i32 %84, 1
  store i32 %85, ptr %18, align 4
  br label %61, !llvm.loop !14

86:                                               ; preds = %61
  br label %87

87:                                               ; preds = %86
  %88 = load i32, ptr %17, align 4
  %89 = add nsw i32 %88, 1
  store i32 %89, ptr %17, align 4
  br label %56, !llvm.loop !15

90:                                               ; preds = %56
  store i32 0, ptr %17, align 4
  br label %91

91:                                               ; preds = %122, %90
  %92 = load i32, ptr %17, align 4
  %93 = load i32, ptr %11, align 4
  %94 = icmp slt i32 %92, %93
  br i1 %94, label %95, label %125

95:                                               ; preds = %91
  store i32 0, ptr %18, align 4
  br label %96

96:                                               ; preds = %118, %95
  %97 = load i32, ptr %18, align 4
  %98 = load i32, ptr %10, align 4
  %99 = icmp slt i32 %97, %98
  br i1 %99, label %100, label %121

100:                                              ; preds = %96
  %101 = load i32, ptr %17, align 4
  %102 = load i32, ptr %18, align 4
  %103 = add nsw i32 %102, 2
  %104 = mul nsw i32 %101, %103
  %105 = load i32, ptr %10, align 4
  %106 = srem i32 %104, %105
  %107 = sitofp i32 %106 to double
  %108 = load i32, ptr %10, align 4
  %109 = sitofp i32 %108 to double
  %110 = fdiv double %107, %109
  %111 = load ptr, ptr %16, align 8
  %112 = load i32, ptr %17, align 4
  %113 = sext i32 %112 to i64
  %114 = getelementptr inbounds [25 x double], ptr %111, i64 %113
  %115 = load i32, ptr %18, align 4
  %116 = sext i32 %115 to i64
  %117 = getelementptr inbounds [25 x double], ptr %114, i64 0, i64 %116
  store double %110, ptr %117, align 8
  br label %118

118:                                              ; preds = %100
  %119 = load i32, ptr %18, align 4
  %120 = add nsw i32 %119, 1
  store i32 %120, ptr %18, align 4
  br label %96, !llvm.loop !16

121:                                              ; preds = %96
  br label %122

122:                                              ; preds = %121
  %123 = load i32, ptr %17, align 4
  %124 = add nsw i32 %123, 1
  store i32 %124, ptr %17, align 4
  br label %91, !llvm.loop !17

125:                                              ; preds = %91
  ret void
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemm(i32 noundef %0, i32 noundef %1, i32 noundef %2, float noundef %3, float noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 {
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
  store i32 %1, ptr %10, align 4
  store i32 %2, ptr %11, align 4
  store float %3, ptr %12, align 4
  store float %4, ptr %13, align 4
  store ptr %5, ptr %14, align 8
  store ptr %6, ptr %15, align 8
  store ptr %7, ptr %16, align 8
  store i32 0, ptr %17, align 4
  br label %20

20:                                               ; preds = %89, %8
  %21 = load i32, ptr %17, align 4
  %22 = load i32, ptr %9, align 4
  %23 = icmp slt i32 %21, %22
  br i1 %23, label %24, label %92

24:                                               ; preds = %20
  store i32 0, ptr %18, align 4
  br label %25

25:                                               ; preds = %40, %24
  %26 = load i32, ptr %18, align 4
  %27 = load i32, ptr %10, align 4
  %28 = icmp slt i32 %26, %27
  br i1 %28, label %29, label %43

29:                                               ; preds = %25
  %30 = load float, ptr %13, align 4
  %31 = load ptr, ptr %14, align 8
  %32 = load i32, ptr %17, align 4
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds [25 x float], ptr %31, i64 %33
  %35 = load i32, ptr %18, align 4
  %36 = sext i32 %35 to i64
  %37 = getelementptr inbounds [25 x float], ptr %34, i64 0, i64 %36
  %38 = load float, ptr %37, align 4
  %39 = fmul float %38, %30
  store float %39, ptr %37, align 4
  br label %40

40:                                               ; preds = %29
  %41 = load i32, ptr %18, align 4
  %42 = add nsw i32 %41, 1
  store i32 %42, ptr %18, align 4
  br label %25, !llvm.loop !18

43:                                               ; preds = %25
  store i32 0, ptr %19, align 4
  br label %44

44:                                               ; preds = %85, %43
  %45 = load i32, ptr %19, align 4
  %46 = load i32, ptr %11, align 4
  %47 = icmp slt i32 %45, %46
  br i1 %47, label %48, label %88

48:                                               ; preds = %44
  store i32 0, ptr %18, align 4
  br label %49

49:                                               ; preds = %81, %48
  %50 = load i32, ptr %18, align 4
  %51 = load i32, ptr %10, align 4
  %52 = icmp slt i32 %50, %51
  br i1 %52, label %53, label %84

53:                                               ; preds = %49
  %54 = load float, ptr %12, align 4
  %55 = load ptr, ptr %15, align 8
  %56 = load i32, ptr %17, align 4
  %57 = sext i32 %56 to i64
  %58 = getelementptr inbounds [30 x float], ptr %55, i64 %57
  %59 = load i32, ptr %19, align 4
  %60 = sext i32 %59 to i64
  %61 = getelementptr inbounds [30 x float], ptr %58, i64 0, i64 %60
  %62 = load float, ptr %61, align 4
  %63 = fmul float %54, %62
  %64 = load ptr, ptr %16, align 8
  %65 = load i32, ptr %19, align 4
  %66 = sext i32 %65 to i64
  %67 = getelementptr inbounds [25 x float], ptr %64, i64 %66
  %68 = load i32, ptr %18, align 4
  %69 = sext i32 %68 to i64
  %70 = getelementptr inbounds [25 x float], ptr %67, i64 0, i64 %69
  %71 = load float, ptr %70, align 4
  %72 = load ptr, ptr %14, align 8
  %73 = load i32, ptr %17, align 4
  %74 = sext i32 %73 to i64
  %75 = getelementptr inbounds [25 x float], ptr %72, i64 %74
  %76 = load i32, ptr %18, align 4
  %77 = sext i32 %76 to i64
  %78 = getelementptr inbounds [25 x float], ptr %75, i64 0, i64 %77
  %79 = load float, ptr %78, align 4
  %80 = call float @llvm.fmuladd.f32(float %63, float %71, float %79)
  store float %80, ptr %78, align 4
  br label %81

81:                                               ; preds = %53
  %82 = load i32, ptr %18, align 4
  %83 = add nsw i32 %82, 1
  store i32 %83, ptr %18, align 4
  br label %49, !llvm.loop !19

84:                                               ; preds = %49
  br label %85

85:                                               ; preds = %84
  %86 = load i32, ptr %19, align 4
  %87 = add nsw i32 %86, 1
  store i32 %87, ptr %19, align 4
  br label %44, !llvm.loop !20

88:                                               ; preds = %44
  br label %89

89:                                               ; preds = %88
  %90 = load i32, ptr %17, align 4
  %91 = add nsw i32 %90, 1
  store i32 %91, ptr %17, align 4
  br label %20, !llvm.loop !21

92:                                               ; preds = %20
  ret void
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemm_double(i32 noundef %0, i32 noundef %1, i32 noundef %2, double noundef %3, double noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7) #0 {
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
  store i32 %1, ptr %10, align 4
  store i32 %2, ptr %11, align 4
  store double %3, ptr %12, align 8
  store double %4, ptr %13, align 8
  store ptr %5, ptr %14, align 8
  store ptr %6, ptr %15, align 8
  store ptr %7, ptr %16, align 8
  store i32 0, ptr %17, align 4
  br label %20

20:                                               ; preds = %89, %8
  %21 = load i32, ptr %17, align 4
  %22 = load i32, ptr %9, align 4
  %23 = icmp slt i32 %21, %22
  br i1 %23, label %24, label %92

24:                                               ; preds = %20
  store i32 0, ptr %18, align 4
  br label %25

25:                                               ; preds = %40, %24
  %26 = load i32, ptr %18, align 4
  %27 = load i32, ptr %10, align 4
  %28 = icmp slt i32 %26, %27
  br i1 %28, label %29, label %43

29:                                               ; preds = %25
  %30 = load double, ptr %13, align 8
  %31 = load ptr, ptr %14, align 8
  %32 = load i32, ptr %17, align 4
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds [25 x double], ptr %31, i64 %33
  %35 = load i32, ptr %18, align 4
  %36 = sext i32 %35 to i64
  %37 = getelementptr inbounds [25 x double], ptr %34, i64 0, i64 %36
  %38 = load double, ptr %37, align 8
  %39 = fmul double %38, %30
  store double %39, ptr %37, align 8
  br label %40

40:                                               ; preds = %29
  %41 = load i32, ptr %18, align 4
  %42 = add nsw i32 %41, 1
  store i32 %42, ptr %18, align 4
  br label %25, !llvm.loop !22

43:                                               ; preds = %25
  store i32 0, ptr %19, align 4
  br label %44

44:                                               ; preds = %85, %43
  %45 = load i32, ptr %19, align 4
  %46 = load i32, ptr %11, align 4
  %47 = icmp slt i32 %45, %46
  br i1 %47, label %48, label %88

48:                                               ; preds = %44
  store i32 0, ptr %18, align 4
  br label %49

49:                                               ; preds = %81, %48
  %50 = load i32, ptr %18, align 4
  %51 = load i32, ptr %10, align 4
  %52 = icmp slt i32 %50, %51
  br i1 %52, label %53, label %84

53:                                               ; preds = %49
  %54 = load double, ptr %12, align 8
  %55 = load ptr, ptr %15, align 8
  %56 = load i32, ptr %17, align 4
  %57 = sext i32 %56 to i64
  %58 = getelementptr inbounds [30 x double], ptr %55, i64 %57
  %59 = load i32, ptr %19, align 4
  %60 = sext i32 %59 to i64
  %61 = getelementptr inbounds [30 x double], ptr %58, i64 0, i64 %60
  %62 = load double, ptr %61, align 8
  %63 = fmul double %54, %62
  %64 = load ptr, ptr %16, align 8
  %65 = load i32, ptr %19, align 4
  %66 = sext i32 %65 to i64
  %67 = getelementptr inbounds [25 x double], ptr %64, i64 %66
  %68 = load i32, ptr %18, align 4
  %69 = sext i32 %68 to i64
  %70 = getelementptr inbounds [25 x double], ptr %67, i64 0, i64 %69
  %71 = load double, ptr %70, align 8
  %72 = load ptr, ptr %14, align 8
  %73 = load i32, ptr %17, align 4
  %74 = sext i32 %73 to i64
  %75 = getelementptr inbounds [25 x double], ptr %72, i64 %74
  %76 = load i32, ptr %18, align 4
  %77 = sext i32 %76 to i64
  %78 = getelementptr inbounds [25 x double], ptr %75, i64 0, i64 %77
  %79 = load double, ptr %78, align 8
  %80 = call double @llvm.fmuladd.f64(double %63, double %71, double %79)
  store double %80, ptr %78, align 8
  br label %81

81:                                               ; preds = %53
  %82 = load i32, ptr %18, align 4
  %83 = add nsw i32 %82, 1
  store i32 %83, ptr %18, align 4
  br label %49, !llvm.loop !23

84:                                               ; preds = %49
  br label %85

85:                                               ; preds = %84
  %86 = load i32, ptr %19, align 4
  %87 = add nsw i32 %86, 1
  store i32 %87, ptr %19, align 4
  br label %44, !llvm.loop !24

88:                                               ; preds = %44
  br label %89

89:                                               ; preds = %88
  %90 = load i32, ptr %17, align 4
  %91 = add nsw i32 %90, 1
  store i32 %91, ptr %17, align 4
  br label %20, !llvm.loop !25

92:                                               ; preds = %20
  ret void
}

; Function Attrs: noinline nounwind uwtable
define internal void @print_array(i32 noundef %0, i32 noundef %1, ptr noundef %2, ptr noundef %3) #0 {
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
  store i32 %1, ptr %6, align 4
  store ptr %2, ptr %7, align 8
  store ptr %3, ptr %8, align 8
  store float 0.000000e+00, ptr %11, align 4
  store float 0.000000e+00, ptr %12, align 4
  store float 0.000000e+00, ptr %13, align 4
  store double 0.000000e+00, ptr %14, align 8
  store double 0.000000e+00, ptr %15, align 8
  store double 0.000000e+00, ptr %16, align 8
  %22 = load ptr, ptr @stderr, align 8
  %23 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %22, ptr noundef @.str) #4
  %24 = load ptr, ptr @stderr, align 8
  %25 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %24, ptr noundef @.str.1, ptr noundef @.str.2) #4
  store i32 0, ptr %9, align 4
  br label %26

26:                                               ; preds = %80, %4
  %27 = load i32, ptr %9, align 4
  %28 = load i32, ptr %5, align 4
  %29 = icmp slt i32 %27, %28
  br i1 %29, label %30, label %83

30:                                               ; preds = %26
  store i32 0, ptr %10, align 4
  br label %31

31:                                               ; preds = %76, %30
  %32 = load i32, ptr %10, align 4
  %33 = load i32, ptr %6, align 4
  %34 = icmp slt i32 %32, %33
  br i1 %34, label %35, label %79

35:                                               ; preds = %31
  %36 = load ptr, ptr %7, align 8
  %37 = load i32, ptr %9, align 4
  %38 = sext i32 %37 to i64
  %39 = getelementptr inbounds [25 x float], ptr %36, i64 %38
  %40 = load i32, ptr %10, align 4
  %41 = sext i32 %40 to i64
  %42 = getelementptr inbounds [25 x float], ptr %39, i64 0, i64 %41
  %43 = load float, ptr %42, align 4
  store float %43, ptr %17, align 4
  %44 = load ptr, ptr %8, align 8
  %45 = load i32, ptr %9, align 4
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds [25 x double], ptr %44, i64 %46
  %48 = load i32, ptr %10, align 4
  %49 = sext i32 %48 to i64
  %50 = getelementptr inbounds [25 x double], ptr %47, i64 0, i64 %49
  %51 = load double, ptr %50, align 8
  store double %51, ptr %18, align 8
  %52 = load float, ptr %17, align 4
  %53 = fcmp olt float %52, 0.000000e+00
  br i1 %53, label %54, label %57

54:                                               ; preds = %35
  %55 = load float, ptr %17, align 4
  %56 = fneg float %55
  store float %56, ptr %17, align 4
  br label %57

57:                                               ; preds = %54, %35
  %58 = load double, ptr %18, align 8
  %59 = fcmp olt double %58, 0.000000e+00
  br i1 %59, label %60, label %63

60:                                               ; preds = %57
  %61 = load double, ptr %18, align 8
  %62 = fneg double %61
  store double %62, ptr %18, align 8
  br label %63

63:                                               ; preds = %60, %57
  %64 = load float, ptr %17, align 4
  %65 = load float, ptr %11, align 4
  %66 = fcmp ogt float %64, %65
  br i1 %66, label %67, label %69

67:                                               ; preds = %63
  %68 = load float, ptr %17, align 4
  store float %68, ptr %11, align 4
  br label %69

69:                                               ; preds = %67, %63
  %70 = load double, ptr %18, align 8
  %71 = load double, ptr %14, align 8
  %72 = fcmp ogt double %70, %71
  br i1 %72, label %73, label %75

73:                                               ; preds = %69
  %74 = load double, ptr %18, align 8
  store double %74, ptr %14, align 8
  br label %75

75:                                               ; preds = %73, %69
  br label %76

76:                                               ; preds = %75
  %77 = load i32, ptr %10, align 4
  %78 = add nsw i32 %77, 1
  store i32 %78, ptr %10, align 4
  br label %31, !llvm.loop !26

79:                                               ; preds = %31
  br label %80

80:                                               ; preds = %79
  %81 = load i32, ptr %9, align 4
  %82 = add nsw i32 %81, 1
  store i32 %82, ptr %9, align 4
  br label %26, !llvm.loop !27

83:                                               ; preds = %26
  %84 = load float, ptr %11, align 4
  %85 = fcmp une float %84, 0.000000e+00
  br i1 %85, label %86, label %121

86:                                               ; preds = %83
  store i32 0, ptr %9, align 4
  br label %87

87:                                               ; preds = %115, %86
  %88 = load i32, ptr %9, align 4
  %89 = load i32, ptr %5, align 4
  %90 = icmp slt i32 %88, %89
  br i1 %90, label %91, label %118

91:                                               ; preds = %87
  store i32 0, ptr %10, align 4
  br label %92

92:                                               ; preds = %111, %91
  %93 = load i32, ptr %10, align 4
  %94 = load i32, ptr %6, align 4
  %95 = icmp slt i32 %93, %94
  br i1 %95, label %96, label %114

96:                                               ; preds = %92
  %97 = load ptr, ptr %7, align 8
  %98 = load i32, ptr %9, align 4
  %99 = sext i32 %98 to i64
  %100 = getelementptr inbounds [25 x float], ptr %97, i64 %99
  %101 = load i32, ptr %10, align 4
  %102 = sext i32 %101 to i64
  %103 = getelementptr inbounds [25 x float], ptr %100, i64 0, i64 %102
  %104 = load float, ptr %103, align 4
  %105 = load float, ptr %11, align 4
  %106 = fdiv float %104, %105
  store float %106, ptr %19, align 4
  %107 = load float, ptr %19, align 4
  %108 = load float, ptr %19, align 4
  %109 = load float, ptr %12, align 4
  %110 = call float @llvm.fmuladd.f32(float %107, float %108, float %109)
  store float %110, ptr %12, align 4
  br label %111

111:                                              ; preds = %96
  %112 = load i32, ptr %10, align 4
  %113 = add nsw i32 %112, 1
  store i32 %113, ptr %10, align 4
  br label %92, !llvm.loop !28

114:                                              ; preds = %92
  br label %115

115:                                              ; preds = %114
  %116 = load i32, ptr %9, align 4
  %117 = add nsw i32 %116, 1
  store i32 %117, ptr %9, align 4
  br label %87, !llvm.loop !29

118:                                              ; preds = %87
  %119 = load float, ptr %12, align 4
  %120 = call float @sqrtf(float noundef %119) #4
  store float %120, ptr %13, align 4
  br label %121

121:                                              ; preds = %118, %83
  %122 = load double, ptr %14, align 8
  %123 = fcmp une double %122, 0.000000e+00
  br i1 %123, label %124, label %159

124:                                              ; preds = %121
  store i32 0, ptr %9, align 4
  br label %125

125:                                              ; preds = %153, %124
  %126 = load i32, ptr %9, align 4
  %127 = load i32, ptr %5, align 4
  %128 = icmp slt i32 %126, %127
  br i1 %128, label %129, label %156

129:                                              ; preds = %125
  store i32 0, ptr %10, align 4
  br label %130

130:                                              ; preds = %149, %129
  %131 = load i32, ptr %10, align 4
  %132 = load i32, ptr %6, align 4
  %133 = icmp slt i32 %131, %132
  br i1 %133, label %134, label %152

134:                                              ; preds = %130
  %135 = load ptr, ptr %8, align 8
  %136 = load i32, ptr %9, align 4
  %137 = sext i32 %136 to i64
  %138 = getelementptr inbounds [25 x double], ptr %135, i64 %137
  %139 = load i32, ptr %10, align 4
  %140 = sext i32 %139 to i64
  %141 = getelementptr inbounds [25 x double], ptr %138, i64 0, i64 %140
  %142 = load double, ptr %141, align 8
  %143 = load double, ptr %14, align 8
  %144 = fdiv double %142, %143
  store double %144, ptr %20, align 8
  %145 = load double, ptr %20, align 8
  %146 = load double, ptr %20, align 8
  %147 = load double, ptr %15, align 8
  %148 = call double @llvm.fmuladd.f64(double %145, double %146, double %147)
  store double %148, ptr %15, align 8
  br label %149

149:                                              ; preds = %134
  %150 = load i32, ptr %10, align 4
  %151 = add nsw i32 %150, 1
  store i32 %151, ptr %10, align 4
  br label %130, !llvm.loop !30

152:                                              ; preds = %130
  br label %153

153:                                              ; preds = %152
  %154 = load i32, ptr %9, align 4
  %155 = add nsw i32 %154, 1
  store i32 %155, ptr %9, align 4
  br label %125, !llvm.loop !31

156:                                              ; preds = %125
  %157 = load double, ptr %15, align 8
  %158 = call double @sqrt(double noundef %157) #4
  store double %158, ptr %16, align 8
  br label %159

159:                                              ; preds = %156, %121
  %160 = load ptr, ptr @stderr, align 8
  %161 = load float, ptr %11, align 4
  %162 = fpext float %161 to double
  %163 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %160, ptr noundef @.str.3, double noundef %162) #4
  %164 = load ptr, ptr @stderr, align 8
  %165 = load float, ptr %13, align 4
  %166 = fpext float %165 to double
  %167 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %164, ptr noundef @.str.4, double noundef %166) #4
  %168 = load ptr, ptr @stderr, align 8
  %169 = load double, ptr %14, align 8
  %170 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %168, ptr noundef @.str.5, double noundef %169) #4
  %171 = load ptr, ptr @stderr, align 8
  %172 = load double, ptr %16, align 8
  %173 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %171, ptr noundef @.str.6, double noundef %172) #4
  %174 = load double, ptr %16, align 8
  %175 = load float, ptr %13, align 4
  %176 = fpext float %175 to double
  %177 = fsub double %174, %176
  store double %177, ptr %21, align 8
  %178 = load ptr, ptr @stderr, align 8
  %179 = load double, ptr %21, align 8
  %180 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %178, ptr noundef @.str.7, double noundef %179) #4
  %181 = load ptr, ptr @stderr, align 8
  %182 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %181, ptr noundef @.str.8, ptr noundef @.str.9) #4
  %183 = load ptr, ptr @stderr, align 8
  %184 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %183, ptr noundef @.str.10) #4
  ret void
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

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 7, !"Dwarf Version", i32 4}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
!7 = distinct !{!7, !6}
!8 = distinct !{!8, !6}
!9 = distinct !{!9, !6}
!10 = distinct !{!10, !6}
!11 = distinct !{!11, !6}
!12 = distinct !{!12, !6}
!13 = distinct !{!13, !6}
!14 = distinct !{!14, !6}
!15 = distinct !{!15, !6}
!16 = distinct !{!16, !6}
!17 = distinct !{!17, !6}
!18 = distinct !{!18, !6}
!19 = distinct !{!19, !6}
!20 = distinct !{!20, !6}
!21 = distinct !{!21, !6}
!22 = distinct !{!22, !6}
!23 = distinct !{!23, !6}
!24 = distinct !{!24, !6}
!25 = distinct !{!25, !6}
!26 = distinct !{!26, !6}
!27 = distinct !{!27, !6}
!28 = distinct !{!28, !6}
!29 = distinct !{!29, !6}
!30 = distinct !{!30, !6}
!31 = distinct !{!31, !6}

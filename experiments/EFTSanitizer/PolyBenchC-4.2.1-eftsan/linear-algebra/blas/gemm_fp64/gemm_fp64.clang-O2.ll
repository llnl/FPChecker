; ModuleID = 'gemm.c'
source_filename = "gemm.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@stderr = external dso_local local_unnamed_addr global ptr, align 8
@.str = private unnamed_addr constant [23 x i8] c"==BEGIN DUMP_ARRAYS==\0A\00", align 1
@.str.1 = private unnamed_addr constant [15 x i8] c"begin dump: %s\00", align 1
@.str.2 = private unnamed_addr constant [3 x i8] c"C\0A\00", align 1
@.str.3 = private unnamed_addr constant [23 x i8] c"Max value in C: %.17e\0A\00", align 1
@.str.4 = private unnamed_addr constant [18 x i8] c"Norm of C: %.17e\0A\00", align 1
@.str.5 = private unnamed_addr constant [34 x i8] c"Max value in long_double: %.21Le\0A\00", align 1
@.str.6 = private unnamed_addr constant [29 x i8] c"Norm of long_double: %.21Le\0A\00", align 1
@.str.7 = private unnamed_addr constant [19 x i8] c"Norm error: %.17e\0A\00", align 1
@.str.8 = private unnamed_addr constant [17 x i8] c"\0Aend   dump: %s\0A\00", align 1
@.str.9 = private unnamed_addr constant [2 x i8] c"C\00", align 1
@.str.10 = private unnamed_addr constant [23 x i8] c"==END   DUMP_ARRAYS==\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local noundef i32 @main(i32 noundef %0, ptr noundef readnone captures(none) %1) local_unnamed_addr #0 {
  %3 = tail call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 8) #8
  %4 = tail call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 8) #8
  %5 = tail call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 8) #8
  %6 = tail call ptr @polybench_alloc_data(i64 noundef 500, i32 noundef 16) #8
  %7 = tail call ptr @polybench_alloc_data(i64 noundef 600, i32 noundef 16) #8
  %8 = tail call ptr @polybench_alloc_data(i64 noundef 750, i32 noundef 16) #8
  br label %9

9:                                                ; preds = %31, %2
  %10 = phi i64 [ 0, %2 ], [ %32, %31 ]
  br label %11

11:                                               ; preds = %21, %9
  %12 = phi i64 [ 0, %9 ], [ %30, %21 ]
  %13 = mul nuw nsw i64 %12, %10
  %14 = trunc i64 %13 to i32
  %15 = or disjoint i32 %14, 1
  %16 = urem i32 %15, 20
  %17 = uitofp nneg i32 %16 to double
  %18 = fdiv double %17, 2.000000e+01
  %19 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %10, i64 %12
  store double %18, ptr %19, align 8, !tbaa !4
  %20 = icmp eq i64 %12, 24
  br i1 %20, label %31, label %21, !llvm.loop !8

21:                                               ; preds = %11
  %22 = or disjoint i64 %12, 1
  %23 = mul nuw nsw i64 %22, %10
  %24 = trunc i64 %23 to i32
  %25 = add nuw nsw i32 %24, 1
  %26 = urem i32 %25, 20
  %27 = uitofp nneg i32 %26 to double
  %28 = fdiv double %27, 2.000000e+01
  %29 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %10, i64 %22
  store double %28, ptr %29, align 8, !tbaa !4
  %30 = add nuw nsw i64 %12, 2
  br label %11

31:                                               ; preds = %11
  %32 = add nuw nsw i64 %10, 1
  %33 = icmp eq i64 %32, 20
  br i1 %33, label %34, label %9, !llvm.loop !10

34:                                               ; preds = %31, %53
  %35 = phi i64 [ %54, %53 ], [ 0, %31 ]
  br label %36

36:                                               ; preds = %36, %34
  %37 = phi i64 [ 0, %34 ], [ %45, %36 ]
  %38 = or disjoint i64 %37, 1
  %39 = mul nuw nsw i64 %38, %35
  %40 = trunc nuw nsw i64 %39 to i32
  %41 = urem i32 %40, 30
  %42 = uitofp nneg i32 %41 to double
  %43 = fdiv double %42, 3.000000e+01
  %44 = getelementptr inbounds nuw [30 x double], ptr %4, i64 %35, i64 %37
  store double %43, ptr %44, align 8, !tbaa !4
  %45 = add nuw nsw i64 %37, 2
  %46 = mul nuw nsw i64 %45, %35
  %47 = trunc nuw nsw i64 %46 to i32
  %48 = urem i32 %47, 30
  %49 = uitofp nneg i32 %48 to double
  %50 = fdiv double %49, 3.000000e+01
  %51 = getelementptr inbounds nuw [30 x double], ptr %4, i64 %35, i64 %38
  store double %50, ptr %51, align 8, !tbaa !4
  %52 = icmp eq i64 %45, 30
  br i1 %52, label %53, label %36, !llvm.loop !11

53:                                               ; preds = %36
  %54 = add nuw nsw i64 %35, 1
  %55 = icmp eq i64 %54, 20
  br i1 %55, label %56, label %34, !llvm.loop !12

56:                                               ; preds = %53, %78
  %57 = phi i64 [ %79, %78 ], [ 0, %53 ]
  br label %58

58:                                               ; preds = %68, %56
  %59 = phi i64 [ 0, %56 ], [ %77, %68 ]
  %60 = add nuw nsw i64 %59, 2
  %61 = mul nuw nsw i64 %60, %57
  %62 = trunc nuw nsw i64 %61 to i32
  %63 = urem i32 %62, 25
  %64 = uitofp nneg i32 %63 to double
  %65 = fdiv double %64, 2.500000e+01
  %66 = getelementptr inbounds nuw [25 x double], ptr %5, i64 %57, i64 %59
  store double %65, ptr %66, align 8, !tbaa !4
  %67 = icmp eq i64 %59, 24
  br i1 %67, label %78, label %68, !llvm.loop !13

68:                                               ; preds = %58
  %69 = or disjoint i64 %59, 1
  %70 = add nuw nsw i64 %59, 3
  %71 = mul nuw nsw i64 %70, %57
  %72 = trunc nuw nsw i64 %71 to i32
  %73 = urem i32 %72, 25
  %74 = uitofp nneg i32 %73 to double
  %75 = fdiv double %74, 2.500000e+01
  %76 = getelementptr inbounds nuw [25 x double], ptr %5, i64 %57, i64 %69
  store double %75, ptr %76, align 8, !tbaa !4
  %77 = add nuw nsw i64 %59, 2
  br label %58

78:                                               ; preds = %58
  %79 = add nuw nsw i64 %57, 1
  %80 = icmp eq i64 %79, 30
  br i1 %80, label %81, label %56, !llvm.loop !14

81:                                               ; preds = %78, %103
  %82 = phi i64 [ %104, %103 ], [ 0, %78 ]
  br label %83

83:                                               ; preds = %93, %81
  %84 = phi i64 [ 0, %81 ], [ %102, %93 ]
  %85 = mul nuw nsw i64 %84, %82
  %86 = trunc i64 %85 to i32
  %87 = or disjoint i32 %86, 1
  %88 = urem i32 %87, 20
  %89 = uitofp nneg i32 %88 to x86_fp80
  %90 = fdiv x86_fp80 %89, 0xK4003A000000000000000
  %91 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %82, i64 %84
  store x86_fp80 %90, ptr %91, align 16, !tbaa !15
  %92 = icmp eq i64 %84, 24
  br i1 %92, label %103, label %93, !llvm.loop !17

93:                                               ; preds = %83
  %94 = or disjoint i64 %84, 1
  %95 = mul nuw nsw i64 %94, %82
  %96 = trunc i64 %95 to i32
  %97 = add nuw nsw i32 %96, 1
  %98 = urem i32 %97, 20
  %99 = uitofp nneg i32 %98 to x86_fp80
  %100 = fdiv x86_fp80 %99, 0xK4003A000000000000000
  %101 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %82, i64 %94
  store x86_fp80 %100, ptr %101, align 16, !tbaa !15
  %102 = add nuw nsw i64 %84, 2
  br label %83

103:                                              ; preds = %83
  %104 = add nuw nsw i64 %82, 1
  %105 = icmp eq i64 %104, 20
  br i1 %105, label %106, label %81, !llvm.loop !18

106:                                              ; preds = %103, %125
  %107 = phi i64 [ %126, %125 ], [ 0, %103 ]
  br label %108

108:                                              ; preds = %108, %106
  %109 = phi i64 [ 0, %106 ], [ %117, %108 ]
  %110 = or disjoint i64 %109, 1
  %111 = mul nuw nsw i64 %110, %107
  %112 = trunc nuw nsw i64 %111 to i32
  %113 = urem i32 %112, 30
  %114 = uitofp nneg i32 %113 to x86_fp80
  %115 = fdiv x86_fp80 %114, 0xK4003F000000000000000
  %116 = getelementptr inbounds nuw [30 x x86_fp80], ptr %7, i64 %107, i64 %109
  store x86_fp80 %115, ptr %116, align 16, !tbaa !15
  %117 = add nuw nsw i64 %109, 2
  %118 = mul nuw nsw i64 %117, %107
  %119 = trunc nuw nsw i64 %118 to i32
  %120 = urem i32 %119, 30
  %121 = uitofp nneg i32 %120 to x86_fp80
  %122 = fdiv x86_fp80 %121, 0xK4003F000000000000000
  %123 = getelementptr inbounds nuw [30 x x86_fp80], ptr %7, i64 %107, i64 %110
  store x86_fp80 %122, ptr %123, align 16, !tbaa !15
  %124 = icmp eq i64 %117, 30
  br i1 %124, label %125, label %108, !llvm.loop !19

125:                                              ; preds = %108
  %126 = add nuw nsw i64 %107, 1
  %127 = icmp eq i64 %126, 20
  br i1 %127, label %128, label %106, !llvm.loop !20

128:                                              ; preds = %125, %150
  %129 = phi i64 [ %151, %150 ], [ 0, %125 ]
  br label %130

130:                                              ; preds = %140, %128
  %131 = phi i64 [ 0, %128 ], [ %149, %140 ]
  %132 = add nuw nsw i64 %131, 2
  %133 = mul nuw nsw i64 %132, %129
  %134 = trunc nuw nsw i64 %133 to i32
  %135 = urem i32 %134, 25
  %136 = uitofp nneg i32 %135 to x86_fp80
  %137 = fdiv x86_fp80 %136, 0xK4003C800000000000000
  %138 = getelementptr inbounds nuw [25 x x86_fp80], ptr %8, i64 %129, i64 %131
  store x86_fp80 %137, ptr %138, align 16, !tbaa !15
  %139 = icmp eq i64 %131, 24
  br i1 %139, label %150, label %140, !llvm.loop !21

140:                                              ; preds = %130
  %141 = or disjoint i64 %131, 1
  %142 = add nuw nsw i64 %131, 3
  %143 = mul nuw nsw i64 %142, %129
  %144 = trunc nuw nsw i64 %143 to i32
  %145 = urem i32 %144, 25
  %146 = uitofp nneg i32 %145 to x86_fp80
  %147 = fdiv x86_fp80 %146, 0xK4003C800000000000000
  %148 = getelementptr inbounds nuw [25 x x86_fp80], ptr %8, i64 %129, i64 %141
  store x86_fp80 %147, ptr %148, align 16, !tbaa !15
  %149 = add nuw nsw i64 %131, 2
  br label %130

150:                                              ; preds = %130
  %151 = add nuw nsw i64 %129, 1
  %152 = icmp eq i64 %151, 30
  br i1 %152, label %153, label %128, !llvm.loop !22

153:                                              ; preds = %150, %201
  %154 = phi i64 [ %202, %201 ], [ 0, %150 ]
  br label %155

155:                                              ; preds = %161, %153
  %156 = phi i64 [ 0, %153 ], [ %174, %161 ]
  %157 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %154, i64 %156
  %158 = load double, ptr %157, align 8, !tbaa !4
  %159 = fmul double %158, 1.200000e+00
  store double %159, ptr %157, align 8, !tbaa !4
  %160 = icmp eq i64 %156, 24
  br i1 %160, label %175, label %161, !llvm.loop !23

161:                                              ; preds = %155
  %162 = or disjoint i64 %156, 1
  %163 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %154, i64 %162
  %164 = load double, ptr %163, align 8, !tbaa !4
  %165 = fmul double %164, 1.200000e+00
  store double %165, ptr %163, align 8, !tbaa !4
  %166 = or disjoint i64 %156, 2
  %167 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %154, i64 %166
  %168 = load double, ptr %167, align 8, !tbaa !4
  %169 = fmul double %168, 1.200000e+00
  store double %169, ptr %167, align 8, !tbaa !4
  %170 = or disjoint i64 %156, 3
  %171 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %154, i64 %170
  %172 = load double, ptr %171, align 8, !tbaa !4
  %173 = fmul double %172, 1.200000e+00
  store double %173, ptr %171, align 8, !tbaa !4
  %174 = add nuw nsw i64 %156, 4
  br label %155

175:                                              ; preds = %155, %198
  %176 = phi i64 [ %199, %198 ], [ 0, %155 ]
  %177 = getelementptr inbounds nuw [30 x double], ptr %4, i64 %154, i64 %176
  br label %178

178:                                              ; preds = %188, %175
  %179 = phi i64 [ 0, %175 ], [ %197, %188 ]
  %180 = load double, ptr %177, align 8, !tbaa !4
  %181 = fmul double %180, 1.500000e+00
  %182 = getelementptr inbounds nuw [25 x double], ptr %5, i64 %176, i64 %179
  %183 = load double, ptr %182, align 8, !tbaa !4
  %184 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %154, i64 %179
  %185 = load double, ptr %184, align 8, !tbaa !4
  %186 = tail call double @llvm.fmuladd.f64(double %181, double %183, double %185)
  store double %186, ptr %184, align 8, !tbaa !4
  %187 = icmp eq i64 %179, 24
  br i1 %187, label %198, label %188, !llvm.loop !24

188:                                              ; preds = %178
  %189 = or disjoint i64 %179, 1
  %190 = load double, ptr %177, align 8, !tbaa !4
  %191 = fmul double %190, 1.500000e+00
  %192 = getelementptr inbounds nuw [25 x double], ptr %5, i64 %176, i64 %189
  %193 = load double, ptr %192, align 8, !tbaa !4
  %194 = getelementptr inbounds nuw [25 x double], ptr %3, i64 %154, i64 %189
  %195 = load double, ptr %194, align 8, !tbaa !4
  %196 = tail call double @llvm.fmuladd.f64(double %191, double %193, double %195)
  store double %196, ptr %194, align 8, !tbaa !4
  %197 = add nuw nsw i64 %179, 2
  br label %178

198:                                              ; preds = %178
  %199 = add nuw nsw i64 %176, 1
  %200 = icmp eq i64 %199, 30
  br i1 %200, label %201, label %175, !llvm.loop !25

201:                                              ; preds = %198
  %202 = add nuw nsw i64 %154, 1
  %203 = icmp eq i64 %202, 20
  br i1 %203, label %204, label %153, !llvm.loop !26

204:                                              ; preds = %201, %252
  %205 = phi i64 [ %253, %252 ], [ 0, %201 ]
  br label %206

206:                                              ; preds = %212, %204
  %207 = phi i64 [ 0, %204 ], [ %225, %212 ]
  %208 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %205, i64 %207
  %209 = load x86_fp80, ptr %208, align 16, !tbaa !15
  %210 = fmul x86_fp80 %209, 0xK3FFF999999999999999A
  store x86_fp80 %210, ptr %208, align 16, !tbaa !15
  %211 = icmp eq i64 %207, 24
  br i1 %211, label %226, label %212, !llvm.loop !27

212:                                              ; preds = %206
  %213 = or disjoint i64 %207, 1
  %214 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %205, i64 %213
  %215 = load x86_fp80, ptr %214, align 16, !tbaa !15
  %216 = fmul x86_fp80 %215, 0xK3FFF999999999999999A
  store x86_fp80 %216, ptr %214, align 16, !tbaa !15
  %217 = or disjoint i64 %207, 2
  %218 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %205, i64 %217
  %219 = load x86_fp80, ptr %218, align 16, !tbaa !15
  %220 = fmul x86_fp80 %219, 0xK3FFF999999999999999A
  store x86_fp80 %220, ptr %218, align 16, !tbaa !15
  %221 = or disjoint i64 %207, 3
  %222 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %205, i64 %221
  %223 = load x86_fp80, ptr %222, align 16, !tbaa !15
  %224 = fmul x86_fp80 %223, 0xK3FFF999999999999999A
  store x86_fp80 %224, ptr %222, align 16, !tbaa !15
  %225 = add nuw nsw i64 %207, 4
  br label %206

226:                                              ; preds = %206, %249
  %227 = phi i64 [ %250, %249 ], [ 0, %206 ]
  %228 = getelementptr inbounds nuw [30 x x86_fp80], ptr %7, i64 %205, i64 %227
  br label %229

229:                                              ; preds = %239, %226
  %230 = phi i64 [ 0, %226 ], [ %248, %239 ]
  %231 = load x86_fp80, ptr %228, align 16, !tbaa !15
  %232 = fmul x86_fp80 %231, 0xK3FFFC000000000000000
  %233 = getelementptr inbounds nuw [25 x x86_fp80], ptr %8, i64 %227, i64 %230
  %234 = load x86_fp80, ptr %233, align 16, !tbaa !15
  %235 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %205, i64 %230
  %236 = load x86_fp80, ptr %235, align 16, !tbaa !15
  %237 = tail call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %232, x86_fp80 %234, x86_fp80 %236)
  store x86_fp80 %237, ptr %235, align 16, !tbaa !15
  %238 = icmp eq i64 %230, 24
  br i1 %238, label %249, label %239, !llvm.loop !28

239:                                              ; preds = %229
  %240 = or disjoint i64 %230, 1
  %241 = load x86_fp80, ptr %228, align 16, !tbaa !15
  %242 = fmul x86_fp80 %241, 0xK3FFFC000000000000000
  %243 = getelementptr inbounds nuw [25 x x86_fp80], ptr %8, i64 %227, i64 %240
  %244 = load x86_fp80, ptr %243, align 16, !tbaa !15
  %245 = getelementptr inbounds nuw [25 x x86_fp80], ptr %6, i64 %205, i64 %240
  %246 = load x86_fp80, ptr %245, align 16, !tbaa !15
  %247 = tail call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %242, x86_fp80 %244, x86_fp80 %246)
  store x86_fp80 %247, ptr %245, align 16, !tbaa !15
  %248 = add nuw nsw i64 %230, 2
  br label %229

249:                                              ; preds = %229
  %250 = add nuw nsw i64 %227, 1
  %251 = icmp eq i64 %250, 30
  br i1 %251, label %252, label %226, !llvm.loop !29

252:                                              ; preds = %249
  %253 = add nuw nsw i64 %205, 1
  %254 = icmp eq i64 %253, 20
  br i1 %254, label %255, label %204, !llvm.loop !30

255:                                              ; preds = %252
  tail call fastcc void @print_array(ptr noundef %3, ptr noundef nonnull %6)
  tail call void @free(ptr noundef %3) #8
  tail call void @free(ptr noundef %4) #8
  tail call void @free(ptr noundef %5) #8
  tail call void @free(ptr noundef nonnull %6) #8
  tail call void @free(ptr noundef nonnull %7) #8
  tail call void @free(ptr noundef nonnull %8) #8
  ret i32 0
}

declare dso_local ptr @polybench_alloc_data(i64 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: cold nofree nounwind uwtable
define internal fastcc void @print_array(ptr noundef readonly captures(none) %0, ptr noundef readonly captures(none) %1) unnamed_addr #2 {
  %3 = load ptr, ptr @stderr, align 8, !tbaa !31
  %4 = tail call i64 @fwrite(ptr nonnull @.str, i64 22, i64 1, ptr %3) #9
  %5 = load ptr, ptr @stderr, align 8, !tbaa !31
  %6 = tail call i32 (ptr, ptr, ...) @fprintf(ptr noundef %5, ptr noundef nonnull @.str.1, ptr noundef nonnull @.str.2) #10
  br label %7

7:                                                ; preds = %2, %31
  %8 = phi i64 [ 0, %2 ], [ %32, %31 ]
  %9 = phi double [ 0.000000e+00, %2 ], [ %26, %31 ]
  %10 = phi x86_fp80 [ 0xK00000000000000000000, %2 ], [ %28, %31 ]
  br label %11

11:                                               ; preds = %7, %11
  %12 = phi i64 [ 0, %7 ], [ %29, %11 ]
  %13 = phi double [ %9, %7 ], [ %26, %11 ]
  %14 = phi x86_fp80 [ %10, %7 ], [ %28, %11 ]
  %15 = getelementptr inbounds nuw [25 x double], ptr %0, i64 %8, i64 %12
  %16 = load double, ptr %15, align 8, !tbaa !4
  %17 = getelementptr inbounds nuw [25 x x86_fp80], ptr %1, i64 %8, i64 %12
  %18 = load x86_fp80, ptr %17, align 16, !tbaa !15
  %19 = fcmp olt double %16, 0.000000e+00
  %20 = fneg double %16
  %21 = select i1 %19, double %20, double %16
  %22 = fcmp olt x86_fp80 %18, 0xK00000000000000000000
  %23 = fneg x86_fp80 %18
  %24 = select i1 %22, x86_fp80 %23, x86_fp80 %18
  %25 = fcmp ogt double %21, %13
  %26 = select i1 %25, double %21, double %13
  %27 = fcmp ogt x86_fp80 %24, %14
  %28 = select i1 %27, x86_fp80 %24, x86_fp80 %14
  %29 = add nuw nsw i64 %12, 1
  %30 = icmp eq i64 %29, 25
  br i1 %30, label %31, label %11, !llvm.loop !34

31:                                               ; preds = %11
  %32 = add nuw nsw i64 %8, 1
  %33 = icmp eq i64 %32, 20
  br i1 %33, label %34, label %7, !llvm.loop !35

34:                                               ; preds = %31
  %35 = fcmp une double %26, 0.000000e+00
  br i1 %35, label %36, label %69

36:                                               ; preds = %34, %64
  %37 = phi i64 [ %65, %64 ], [ 0, %34 ]
  %38 = phi double [ %45, %64 ], [ 0.000000e+00, %34 ]
  br label %39

39:                                               ; preds = %47, %36
  %40 = phi i64 [ 0, %36 ], [ %63, %47 ]
  %41 = phi double [ %38, %36 ], [ %62, %47 ]
  %42 = getelementptr inbounds nuw [25 x double], ptr %0, i64 %37, i64 %40
  %43 = load double, ptr %42, align 8, !tbaa !4
  %44 = fdiv double %43, %26
  %45 = tail call double @llvm.fmuladd.f64(double %44, double %44, double %41)
  %46 = icmp eq i64 %40, 24
  br i1 %46, label %64, label %47, !llvm.loop !36

47:                                               ; preds = %39
  %48 = or disjoint i64 %40, 1
  %49 = getelementptr inbounds nuw [25 x double], ptr %0, i64 %37, i64 %48
  %50 = load double, ptr %49, align 8, !tbaa !4
  %51 = fdiv double %50, %26
  %52 = tail call double @llvm.fmuladd.f64(double %51, double %51, double %45)
  %53 = or disjoint i64 %40, 2
  %54 = getelementptr inbounds nuw [25 x double], ptr %0, i64 %37, i64 %53
  %55 = load double, ptr %54, align 8, !tbaa !4
  %56 = fdiv double %55, %26
  %57 = tail call double @llvm.fmuladd.f64(double %56, double %56, double %52)
  %58 = or disjoint i64 %40, 3
  %59 = getelementptr inbounds nuw [25 x double], ptr %0, i64 %37, i64 %58
  %60 = load double, ptr %59, align 8, !tbaa !4
  %61 = fdiv double %60, %26
  %62 = tail call double @llvm.fmuladd.f64(double %61, double %61, double %57)
  %63 = add nuw nsw i64 %40, 4
  br label %39

64:                                               ; preds = %39
  %65 = add nuw nsw i64 %37, 1
  %66 = icmp eq i64 %65, 20
  br i1 %66, label %67, label %36, !llvm.loop !37

67:                                               ; preds = %64
  %68 = tail call double @sqrt(double noundef %45) #8, !tbaa !38
  br label %69

69:                                               ; preds = %67, %34
  %70 = phi double [ %68, %67 ], [ 0.000000e+00, %34 ]
  %71 = fcmp une x86_fp80 %28, 0xK00000000000000000000
  br i1 %71, label %72, label %105

72:                                               ; preds = %69, %100
  %73 = phi i64 [ %101, %100 ], [ 0, %69 ]
  %74 = phi x86_fp80 [ %81, %100 ], [ 0xK00000000000000000000, %69 ]
  br label %75

75:                                               ; preds = %83, %72
  %76 = phi i64 [ 0, %72 ], [ %99, %83 ]
  %77 = phi x86_fp80 [ %74, %72 ], [ %98, %83 ]
  %78 = getelementptr inbounds nuw [25 x x86_fp80], ptr %1, i64 %73, i64 %76
  %79 = load x86_fp80, ptr %78, align 16, !tbaa !15
  %80 = fdiv x86_fp80 %79, %28
  %81 = tail call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %80, x86_fp80 %80, x86_fp80 %77)
  %82 = icmp eq i64 %76, 24
  br i1 %82, label %100, label %83, !llvm.loop !40

83:                                               ; preds = %75
  %84 = or disjoint i64 %76, 1
  %85 = getelementptr inbounds nuw [25 x x86_fp80], ptr %1, i64 %73, i64 %84
  %86 = load x86_fp80, ptr %85, align 16, !tbaa !15
  %87 = fdiv x86_fp80 %86, %28
  %88 = tail call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %87, x86_fp80 %87, x86_fp80 %81)
  %89 = or disjoint i64 %76, 2
  %90 = getelementptr inbounds nuw [25 x x86_fp80], ptr %1, i64 %73, i64 %89
  %91 = load x86_fp80, ptr %90, align 16, !tbaa !15
  %92 = fdiv x86_fp80 %91, %28
  %93 = tail call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %92, x86_fp80 %92, x86_fp80 %88)
  %94 = or disjoint i64 %76, 3
  %95 = getelementptr inbounds nuw [25 x x86_fp80], ptr %1, i64 %73, i64 %94
  %96 = load x86_fp80, ptr %95, align 16, !tbaa !15
  %97 = fdiv x86_fp80 %96, %28
  %98 = tail call x86_fp80 @llvm.fmuladd.f80(x86_fp80 %97, x86_fp80 %97, x86_fp80 %93)
  %99 = add nuw nsw i64 %76, 4
  br label %75

100:                                              ; preds = %75
  %101 = add nuw nsw i64 %73, 1
  %102 = icmp eq i64 %101, 20
  br i1 %102, label %103, label %72, !llvm.loop !41

103:                                              ; preds = %100
  %104 = tail call x86_fp80 @sqrtl(x86_fp80 noundef %81) #8, !tbaa !38
  br label %105

105:                                              ; preds = %103, %69
  %106 = phi x86_fp80 [ %104, %103 ], [ 0xK00000000000000000000, %69 ]
  %107 = load ptr, ptr @stderr, align 8, !tbaa !31
  %108 = tail call i32 (ptr, ptr, ...) @fprintf(ptr noundef %107, ptr noundef nonnull @.str.3, double noundef %26) #10
  %109 = load ptr, ptr @stderr, align 8, !tbaa !31
  %110 = tail call i32 (ptr, ptr, ...) @fprintf(ptr noundef %109, ptr noundef nonnull @.str.4, double noundef %70) #10
  %111 = load ptr, ptr @stderr, align 8, !tbaa !31
  %112 = tail call i32 (ptr, ptr, ...) @fprintf(ptr noundef %111, ptr noundef nonnull @.str.5, x86_fp80 noundef %28) #10
  %113 = load ptr, ptr @stderr, align 8, !tbaa !31
  %114 = tail call i32 (ptr, ptr, ...) @fprintf(ptr noundef %113, ptr noundef nonnull @.str.6, x86_fp80 noundef %106) #10
  %115 = fpext double %70 to x86_fp80
  %116 = fsub x86_fp80 %106, %115
  %117 = fptrunc x86_fp80 %116 to double
  %118 = load ptr, ptr @stderr, align 8, !tbaa !31
  %119 = tail call i32 (ptr, ptr, ...) @fprintf(ptr noundef %118, ptr noundef nonnull @.str.7, double noundef %117) #10
  %120 = load ptr, ptr @stderr, align 8, !tbaa !31
  %121 = tail call i32 (ptr, ptr, ...) @fprintf(ptr noundef %120, ptr noundef nonnull @.str.8, ptr noundef nonnull @.str.9) #10
  %122 = load ptr, ptr @stderr, align 8, !tbaa !31
  %123 = tail call i64 @fwrite(ptr nonnull @.str.10, i64 22, i64 1, ptr %122) #9
  ret void
}

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare dso_local void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.fmuladd.f64(double, double, double) #4

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare x86_fp80 @llvm.fmuladd.f80(x86_fp80, x86_fp80, x86_fp80) #4

; Function Attrs: nofree nounwind
declare dso_local noundef i32 @fprintf(ptr noundef captures(none), ptr noundef readonly captures(none), ...) local_unnamed_addr #5

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(errnomem: write)
declare dso_local double @sqrt(double noundef) local_unnamed_addr #6

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(errnomem: write)
declare dso_local x86_fp80 @sqrtl(x86_fp80 noundef) local_unnamed_addr #6

; Function Attrs: nofree nounwind
declare noundef i64 @fwrite(ptr noundef readonly captures(none), i64 noundef, i64 noundef, ptr noundef captures(none)) local_unnamed_addr #7

attributes #0 = { nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { cold nofree nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #5 = { nofree nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { mustprogress nocallback nofree nounwind willreturn memory(errnomem: write) "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { nofree nounwind }
attributes #8 = { nounwind }
attributes #9 = { cold }
attributes #10 = { cold nounwind }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 7, !"Dwarf Version", i32 4}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{!"clang version 21.1.8 ( 21.1.8-1.module+el8.10.0+23969+c061985f)"}
!4 = !{!5, !5, i64 0}
!5 = !{!"double", !6, i64 0}
!6 = !{!"omnipotent char", !7, i64 0}
!7 = !{!"Simple C/C++ TBAA"}
!8 = distinct !{!8, !9}
!9 = !{!"llvm.loop.mustprogress"}
!10 = distinct !{!10, !9}
!11 = distinct !{!11, !9}
!12 = distinct !{!12, !9}
!13 = distinct !{!13, !9}
!14 = distinct !{!14, !9}
!15 = !{!16, !16, i64 0}
!16 = !{!"long double", !6, i64 0}
!17 = distinct !{!17, !9}
!18 = distinct !{!18, !9}
!19 = distinct !{!19, !9}
!20 = distinct !{!20, !9}
!21 = distinct !{!21, !9}
!22 = distinct !{!22, !9}
!23 = distinct !{!23, !9}
!24 = distinct !{!24, !9}
!25 = distinct !{!25, !9}
!26 = distinct !{!26, !9}
!27 = distinct !{!27, !9}
!28 = distinct !{!28, !9}
!29 = distinct !{!29, !9}
!30 = distinct !{!30, !9}
!31 = !{!32, !32, i64 0}
!32 = !{!"p1 _ZTS8_IO_FILE", !33, i64 0}
!33 = !{!"any pointer", !6, i64 0}
!34 = distinct !{!34, !9}
!35 = distinct !{!35, !9}
!36 = distinct !{!36, !9}
!37 = distinct !{!37, !9}
!38 = !{!39, !39, i64 0}
!39 = !{!"int", !6, i64 0}
!40 = distinct !{!40, !9}
!41 = distinct !{!41, !9}

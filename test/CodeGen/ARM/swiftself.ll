; RUN: llc -verify-machineinstrs -mtriple=armv7k-apple-ios8.0 -mcpu=cortex-a7 -o - %s | FileCheck --check-prefix=CHECK --check-prefix=OPT --check-prefix=TAILCALL %s
; RUN: llc -O0 -verify-machineinstrs -mtriple=armv7k-apple-ios8.0 -mcpu=cortex-a7 -o - %s | FileCheck %s

; RUN: llc -verify-machineinstrs -mtriple=armv7-apple-ios -o - %s | FileCheck --check-prefix=CHECK --check-prefix=OPT %s
; RUN: llc -O0 -verify-machineinstrs -mtriple=armv7-apple-ios -o - %s | FileCheck %s

; Parameter with swiftself should be allocated to r10.
; CHECK-LABEL: swiftself_param:
; CHECK: mov r0, r10
define i8 *@swiftself_param(i8* swiftself %addr0) {
    ret i8 *%addr0
}

; Check that r10 is used to pass a swiftself argument.
; CHECK-LABEL: call_swiftself:
; CHECK: mov r10, r0
; CHECK: bl {{_?}}swiftself_param
define i8 *@call_swiftself(i8* %arg) {
  %res = call i8 *@swiftself_param(i8* swiftself %arg)
  ret i8 *%res
}

; r10 should be saved by the callee even if used for swiftself
; CHECK-LABEL: swiftself_clobber:
; CHECK: push {r10}
; ...
; CHECK: pop {r10}
define i8 *@swiftself_clobber(i8* swiftself %addr0) {
  call void asm sideeffect "", "~{r10}"()
  ret i8 *%addr0
}

; Demonstrate that we do not need any movs when calling multiple functions
; with swiftself argument.
; CHECK-LABEL: swiftself_passthrough:
; OPT-NOT: mov{{.*}}r10
; OPT: bl {{_?}}swiftself_param
; OPT-NOT: mov{{.*}}r10
; OPT-NEXT: bl {{_?}}swiftself_param
define void @swiftself_passthrough(i8* swiftself %addr0) {
  call i8 *@swiftself_param(i8* swiftself %addr0)
  call i8 *@swiftself_param(i8* swiftself %addr0)
  ret void
}

; We can use a tail call if the callee swiftself is the same as the caller one.
; CHECK-LABEL: swiftself_tail:
; TAILCALL: b {{_?}}swiftself_param
; TAILCALL-NOT: pop
define i8* @swiftself_tail(i8* swiftself %addr0) {
  call void asm sideeffect "", "~{r10}"()
  %res = tail call i8* @swiftself_param(i8* swiftself %addr0)
  ret i8* %res
}

; We can not use a tail call if the callee swiftself is not the same as the
; caller one.
; CHECK-LABEL: swiftself_notail:
; CHECK: mov r10, r0
; CHECK: bl {{_?}}swiftself_param
; CHECK: pop
define i8* @swiftself_notail(i8* swiftself %addr0, i8* %addr1) nounwind {
  %res = tail call i8* @swiftself_param(i8* swiftself %addr1)
  ret i8* %res
}

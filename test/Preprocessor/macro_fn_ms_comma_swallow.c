// Test the GNU comma swallowing extension.
// RUN: %clang_cc1 %s -E -fms-compatibility | FileCheck -strict-whitespace %s

// should eat the comma before emtpy varargs
// CHECK: 1: {foo}
#define X1(b, ...) {b,__VA_ARGS__}
1: X1(foo)

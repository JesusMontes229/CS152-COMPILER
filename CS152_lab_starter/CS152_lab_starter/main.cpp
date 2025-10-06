#include <string>
#include <iostream>

using namespace std;

#include "webpage/boilerplate.hpp"

int main() {
    string wasm =R"(
(module
  (import "env" "print" (func $print (param i32)))
  (import "env" "putch" (func $putch (param i32)))
  
  (func $main (result i32)
    i32.const 72
    call $putch
    i32.const 101
    call $putch
    i32.const 108
    call $putch
    i32.const 108
    call $putch
    i32.const 111
    call $putch
    i32.const 32
    call $putch
    i32.const 87
    call $putch
    i32.const 111
    call $putch
    i32.const 114
    call $putch
    i32.const 108
    call $putch
    i32.const 100
    call $putch

    i32.const 0
)
  (export "main" (func $main))
))";
    cout << WEB_PAGE_PREAMBLE;
    cout << wasm << "\n";
    cout << WEB_PAGE_POSTAMBLE;    
    return 0;
}





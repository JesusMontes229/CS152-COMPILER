#include <string>
#include <iostream>

using namespace std;

#include "webpage/boilerplate.hpp"

int main() {
    string wasm =R"(
(module
  (import "env" "print" (func $print (param i32)))
  
  (func $main (result i32)
    i32.const 42
    call $print
    i32.const 0)
    
  (export "main" (func $main))
)
    )";
    cout << WEB_PAGE_PREAMBLE;
    cout << wasm << "\n";
    cout << WEB_PAGE_POSTAMBLE;    
    return 0;
}





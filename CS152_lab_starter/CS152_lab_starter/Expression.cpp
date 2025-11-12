#include <vector>
#include <string>
#include <iostream>
#include <variant>
#include <optional>
#include <functional>
#include <sstream>
#include "HW2.cpp"

using namespace std;
#include "webpage/boilerplate.hpp"

void expression_lab()
{
    const char* source = R"(main () {
   print(1 + 1 - 2)  // 0
   print(9 - 4 * 2)  // 1
   print(~0 + 3)  // 2
   print(428472393 & 1 + 2)  // 3
   print(-7 + 11)  // 4
   print(+7 - 2)  // 5
   print(!0 + 5)  // 6
   
}
)";
Lexer lexer{source};
    auto tokens = lexer();
    Parser parser{tokens.data()};
    Codegen gen{parser.program};

    cout << WEB_PAGE_PREAMBLE;
    cout << gen.wasm.str() << "\n";
    cout << WEB_PAGE_POSTAMBLE;
}


int main(void) {
    expression_lab();
    return 0;
}
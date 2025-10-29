#include <string>
#include <iostream>
#include <vector>
#include <variant>
#include <optional>

using namespace std;

#include "webpage/boilerplate.hpp"

struct stmt;
struct expr;
struct Param{
    string ID;
    bool is_array;
};
struct var_dec{
    string ID;
    optional<string> array_size;
};
struct Int{string value;};
struct VarUse{ string name;};
struct ArrayAccess{string arrayID; vector<expr> index;};
struct FnCall{string funcID; vector<expr> arguements;};
struct Arithmetic{string Opcode; vector<expr> operands;};


struct expr: public variant<Int, VarUse, ArrayAccess, FnCall, Arithmetic>{
    using variant<Int, VarUse, ArrayAccess, FnCall, Arithmetic> :: variant;
};
struct Let{
    vector<var_dec> declarations;
};
struct Return{
    expr _returnValue;
};
struct Block{
    vector<stmt> body;
};
struct Loop{
    Block LoopBody;
};
struct Break{
};
struct Continue{
};
struct If{
    expr Conditon;
    Block IfConditionTrue;
    optional<Block> ElseBlock;
};
struct Assignment{
    expr rhs, lhs;
};
struct stmt: public variant<Block, expr, Let, Return, Loop, Continue, Break, If, Assignment> {
    using variant<Block, expr, Let, Return, Loop, Continue, Break, If, Assignment> :: variant;
};

struct Function{
    string ID;
    vector<Param> param; 
    Block body;
};

struct Program{
    vector<Function> Funcs; 
};
void ast_test() {
   Program p{ {
       Function{
           "add",
           { {"lhs", false}, {"rhs", false} },
           Block{ {
               Let{{ var_dec{"x", ""}, var_dec{"y", ""} }},
               Assignment {
                   VarUse{"x"},
                   Arithmetic{"+", { VarUse{"lhs"}, VarUse{"rhs"} }}
               },
               Int {"5"},
               ArrayAccess{ "x", { Arithmetic{"*", { VarUse{"x"}, VarUse{"y"} }} } },
           } }
       },
       Function{
           "main",
           {},
           Block{ {
               Loop { { {
                   FnCall{"add", {Int{"x"}, Int{"y"}}},
                   Return{Int{"0"}}
               } } },
               Break {},
               Continue {},
               If{ FnCall{"add", {Int{"x"}, Int{"y"}}}, {},{}, }
           } }
       }
   } };
   (void)p;
}
int main() {
    ast_test();
   return 0;
}





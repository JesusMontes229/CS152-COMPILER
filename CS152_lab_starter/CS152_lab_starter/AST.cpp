#include <vector>
#include <string>
#include <iostream>
#include <variant>
#include <optional>
#include <functional>

using namespace std;


struct VariableDeclarations {
    string name;
    optional<string> array_size;
};


struct Parameter {
    string name;    
    bool is_array(); 
};  


struct Stmt;   

struct Expr; 


struct Block {
    vector<Stmt> body;
};


struct Function {
    string name;        
    vector<Parameter> parameters;
    Block body;
}; 


struct Program { 
    vector<Function> functions;
};



struct Let { 
    vector<VariableDeclarations> declarations;    
};



struct Int { 
    string value; 
}; 



struct FunctionCall {
    string name;
    vector<Expr> arguments;
};

struct BinaryOperation {
    vector<Expr> lhs, rhs; 
    string opcode;
};
struct UnaryOperation {
    vector<Expr> lhs;
    string opcode;
};

struct IntegerLiteral {
    string value;
};
struct VariableAccess { 
    string name; 
};
struct ArrayAccess {
    string name;
    string idex;
};

struct Expr : public variant<IntegerLiteral, VariableAccess, FunctionCall, ArrayAccess, BinaryOperation, UnaryOperation> {  
    using variant<IntegerLiteral, VariableAccess, FunctionCall, ArrayAccess, BinaryOperation, UnaryOperation>::variant;
};


struct Return { 
    Expr return_value;
};


struct Loop { 
    Block body;
};


struct Break { 
    ;
};


struct Continue { 
    ;
}; 


struct If { 
    Expr cond;
    Block if_body, else_body; 
};


struct Assign { 
    Expr lhs, rhs;
}; 



struct Stmt : public variant<Block, Expr, Let, Return, Loop, Break, Continue, If, Assign>{ 
    using variant<Block, Expr, Let, Return, Loop, Break, Continue, If, Assign>::variant;  
};





// int main(void) {    
    

//     IntegerLiteral two{"2"}, four{"4"}, ten{"10"};
    
//     BinaryOperation mul{ {four}, {ten}, "*" };
//     BinaryOperation add{ {two}, {mul}, "+" }; 
//     Expr expr = {add};



//     function<void(Expr&)> walk;
//     walk = [&](Expr& ex){  
//         if(auto binop = get_if<BinaryOperation>(&ex)){
//             cout << "(";
//             walk(binop->lhs[0]);
//             cout << " " << binop->opcode << " ";
//             walk(binop->rhs[0]);
//             cout << ")";
//         }
//         else if(auto literal = get_if<IntegerLiteral>(&ex)){
//             cout << literal->value;
//         }
//     };

//     walk(expr);
//     cout << "\n";

//     return 0;
// }
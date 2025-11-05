#include <string>
#include <iostream>
#include <vector>
#include <variant>
#include <optional>
#include <functional>
#include <sstream>

using namespace std;

#include "webpage/boilerplate.hpp"
struct Token {
    string type, value;
    unsigned long line;

    void dump(){
        cerr << type << ", on the line " << line << ": " << value << "\n";
    }

};


struct Lexer {

    const char *it;  
    unsigned long line = 1;


    vector<Token> operator()() {
        vector<Token> tokens;

        while(*it) {tokens.push_back(next()); }
        //tokens.push_back(token("eof"));

        return tokens;
    }

    Token token(string type, string value = ""){
        return {std::move(type), std::move(value), line};
    }

    bool is_whitespace(){
        for(const char& ch : " \n\t\r\v\f"){
            if(*it == ch && *it != '\0'){ 
                return true;
            }
        }
        return false;
    }

    Token whitespace(){
        while(is_whitespace()) {
            if('\n' == *it){  
                line += 1;
            }
            ++it; 
            }

        return next();
    }

    Token comment(){
        while(it[0] && it[0] != '\n'){
            ++it;
        }
        return next();
    }

    bool is_digit(){
        return *it >= '0' && *it <= '9';
    }

    bool is_letter(){
        return *it >= 'a' && *it <= 'z' || *it >= 'A' && *it <= 'Z';
    }

    private:

    Token next() {
        if(is_whitespace()){
            return whitespace();
        }

        if(it[0] == '/' && it[1] == '/'){
            return comment();
        }

        for(const auto& digraph : { "==", "!=", "<<", "<=", ">>", ">=" } ){
            if(it[0] == digraph[0] && it[1] == digraph[1]) {
                it += 2;               
                return token(digraph);
            }
        }

        for(const char& monograph : "~^*%():{}[]+-<>!=&|/," ){ 
            if(it[0] == monograph && it[0] != '\0' ){
                ++it;
                return token({monograph});
            }
        }

        if(is_digit()) {
            string value = "";
            while(is_digit()) {
                value += *it++;  
            }

            return token("int", std::move(value));
        }

        if(is_letter() || *it == '_'){
            string value;
            while(is_letter() || is_digit() || *it == '_'){
                value += *it++;
            }

            for(const auto& keyword : { "let", "break", "continue", "return", "loop", "if", "else" }){
                if(keyword == value){
                    return token(keyword);
                }    
            }
            return token("id",std::move(value));
        }

        if(0 == *it){
            return token("eof");
        }

        cerr << "lexer hit invalid charcter " << *it << "\n";
        exit(1);
    }

};

struct VariableDeclarations {
    string name;
    optional<string> array_size;
};


struct Parameter {
    string name;    
    bool is_array; 
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

struct Parser{
    Parser(Token* it) : it{it}
    {

    }
    Program operator()() {return program(); }
    private:
    Token* it;
    string Expect(string type)
{
    if(type != it->type)
    {
        cerr << "Expected: " << type << " but saw: " << it->type << "\n";
        exit(1);
    }
    auto res = it->value;
    ++it;
    return res;
}
bool is(string type)
{
    //cout << it->type  << " " << it->value << "\n";
    return type == it->type;
}
bool was(string type)
{
    if(is(type))
    {
        ++it;
        return true;
    }
    return false;
}
Expr Primary()
{
    if(is("int")) {return IntegerLiteral{Expect("int")};}
    else if(is("id")){
        auto name = Expect("id");
        vector<Expr> args;
        Expect("(");
        while(!is(")"))
        {
            args.push_back(expr());
            if(!was(",")){break;}
        }
        Expect(")");
        return FunctionCall{std::move(name),std:: move(args)};
    }
    else{
        cerr << "expression parser hit unexpected token";
        it->dump();
        cerr << "\n";
        exit(1);
    }

}
Expr expr()
{
    return Primary();
}
Stmt stmt()
{
    return expr();
}
Block block()
{
    Block b;
    Expect("{");
     while(!is("}"))
    {
        b.body.push_back(stmt());
    }
    Expect("}");
    
    return b;

}
Parameter param()
{
    Parameter p;
    p.name = Expect("id");
    p.is_array = is("[");
    if(was("[")){Expect("]");}
    return p;
}
Function function()
{
    Function f;
    f.name = Expect("id");
    Expect("(");
    while(!is(")"))
    {
        f.parameters.push_back(param());
        if(!was(",")){break;}
    }
    Expect(")");
    f.body = block();
    return f;

}
Program program()
{
    Program p;
    while(!is("eof")){
        p.functions.push_back(function());
    }
    return p;
}

};
struct CodeGen{
    void expr(Expr &e)
    {
        if(auto *int_lit = get_if<IntegerLiteral>(&e))
        {
            //cout << "int_lit->value: " << int_lit->value << "\n";
            output << "i32.const " << int_lit->value << "\n";
        }
        else if(auto *call = get_if<FunctionCall>(&e))
        {
            for(auto arg: call->arguments)
            {
                expr(arg);
            }
            output << "call $" << call->name << "\n";
        }
        else
        {
            cerr << "expression case not handled";
            exit(1);
        }
    }
    void stmt(Stmt& s)
    {
        if(auto *e = get_if<Expr>(&s))
        {
            expr(*e);
            output << "drop\n";
        }
        else
        {
            cerr << "stmt case not handled";
            exit(1);
        }
    }
    void block(Block& b)
    {
        //output << "{";
        for(auto s: b.body)
        {
            stmt(s);
        }
        
    }
    void function(Function& fn){
       // output << fn.name;
        output << "(func $" << fn.name << "\n";
        block(fn.body);
        output << ")";
        
    }
    void program(Program& p){
        output << "(module\n";
        output << "(import \"env\" \"print\" (func $print (param i32) (result i32)))\n";
        output << "(import \"env\" \"putch\" (func $putch (param i32) (result i32)))\n";
        for (auto& fn : p.functions) {function(fn); }
        output << "(export \"main\"(func $main))\n";
        output << ")\n";
    }


    CodeGen(Program& p) {program(p);}
    string operator()() {return output.str();}
    private:
    stringstream output;
};

void hw2()
{
    const char *source = R"(main () {
   putch(72)   // 'H'
   putch(87)   // 'W'
   putch(33)   // '!'
   putch(32)   // ' '
   putch(35)   // '#'
   print(2)    // '2'
   putch(10)   // '\n'
   }
   )";
    Lexer lexer{source};
    auto tokens = lexer();
    Parser parser{tokens.data()};
    auto Parsed = parser();
    CodeGen gen(Parsed);
    auto outputstring = gen();
    string wasm = outputstring;

    cout << WEB_PAGE_PREAMBLE;
    cout << wasm << "\n";
    cout << WEB_PAGE_POSTAMBLE;



}
int main()
{
    
    hw2(); 
    return 0;
}


#include <vector>
#include <string>
#include <iostream>
#include <variant>
#include <optional>
#include <functional>
#include <sstream>

#include "AST.cpp"
#include "Lexer.cpp"

using namespace std;


#include "webpage/boilerplate.hpp"


struct Parser {
    Program program;

    Parser(Token *tokens){
        it = tokens;
        program = parse_program();
    }



    private:
    Token *it;

    bool is(string expected_type){
        return it->type == expected_type;
    }
    bool was(string expected_type){
        bool _is = is(expected_type);
        if(_is){
            ++it;
        }
        return _is;
    }

    optional<string> was(const vector<string>& expected_tokens){
        for(const string& expected_token : expected_tokens){
            if(was(expected_token)){
                return expected_token;
            }
        }
        return{};
    }



    string expect(string expected_type){
        if(!is(expected_type) ){
            //cerr << "Expected " << expected_type << " but saw "; 
            cerr << "oh no, wanted  " << expected_type << " but we got " << it->type << "\n";
            exit(EXIT_FAILURE);
        }
        string value = it->value;
        ++it;
        return value;
    }

    Program parse_program(){
        Program p;
        while(!is("eof")){
            p.functions.push_back(parse_function());
        }
        return p;
    }

    Function parse_function(){
        Function f;

        f.name = expect("id");
        expect("(");
        expect(")");

        f.body = parse_block(); 

        return f;
    }

    Block parse_block(){
        Block b;

        expect("{");
        while(!is("}")){
            b.body.push_back(parse_statement());
        }
        expect("}");

        return b;
    }

    Stmt parse_statement(){
        Stmt s;
        s = parse_expression();
        return s;
    }

    Expr parse_expression(){
        return parse_relational();
    }


    Expr parse_relational(){
        Expr lhs = parse_add();
        while(1){
            optional<string> op = was({">", "<", "<=",">=", "==", "!="});
            if(op){
                lhs = BinaryOperation{{std::move(lhs), parse_add()}, *op}; 
            }
            else{
                return lhs;
            }
        }
    }




    Expr parse_add(){
        Expr lhs = parse_mul();
        while(1){
            optional<string> op = was({"+", "-", "^", "|"});
            if(op){
                lhs = BinaryOperation{{std::move(lhs), parse_mul()}, *op}; 
            }
            else{
                return lhs;
            }
        }
    }

    Expr parse_mul(){
        Expr lhs = parse_unary();
        while(1){
            optional<string> op = was({"<<", ">>", "&", "*", "/", "%" });
            if(op){
                lhs = BinaryOperation{{std::move(lhs), parse_unary()}, *op}; 
            }
            else{
                return lhs;
            }
        }
    }


    Expr parse_unary(){
        while(1){
            optional<string> op = was({"+", "-", "~", "!"}); 
            if(op){
                return UnaryOperation{{parse_unary()}, *op}; 
            }
            else{
                return parse_primary();
            }
        }
    }


    Expr parse_primary(){
        if(is("int")){
            return IntegerLiteral{expect("int")};
        }
        if(was("(")){
            Expr e = parse_expression();
            expect(")");
            return e;
        }
        if(is("id")){
            string name = expect("id");

            if(was("[")){ // array access
                ArrayAccess aa;
                aa.name = std::move(name);
                aa.index.push_back(parse_expression()); 
                expect("]");
                return aa;
            }

            if(was("(")){ 
                FunctionCall call;
                call.name = std::move(name);
                while(!is(")")){
                    call.arguments.push_back(parse_expression());
                    if(!is(",")){
                        break;
                    }
                    expect(",");
                }
                expect(")");
                return call;
            }

            return VariableAccess{std::move(name)}; 

        }


        cerr << "Parse Expression Failed D:\n";
        cerr << "\n";
        exit(EXIT_FAILURE);
    }

};


struct Codegen{
    stringstream wasm;

    Codegen(Program& program){
        gen_program(program);

    }   
    private:
    void gen_program(Program& program){
        wasm << "(module\n";
        wasm << " (import \"env\" \"print\" (func $print (param i32) (result i32)))\n";
        wasm << " (import \"env\" \"putch\" (func $putch (param i32) (result i32)))\n";

        for(auto& f : program.functions){
            gen_function(f);
        }

        wasm << "(export \"main\" (func $main))\n";
        wasm << ")\n";
    } 

    void gen_function(Function& f){
        
        wasm << "(func $" << f.name << " (result i32)\n";
        gen_block(f.body);
        wasm << "i32.const 0\n";
        wasm << "         )\n";
    }

    void gen_block(Block& b){
        for(auto& s : b.body){
            gen_statement(s);
        }
    }

    void gen_statement(Stmt& s){
        if(auto *expr = get_if<Expr>(&s)){
            gen_expression(*expr);
            wasm <<"drop\n";
        }
        else{
            cerr << "unhandled statment type\n";
            exit(EXIT_FAILURE);
        }
    }

    void gen_expression(Expr& e){
        if(auto * lit = get_if<IntegerLiteral>(&e)){
           wasm << "i32.const " << lit->value << "\n";
        }
        else if(auto *call = get_if<FunctionCall>(&e)){
            for(auto& arg : call->arguments){
                gen_expression(arg);
            }
            wasm << "call $" << call->name << "\n";
        }

        else if(auto *UnaryOp = get_if<UnaryOperation>(&e)){
            for(auto& arg : UnaryOp->lhs){
                gen_expression(arg);
            }

            if(UnaryOp->opcode == "+"){

            }
            else if(UnaryOp->opcode == "-"){
                wasm << "i32.const -1\n" << "i32.mul\n";
            }
            else if(UnaryOp->opcode == "~"){
                wasm << "i32.const -1\n" << "i32.xor\n";
            }
            else if(UnaryOp->opcode == "!"){
                wasm << "i32.eqz\n";
            }
            else{
                cerr << "UnaryOp unimplemented\n";
                exit(EXIT_FAILURE);
            }
        }


        else if(auto *BinaryOp = get_if<BinaryOperation>(&e)){
            for(auto& arg : BinaryOp->args){
                gen_expression(arg);
            }

            if(BinaryOp->opcode == "+"){
                wasm << "i32.add\n";
            }
            else if(BinaryOp->opcode == "-"){
                wasm << "i32.sub\n";     
            }
            else if(BinaryOp->opcode == "<<"){  
                wasm << "i32.shl\n";
            }
            else if(BinaryOp->opcode == ">>"){ 
                wasm << "i32.shr_s\n"; 
            }
            else if(BinaryOp->opcode == "&"){
                wasm << "i32.and\n";
            }
            else if(BinaryOp->opcode == "*"){
                wasm << "i32.mul\n";
            }
            else if(BinaryOp->opcode == "/"){
                wasm << "i32.div_s\n";
            }
            else if(BinaryOp->opcode == "%"){
                wasm << "i32.rem_s\n";
            }
            else if(BinaryOp->opcode == "^"){
                wasm << "i32.xor\n";
            }
            else if(BinaryOp->opcode == "|"){
                wasm << "i32.or\n";
            }
            else if(BinaryOp->opcode == ">"){
                wasm << "i32.gt_s\n";
            }
            else if(BinaryOp->opcode == ">="){
                wasm << "i32.ge_s\n";
            }
            else if(BinaryOp->opcode == "<"){
                wasm << "i32.lt_s\n";
            }
            else if(BinaryOp->opcode == "<="){
                wasm << "i32.le_s\n";
            }
            else if(BinaryOp->opcode == "=="){
                wasm << "i32.eq\n";
            }
            else if(BinaryOp->opcode == "!="){
                wasm << "i32.ne\n";
            }
            else{
                cerr << "BinaryOp unimplemente\n";
                exit(EXIT_FAILURE);
            }
        }

        else{
            cerr << "unhandled expression type\n";
            exit(EXIT_FAILURE);
        }
    }
};

// string indent(string wasm){
//         string res;
//         const char *it = wasm.c_str();
//         int level =  0;
//         while(*it){
//             res += *it;
//             if('\n' == *it){
//                 for(int i = 0; i < level; ++i){
//                     res += ' ';
//                 }
//             }
//             if('(' == *it){
//                 level += 2;
//             }
//             if(')' == *it){
//                 level -= 2;
//             }
//             ++it;
//         }
//         return res;
// }





void expression_run(){
    const char *source = R"(
        main () {
            print(1 + 1 - 2)  // 0
            print(9 - 4 * 2)  // 1
            print(~0 + 3)  // 2
            print(428472393 & 1 + 2)  // 3
            print(-7 + 11)  // 4
            print(+7 - 2)  // 5
            print(!0 + 5)  // 6
            print(5 << 4 - 5 * 16 + 7) // 7
            print(+-++-++++---++---+++---+++-43-35) // 8
            print(~~5 + 4)  // 9
            print(5 * (3 + 4) - 25)  // 10
            print(11)  // 11
            print(0000000 + 12)  // 12
            print(0005 - 5 + 13)  // 13
            print(12345678 - 12345678 + 14)  // 14
            print(100 << 10 >> 10 - 100 + 15)  // 15
            print((12 + 3) % 4 - 3 + 16)  // 16
            print(7 / 2 - 3 + 17)  // 17
            print((6 != 8 > 5) + 18)   // 18
            print((1 < 2 <= 3 > 4 >= 5 != 7 == 1) - 1 + 19)   // 19
            print(1 + 2 +3 + 4 * 5 * 6 *7 *8 * 9 ^ 1 ^ 1 ^2 ^ 3 ^ 4 - 60483 + 20)   // 20
            print((-5 + (((((((((((((((((((30))))))))))) + 5)))))))) - 30) + 21)   // 21
            print((0 | 1) - 1 + 22)   // 22
            print(~(1 & 0) + 1 + 23)  // 23
            print(~(1 & 1) + 2 + 24)  // 24
            print(~(1 | 0) + 2 + 25)  // 25
            print(~(0 | 0) + 1 + 26)  // 26
            print((1 & 0 | 1 | 1 | 1 | 534543424 & 1) - 1 + 27)  // 27
            print((1 + 2 * 3) - 7 + 28)  // 28
            print((2 * 3 + 4) - 10 + 29)  // 29
            print((1 << 2 + 1) - 5 + 30)  // 30
            print((5 - 3 - 1) - 1 + 31)  // 31
            print((8 / 4 / 2) - 1 + 32)  // 32
            print((-2 * 3) + 6 + 33)  // 33
            print((!0 + 1) - 2 + 34)  // 34
            print((~1 & 3) - 2 + 35)  // 35
            print((1 - 2 - 3) + 4 + 36)  // 36
            print(37 + (16 / 4 / 2) - 2)  // 37
            print((1 << 2 << 1) - 8 + 38)  // 38
            print((1 == 2 == 0) - 1 + 39)  // 39
            print((10 < 2) + 40)  // 40
            print((1 < 3) - 1 + 41)  // 41
            print(((1 < 2) < 3) - 1 + 42)  // 42
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


void hw2(){
    const char *source = R"(
        main () {
            putch(65)   // 'H'
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
    Codegen gen{parser.program};

    cout << WEB_PAGE_PREAMBLE;
    cout << gen.wasm.str() << "\n";
    cout << WEB_PAGE_POSTAMBLE;


}


int main(void) {
    //hw2();

    expression_run();

    return 0;
}
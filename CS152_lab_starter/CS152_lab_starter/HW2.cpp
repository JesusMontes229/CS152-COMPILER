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
       ++it;
       return _is;
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
        // while(!is(")")){
        //     f.parameters.push_back(param());
        // }
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
        if(is("int")){
            //cerr << "ERROR HERE \n";
            return IntegerLiteral{expect("int")};
        }
        FunctionCall call;
        call.name = expect("id");
        expect("(");
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
        // wasm << "        (func $main (result i32)\n";
        // wasm << "            i32.const 0\n";
        // wasm << "        )\n";

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
        else{
            cerr << "unhandled statment type\n";
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

void hw2(){
    const char *source = R"(
        main () {
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
    Codegen gen{parser.program};

    cout << WEB_PAGE_PREAMBLE;
    cout << gen.wasm.str() << "\n";
    cout << WEB_PAGE_POSTAMBLE;


}

int main(void) {
    hw2();
    return 0;
}





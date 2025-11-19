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



    VariableDeclarations parse_var_dec(){
        VariableDeclarations v_d;  
        v_d.name = expect("id");
        if(was("[")){   
            v_d.array_size = expect("int"); 
            expect("]");
        }
        return v_d;
    }


    Stmt parse_statement(){
        if(was("let")){ 
            Let l;
            while(1){
                l.declarations.push_back(parse_var_dec());
                if(!was(",")){
                    return l;
                }
            }
        }
        Expr lhs = parse_expression();
        if(!was("=")){
            return lhs;
        }
        return Assign{std::move(lhs), parse_expression()}; 
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

            if(was("[")){ 
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
        stringstream decl, inst; 
        unsigned long mangle_counter, stack_counter; 

        string mangle(const string& name){
            return name + to_string(mangle_counter);
        }

        struct Symbol{
            string mangled_name;
            bool is_array;
        };

        struct SymbolTable{ 
            vector<unordered_map<string, Symbol>> scopes;

            void operator++(){  
                scopes.push_back({});
            }
            void operator--(){
                scopes.pop_back();
            }

            Symbol& operator[](const string& name){ 
                auto it = scopes.rbegin(); 
                auto end = scopes.rend();

                for(; it != end; ++it){ 
                    auto& scope = *it;
                    if(scope.contains(name)){  
                        return scope[name];
                    }
                }
                cerr << "oh we looked up " << name << " but never found a symbol for it\n";
                exit(EXIT_FAILURE);
            }

        } symbols; 


        void symbol_push(const VariableDeclarations& dec ){
            auto& scope = symbols.scopes.back();
            if(scope.contains(dec.name)){
                cerr << "Attempted redeclaration of " << dec.name << "\n";
                exit(EXIT_FAILURE);
            }
            scope[dec.name] = Symbol{mangle(dec.name), dec.array_size.has_value()};

            if(dec.array_size){ 
                stack_counter += stoul(*dec.array_size); 
            }
        }




    void gen_program(Program& program){
        wasm << "(module\n";
        wasm << " (import \"env\" \"print\" (func $print (param i32) (result i32)))\n";
        wasm << " (import \"env\" \"putch\" (func $putch (param i32) (result i32)))\n";
        
        wasm << "(memory 1 65536)\n"; 
        wasm << "(global $stack_ptr (mut i32) (i32.const 0))\n";

        for(auto& f : program.functions){
            gen_function(f);
        }

        wasm << "(export \"main\" (func $main))\n";
        wasm << ")\n";
    } 


    void gen_function(Function& f){
        decl = {}; 
        inst = {}; 
        mangle_counter = stack_counter = 0; 

        wasm << "(func $" << f.name << " (result i32)\n";
        gen_block(f.body); 


        wasm << decl.str(); 

        wasm << ";; function prologue\n";
        wasm << "global.get $stack_ptr\n";
        wasm << "i32.const " << 4 * stack_counter << "\n"; 
        wasm << "i32.add\n";
        wasm << "global.set $stack_ptr\n"; 
        wasm << ";; ~function prologue\n";

        wasm << inst.str(); 

        wasm << ";; function epilogue\n";
        wasm << "global.get $stack_ptr\n"; 
        wasm << "i32.const " << 4 * stack_counter << "\n"; 
        wasm << "i32.sub\n"; 
        wasm << "global.set $stack_ptr\n"; 
        wasm << ";; ~function epilogue\n";



        wasm << "i32.const 0\n";
        wasm << "         )\n";
    }

    void gen_block(Block& b){
        ++symbols; // entering a scope
        for(auto& s : b.body){
            gen_statement(s);
        }
        --symbols;
    }

   

    void gen_statement(Stmt& s){
        if(auto *expr = get_if<Expr>(&s)){
            gen_expression(*expr);
            inst <<"drop\n";
        }  
        else if(auto *let = get_if<Let>(&s)){
            for(auto& declaration : let->declarations){
                symbol_push(declaration); 
                Symbol& s = symbols[declaration.name];

                if(declaration.array_size){ 
                    decl << "(local $" << s.mangled_name << " i32)\n";
                    inst << "global.get $stack_ptr\n"; 
                    inst << "i32.const " << (4 * stack_counter) << "\n"; 
                    inst << "i32.sub\n";
                    inst << "local.set $" << s.mangled_name << "\n";
                }
                else{  
                    decl << "(local $" << s.mangled_name << " i32)\n";
                }
            }
        }
        else if(auto *assignment = get_if<Assign>(&s)){
            if(auto *lhs_var_acc = get_if<VariableAccess>(&assignment->lhs)){
                Symbol& s = symbols[lhs_var_acc->name];
                if(s.is_array){
                    cerr << "Attempted assignment to array like it was a variable\n";
                    exit(EXIT_FAILURE);
                }

                gen_expression(assignment->rhs);
                inst << "local.set $" << s.mangled_name << "\n";
            }
            else if(auto *lhs_var_acc = get_if<ArrayAccess>(&assignment->lhs)){
                Symbol& s = symbols[lhs_var_acc->name];
                if(!s.is_array){
                    cerr << "Attempted assignment to variable like it was an array\n";
                    exit(EXIT_FAILURE);
                }
                inst << "local.get $" << s.mangled_name << "\n"; 
                gen_expression(lhs_var_acc->index[0]);  
                inst << "i32.const 4\ni32.mul\ni32.add\n"; 
                gen_expression(assignment->rhs); 
                inst << "i32.store\n"; 
            }
            else{
                cerr << "Tried to assign to an expression that isn't assignable\n";
                exit(EXIT_FAILURE);
            }
        }
        else{
            cerr << "unhandled statment type\n";
            exit(EXIT_FAILURE);
        }
    }

    void gen_expression(Expr& e){
        if(auto * lit = get_if<IntegerLiteral>(&e)){
           inst << "i32.const " << lit->value << "\n";
        }
        else if(auto *call = get_if<FunctionCall>(&e)){
            for(auto& arg : call->arguments){
                gen_expression(arg);
            }
            inst << "call $" << call->name << "\n";
        }

        else if(auto *UnaryOp = get_if<UnaryOperation>(&e)){
            for(auto& arg : UnaryOp->lhs){
                gen_expression(arg);
            }

            if(UnaryOp->opcode == "+"){

            }
            else if(UnaryOp->opcode == "-"){
                inst << "i32.const -1\n" << "i32.mul\n";
            }
            else if(UnaryOp->opcode == "~"){
                inst << "i32.const -1\n" << "i32.xor\n";
            }
            else if(UnaryOp->opcode == "!"){
                inst << "i32.eqz\n";
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
                inst << "i32.add\n";
            }
            else if(BinaryOp->opcode == "-"){
                inst << "i32.sub\n";     
            }
            else if(BinaryOp->opcode == "<<"){  
                inst << "i32.shl\n";
            }
            else if(BinaryOp->opcode == ">>"){ 
                inst << "i32.shr_s\n"; 
            }
            else if(BinaryOp->opcode == "&"){
                inst << "i32.and\n";
            }
            else if(BinaryOp->opcode == "*"){
                inst << "i32.mul\n";
            }
            else if(BinaryOp->opcode == "/"){
                inst << "i32.div_s\n";
            }
            else if(BinaryOp->opcode == "%"){
                inst << "i32.rem_s\n";
            }
            else if(BinaryOp->opcode == "^"){
                inst << "i32.xor\n";
            }
            else if(BinaryOp->opcode == "|"){
                inst << "i32.or\n";
            }
            else if(BinaryOp->opcode == ">"){
                inst << "i32.gt_s\n";
            }
            else if(BinaryOp->opcode == ">="){
                inst << "i32.ge_s\n";
            }
            else if(BinaryOp->opcode == "<"){
                inst << "i32.lt_s\n";
            }
            else if(BinaryOp->opcode == "<="){
                inst << "i32.le_s\n";
            }
            else if(BinaryOp->opcode == "=="){
                inst << "i32.eq\n";
            }
            else if(BinaryOp->opcode == "!="){
                inst << "i32.ne\n";
            }
            else{
                cerr << "BinaryOp unimplemente\n";
                exit(EXIT_FAILURE);
            }
        }

        
        else if(auto *Var_acc = get_if<VariableAccess>(&e)){
            Symbol& s = symbols[Var_acc->name];
            inst << "local.get $" << s.mangled_name << "\n";
        }

        else if(auto *Arr_acc = get_if<ArrayAccess>(&e)){
            Symbol& s = symbols[Arr_acc->name];
            if(!s.is_array){
                cerr << "Old C stuff, denied :(\n";
                exit(EXIT_FAILURE);
            }
            
            inst << "local.get $" << s.mangled_name << "\n";
            gen_expression(Arr_acc->index[0]); 
            inst << "i32.const 4\ni32.mul\ni32.add\n";
            inst << "i32.load\n";   
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

void Variable_Run(){
    const char *source = R"(
        main() {
            let x
            x = 5 + 3 - 8 // 0
            
            let w[16], y [32], z, w2[16]


            z = x
            x = 1
            y[x] = 7 + 12 - 18 // 1


            y[0] = y[y[x]] + y[1] * 1 + (y[x] + 1) - 1  // 2


            print(print(z) + 1)
            print(y[x] + 1)
            print(y[0])
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

    //expression_run();

    Variable_Run();

    return 0;
}
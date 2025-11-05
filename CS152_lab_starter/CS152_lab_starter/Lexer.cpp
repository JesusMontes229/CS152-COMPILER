#include <vector>
#include <string>
#include <iostream>

using namespace std;

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

// int main() {

//     Lexer lexer_run{R"(
//         // symbol tokens
//             ~ ^ * % ( ) : { } [ ] + ,
//             a b c
//             = ==
//             ! !=
//             < << <=
//             > >> >=
//             - ->
//             d3434_43d
//             _d3434_43d
//             // comment
//             3434273
//             343
//             -3432
//             98904783
//             let break continue return loop if else
//             // comment   
//     )"};  // input that the lexer is assigned/will check and read to see what tokens it has and such

//     vector<Token> token_gather = lexer_run();  
//                                                
//                                               
//                                             
//     for (auto token_run :token_gather){  
//         token_run.dump();                
//     }                                
//     return 0;
// }


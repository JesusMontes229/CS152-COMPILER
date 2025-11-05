set -e
clear
clang++ \
    -O3 -std=c++20 -ferror-limit=2 \
    -fsanitize=address \
    -Wall -Wno-unqualified-std-cast-call -Wno-logical-op-parentheses \
    HW2.cpp AST.cpp Lexer.cpp -o temp
./temp > index.html
rm temp
echo "Ran Bash Script"
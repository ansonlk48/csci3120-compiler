#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <cctype>

using namespace std;

// Token type codes
enum TokenType {
    // Keywords
    KW_INT = 3,
    KW_MAIN = 4,
    KW_CHAR = 5,
    KW_FOR = 6,
    KW_IF = 7,
    KW_ELSE = 8,
    KW_RETURN = 9,
    
    // Operators
    OP_ASSIGN = 10,
    OP_EQ = 11,       // Assuming second '=' is equality
    OP_GE = 12,
    OP_LE = 13,
    OP_GT = 14,
    OP_LT = 15,
    OP_ADD = 16,
    OP_SUB = 17,
    OP_MUL = 18,
    OP_DIV = 19,
    
    // Punctuation
    PUN_LBRACE = 20,
    PUN_RBRACE = 21,
    // ... add other punctuation codes
};

unordered_map<string, int> keywords = {
    {"int", KW_INT},
    {"main", KW_MAIN},
    {"char", KW_CHAR},
    {"for", KW_FOR},
    {"if", KW_IF},
    {"else", KW_ELSE},
    {"return", KW_RETURN}
};

unordered_map<string, int> operators = {
    {"=", OP_ASSIGN},
    {"==", OP_EQ},
    {">=", OP_GE},
    {"<=", OP_LE},
    {">", OP_GT},
    {"<", OP_LT},
    {"+", OP_ADD},
    {"-", OP_SUB},
    {"*", OP_MUL},
    {"/", OP_DIV}
};

unordered_map<char, int> punctuation = {
    {'{', PUN_LBRACE},
    {'}', PUN_RBRACE}
    // Add other punctuation mappings
};

void tokenize(const string& src) {
    int pos = 0;
    while (pos < src.length()) {
        char current = src[pos];
        
        // Skip whitespace
        if (isspace(current)) {
            pos++;
            continue;
        }
        
        // Handle keywords and identifiers
        if (isalpha(current)) {
            string word;
            while (pos < src.length() && isalnum(src[pos])) {
                word += src[pos++];
            }
            if (keywords.count(word)) {
                cout << "<" << word << ", " << keywords[word] << ">" << endl;
            }
            // Else handle as identifier (add your ID token type)
            continue;
        }
        
        // Handle operators
        if (operators.count(string(1, current))) {
            // Check for multi-character operators
            if (pos + 1 < src.length()) {
                string two_char = string(1, current) + src[pos+1];
                if (operators.count(two_char)) {
                    cout << "<" << two_char << ", " << operators[two_char] << ">" << endl;
                    pos += 2;
                    continue;
                }
            }
            cout << "<" << current << ", " << operators[string(1, current)] << ">" << endl;
            pos++;
            continue;
        }
        
        // Handle punctuation
        if (punctuation.count(current)) {
            cout << "<" << current << ", " << punctuation[current] << ">" << endl;
            pos++;
            continue;
        }
        
        // Handle numbers (add your implementation)
        
        pos++;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source_file>" << endl;
        return 1;
    }
    
    ifstream file(argv[1]);
    string source((istreambuf_iterator<char>(file)), 
                 istreambuf_iterator<char>());
    
    tokenize(source);
    return 0;
}

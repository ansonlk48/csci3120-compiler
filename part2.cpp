#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
using namespace std;

struct Token {
    string value;
    string type;
    int type_num;
};

vector<Token> tokens;
void read_tokens(const string& filename) {
    ifstream fin(filename);
    string line;
    while (getline(fin, line, '>')) { // Read until '>'
        size_t start = line.find('<');
        if (start == string::npos) continue;
        size_t quote1 = line.find('"', start);
        size_t quote2 = line.find('"', quote1 + 1);
        if (quote1 == string::npos || quote2 == string::npos) continue;
        string value = line.substr(quote1 + 1, quote2 - quote1 - 1);

        size_t comma = line.find(',', quote2);
        if (comma == string::npos) continue;
        size_t num_start = line.find_first_of("0123456789", comma);
        if (num_start == string::npos) continue;
        int type_num = stoi(line.substr(num_start));

        tokens.push_back({value, "", type_num});
    }
}

int pos = 0;

// Forward declarations
bool START();
bool EX_DECLA();
bool FUNC_DEF();
bool DECLA();
bool TYPE();
bool id();
bool BLOCK_ST();
bool PARAM_LIST();
bool VAR_LIST();
bool VAR();
bool INITIAL();
bool EP(); 
bool LOGC_EP();
bool MATH_EP();
bool TD();
bool TERM();
bool REAL();
bool BLOCK_ST();
bool STATM();
bool RETURN_ST();
bool WHILE_ST();
bool ASS_ST();   // Placeholder
bool IF_ST();    // Placeholder
bool FOR_ST();   // Placeholder

bool match(const string& val) {
    if (pos < tokens.size() && tokens[pos].value == val) {
        pos++;
        return true;
    }
    return false;
}

bool TYPE() {
    // Assume type_num for types is 10, 12, 15, 16, 17, 18, 19 (from your keywords)
    if (pos < tokens.size() && (
        tokens[pos].type_num == 10 || // int
        tokens[pos].type_num == 12 || // char
        tokens[pos].type_num == 15 || // float
        tokens[pos].type_num == 16 || // double
        tokens[pos].type_num == 17 || // long
        tokens[pos].type_num == 18 || // short
        tokens[pos].type_num == 19    // void
    )) {
        pos++;
        return true;
    }
    return false;
}

bool id() {
    // Assume type_num == 0 for identifiers
    if (pos < tokens.size() && tokens[pos].type_num == 0) {
        pos++;
        return true;
    }
    return false;
}

bool BLOCK_ST() {
    int start = pos;
    if (STATM()) {
        // Try to parse more statements (BLOCK_ST STATM)
        while (STATM()) {}
        return true;
    }
    pos = start;
    return false;
}

bool STATM() {
    int start = pos;
    if (DECLA()) return true;
    pos = start;
    if (ASS_ST()) return true;   // Placeholder
    pos = start;
    if (IF_ST()) return true;    // Placeholder
    pos = start;
    if (FOR_ST()) return true;   // Placeholder
    pos = start;
    if (WHILE_ST()) return true;
    pos = start;
    if (RETURN_ST()) return true;
    pos = start;
    return false;
}

bool RETURN_ST() {
    int start = pos;
    if (match("return")) {
        if (EP() && match(";")) {
            return true;
        }
    }
    pos = start;
    return false;
}

// WHILE_ST → while ( LOGC_EP ) { BLOCK_ST }
bool WHILE_ST() {
    int start = pos;
    if (match("while")) {
        if (match("(")) {
            if (LOGC_EP() && match(")") && match("{")) {
                if (BLOCK_ST() && match("}")) {
                    return true;
                }
            }
        }
    }
    pos = start;
    return false;
}

// Placeholders for ASS_ST, IF_ST, FOR_ST
bool ASS_ST() { return false; }
bool IF_ST()  { return false; }
bool FOR_ST() { return false; }

bool PARAM_LIST() {
    // For now, assume empty parameter list
    return true;
}

// FUNC_DEF → TYPE id ( PARAM_LIST? ) { BLOCK_ST }
bool FUNC_DEF() {
    int start = pos;
    if (TYPE() && id() && match("(")) {
        // Optionally parse PARAM_LIST
        PARAM_LIST(); // If PARAM_LIST returns false, that's okay (it's optional)
        if (match(")") && match("{") && BLOCK_ST() && match("}")) {
            return true;
        }
    }
    pos = start;
    return false;
}

bool DECLA() {
    int start = pos;
    if (TYPE() && id() && match(";")) {
        return true;
    }
    pos = start;
    return false;
}

bool VAR_LIST() {
    int start = pos;
    if (VAR()) {
        while (true) {
            int temp = pos;
            if (match(",")) {
                if (!VAR()) {
                    pos = temp;
                    break;
                }
            } else {
                break;
            }
        }
        return true;
    }
    pos = start;
    return false;
}

bool VAR() {
    int start = pos;
    if (id()) {
        if (match("[")) {
            if (pos < tokens.size() && tokens[pos].type_num == 1) { // Assume intc type_num == 1
                pos++;
                if (match("]")) {
                    if (INITIAL()) return true;
                }
            }
        } else if (INITIAL()) {
            return true;
        }
    }
    pos = start;
    return false;
}

bool INITIAL() {
    int start = pos;
    if (match("=")) {
        if (EP()) return true;
        pos = start;
        return false;
    }
    // ɛ (empty) is allowed
    return true;
}

// Placeholder for expression parsing
bool EP() {
    int start = pos;
    // LOGC_EP not implemented, so skip for now
    if (MATH_EP()) return true;
    pos = start;
    // str: assume type_num == 2 for string constants
    if (pos < tokens.size() && tokens[pos].type_num == 2) {
        pos++;
        return true;
    }
    pos = start;
    return false;
}

// F -> id | int | real
bool logic_primary() {
    int start = pos;
    if (pos < tokens.size() && tokens[pos].type_num == 0) { // id
        pos++;
        return true;
    }
    if (REAL()) return true; // handles both real and signed int
    if (pos < tokens.size() && tokens[pos].type_num == 1) { // intc
        pos++;
        return true;
    }
    pos = start;
    return false;
}

// E -> F | F > F | F < F
bool rel_expr() {
    int start = pos;
    if (logic_primary()) {
        int temp = pos;
        if (match(">") || match("<")) {
            if (logic_primary()) return true;
            pos = temp;
        }
        return true;
    }
    pos = start;
    return false;
}

// C -> not C | E
bool not_expr() {
    int start = pos;
    if (match("not")) {
        if (not_expr()) return true;
        pos = start;
        return false;
    }
    if (rel_expr()) return true;
    pos = start;
    return false;
}

// B -> C | C and B
bool and_expr() {
    int start = pos;
    if (not_expr()) {
        int temp = pos;
        if (match("and")) {
            if (and_expr()) return true;
            pos = temp;
        }
        return true;
    }
    pos = start;
    return false;
}

// A -> B | B or A
bool or_expr() {
    int start = pos;
    if (and_expr()) {
        int temp = pos;
        if (match("or")) {
            if (or_expr()) return true;
            pos = temp;
        }
        return true;
    }
    pos = start;
    return false;
}

// LOGC_EP entry point
bool LOGC_EP() {
    return or_expr();
}

// MATH_EP → TD { ( + | - ) TD }
bool MATH_EP() {
    int start = pos;
    if (!TD()) {
        pos = start;
        return false;
    }
    while (true) {
        int temp = pos;
        if (match("+") || match("-")) {
            if (!TD()) {
                pos = temp;
                break;
            }
        } else {
            break;
        }
    }
    return true;
}

// TD → TERM { ( * | / ) TERM }
bool TD() {
    int start = pos;
    if (!TERM()) {
        pos = start;
        return false;
    }
    while (true) {
        int temp = pos;
        if (match("*") || match("/")) {
            if (!TERM()) {
                pos = temp;
                break;
            }
        } else {
            break;
        }
    }
    return true;
}

bool REAL() {
    int start = pos;
    // Optional leading + or -
    bool has_sign = false;
    if (pos < tokens.size() && (tokens[pos].value == "+" || tokens[pos].value == "-")) {
        pos++;
        has_sign = true;
    }
    // intc . [intc] or intc
    if (pos < tokens.size() && tokens[pos].type_num == 1) {
        pos++;
        // Check for decimal part
        if (pos < tokens.size() && tokens[pos].value == ".") {
            pos++;
            // Optional intc after '.'
            if (pos < tokens.size() && tokens[pos].type_num == 1) {
                pos++;
            }
            return true;
        }
        // Accept just signed intc
        if (has_sign) return true;
        // If no sign, let TERM handle plain intc
        pos = start;
        return false;
    }
    // . intc
    if (pos < tokens.size() && tokens[pos].value == ".") {
        pos++;
        if (pos < tokens.size() && tokens[pos].type_num == 1) {
            pos++;
            return true;
        }
        pos = start;
        return false;
    }
    pos = start;
    return false;
}

bool TERM() {
    int start = pos;
    if (match("(")) {
        if (MATH_EP() && match(")")) return true;
        pos = start;
        return false;
    }
    // id: type_num == 0
    if (pos < tokens.size() && tokens[pos].type_num == 0) {
        pos++;
        return true;
    }
    // real
    if (REAL()) return true;
    // intc: type_num == 1
    if (pos < tokens.size() && tokens[pos].type_num == 1) {
        pos++;
        return true;
    }
    pos = start;
    return false;
}


bool EX_DECLA() {
    int start = pos;
    if (FUNC_DEF()) return true;
    pos = start;
    if (DECLA()) return true;
    pos = start;
    return false;
}

bool START() {
    int start = pos;
    if (EX_DECLA()) {
        if (pos < tokens.size()) {
            return START();
        }
        return true;
    }
    pos = start;
    return false;
}

// Example main function
int main() {
    // TODO: Fill tokens vector from token.txt
    // tokens = ...
    read_tokens("tokens.txt");
    // Print tokens for testing
    cout << "Tokens read from file:" << endl;
    for (const auto& t : tokens) {
        cout << "<\"" << t.value << "\", " << t.type_num << ">" << endl;
    }

    if (START() && pos == tokens.size()) {
        cout << "Parsing succeeded!" << endl;
    } else {
        cout << "Parsing failed at token " << pos << endl;
    }
    return 0;
}
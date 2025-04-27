#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include "symboltable.h"
using namespace std;

struct Token {
    string value;
    string type;
    int type_num;
};

GlobalSymbolTable globalSymbolTable;
shared_ptr<FunctionSymbolTable> currentFuncSymTable = nullptr;
shared_ptr<FunctionTable> currentFuncTable = nullptr;
int current_scope = 1;
string current_type = "";
bool emit_quads = true;

int getTypeSize(const string& type) {
    if (type == "int") return 4;
    if (type == "char") return 1;
    if (type == "float") return 4;
    if (type == "double") return 8;
    if (type == "long") return 8;
    if (type == "short") return 2;
    if (type == "void") return 0; // No size for void
    return 0; // Unknown type
}

shared_ptr<SymbolTableEntry> lookup_variable(const string& name) {
    // First search function scope
    if (currentFuncSymTable) {
        for (auto& entry : currentFuncSymTable->entries) {
            if (entry->name == name) return entry;
        }
    }
    // Then search global scope
    for (auto& entry : globalSymbolTable.entries) {
        if (entry->name == name) return entry;
    }
    return nullptr;
}

string get_token_type(const Token& token) {
    if (token.type_num == 0) { // id
        auto entry = lookup_variable(token.value);
        if (!entry) return "";
        if (entry->type == "array") {
            // entry->typePointer points to ArrayTable
            auto arrTable = static_pointer_cast<ArrayTable>(entry->typePointer);
            if (arrTable) return arrTable->type;
            return "";
        }
        return entry->type;
    }
    if (token.type_num == 1) return "int";
    if (token.type_num == 2) return "string";
    if (token.type_num == 10) return "int";
    if (token.type_num == 12) return "char";
    if (token.type_num == 24) return "float";  
    if (token.type_num == 25) return "double";
    if (token.type_num == 26) return "long";
    if (token.type_num == 27) return "short";
    if (token.type_num == 28) return "void";
    return "";
}

shared_ptr<SymbolTableEntry> lookup_function(const string& name) {
    for (auto& entry : globalSymbolTable.entries) {
        if (entry->name == name && entry->type == "funct") return entry;
    }
    return nullptr;
}

struct Quadruple {
    string op;
    string arg1;
    string arg2;
    string result;
};

vector<Quadruple> quads;
string last_expr_result;
int temp_var_count = 0;
int label_count = 0;
string new_label() {
    return "L" + to_string(label_count++);
}

string new_temp() {
    return "t" + to_string(temp_var_count++);
}

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

int furthest_pos = 0;
int pos = 0;

void update_furthest() {
    if (pos > furthest_pos) furthest_pos = pos;
}

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
bool STATM();
bool RETURN_ST();
bool WHILE_ST();
bool ASS_ST();
bool IF_ST();
bool FOR_ST();
bool LVALUE();

bool match(const string& val) {
    update_furthest();
    if (pos < tokens.size() && tokens[pos].value == val) {
        pos++;
        return true;
    }
    return false;
}

bool TYPE() {
    update_furthest();
    // Assume type_num for types is 10, 12, 15, 16, 17, 18, 19 (from your keywords)
    if (pos < tokens.size() && (
        tokens[pos].type_num == 10 || // int
        tokens[pos].type_num == 12 || // char
        tokens[pos].type_num == 24 || // float
        tokens[pos].type_num == 25 || // double
        tokens[pos].type_num == 26 || // long
        tokens[pos].type_num == 27 || // short
        tokens[pos].type_num == 28    // void
    )) {
        pos++;
        return true;
    }
    return false;
}

bool id() {
    update_furthest();
    // Assume type_num == 0 for identifiers
    if (pos < tokens.size() && tokens[pos].type_num == 0) {
        pos++;
        return true;
    }
    return false;
}

bool BLOCK_ST() {
    update_furthest();
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
    update_furthest();
    int start = pos;
    if (DECLA()) return true;
    pos = start;
    if (ASS_ST() && match(";")) return true;
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
    update_furthest();
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
    update_furthest();
    int start = pos;
    if (match("while")) {
        if (match("(")) {
            string begin_label = new_label();
            string true_label = new_label();
            string false_label = new_label();

            quads.push_back({"label", "", "", begin_label}); // L0:

            if (LOGC_EP() && match(")")) {
                string cond_result = last_expr_result;
                quads.push_back({"if", cond_result, "", true_label}); // (if, t0, , L1)
                quads.push_back({"goto", "", "", false_label});       // (goto, , , L2)

                if (match("{")) {
                    quads.push_back({"label", "", "", true_label});   // L1:
                    if (BLOCK_ST() && match("}")) {
                        quads.push_back({"goto", "", "", begin_label}); // (goto, , , L0)
                        quads.push_back({"label", "", "", false_label}); // L2
                        return true;
                    }
                } else if (STATM()) {
                    quads.push_back({"label", "", "", true_label});
                    quads.push_back({"goto", "", "", begin_label});
                    quads.push_back({"label", "", "", false_label});
                    return true;
                }
            }
        }
    }
    pos = start;
    return false;
}

bool LVALUE() {
    update_furthest();
    int start = pos;
    if (id()) {
        if (match("[")) {
            if (EP() && match("]")) {
                return true;
            }
            pos = start;
            return false;
        }
        return true;
    }
    pos = start;
    return false;
}

// Placeholders for ASS_ST, IF_ST, FOR_ST
bool ASS_ST() {
    update_furthest();
    int start = pos;
    int lvalue_pos = pos;
    if (LVALUE() && match("=")) {
        // Get left-hand side variable name
        string lvalue_name = tokens[lvalue_pos].value;
        auto lentry = lookup_variable(lvalue_name);
        string ltype = lentry ? lentry->type : "";

        int ep_pos = pos;
        if (EP()) {
            // For simplicity, let's check if the right side is a variable or constant
            string rtype = "";
            if (ep_pos < tokens.size()) {
                if (tokens[ep_pos].type_num == 0) { // id
                    auto rentry = lookup_variable(tokens[ep_pos].value);
                    rtype = rentry ? rentry->type : "";
                } else if (tokens[ep_pos].type_num == 1) { // intc
                    rtype = "int";
                } else if (tokens[ep_pos].type_num == 2) { // string
                    rtype = "string";
                }
                // Add more cases as needed
            }
            // Type check
            if (!ltype.empty() && !rtype.empty() && ltype != rtype) {
                cout << "Type error: cannot assign " << rtype << " to " << ltype << " at token " << lvalue_pos << endl;
                return false;
            }
            quads.push_back({"=", last_expr_result, "", lvalue_name});
            return true;
        }
    }
    pos = start;
    return false;
}
bool IF_ST() {
    update_furthest();
    int start = pos;
    if (match("if")) {
        if (match("(")) {
            if (LOGC_EP() && match(")")) {
                // Accept either a block or a single statement
                if (match("{")) {
                    if (BLOCK_ST() && match("}")) {
                        // ELSE_PART
                        int temp = pos;
                        if (match("else")) {
                            if (match("{")) {
                                if (BLOCK_ST() && match("}")) {
                                    return true;
                                } else {
                                    pos = temp;
                                    return true;
                                }
                            } else if (STATM()) {
                                return true;
                            } else {
                                pos = temp;
                                return true;
                            }
                        }
                        return true;
                    }
                } else if (STATM()) {
                    // ELSE_PART
                    int temp = pos;
                    if (match("else")) {
                        if (match("{")) {
                            if (BLOCK_ST() && match("}")) {
                                return true;
                            } else {
                                pos = temp;
                                return true;
                            }
                        } else if (STATM()) {
                            return true;
                        } else {
                            pos = temp;
                            return true;
                        }
                    }
                    return true;
                }
            }
        }
    }
    pos = start;
    return false;
}

bool FOR_ST() {
    update_furthest();
    int start = pos;
    if (match("for")) {
        if (match("(")) {
            // --- Initialization ---
            int temp_init = pos;
            if (DECLA() || (ASS_ST() && match(";"))) {
                // ok
            } else {
                pos = temp_init;
                if (!match(";")) { // Only match ; if empty
                    pos = start;
                    return false;
                }
            }

            // --- Condition ---
            string cond_result;
            string begin_label = new_label();
            string true_label = new_label();
            string false_label = new_label();
            int temp_cond = pos;
            bool hasCond = false;
            if (EP()) {
                cond_result = last_expr_result;
                hasCond = true;
            } else {
                pos = temp_cond;
            }
            if (!match(";")) {
                pos = start;
                return false;
            }

            // --- Increment ---
            int inc_start = pos;
            bool hasInc = false;
            if (ASS_ST() || EP()) {
                hasInc = true;
            }
            int inc_end = pos;
            if (!match(")")) {
                pos = start;
                return false;
            }

            // --- Body ---
            quads.push_back({"label", "", "", begin_label}); // L0:

            if (hasCond) {
                quads.push_back({"if", cond_result, "", true_label}); // (if, t0, , L1)
                quads.push_back({"goto", "", "", false_label});       // (goto, , , L2)
            } else {
                // No condition means always true
                quads.push_back({"goto", "", "", true_label});
            }

            if (match("{")) {
                quads.push_back({"label", "", "", true_label}); // L1:
                if (BLOCK_ST() && match("}")) {
                    // --- Increment after body ---
                    if (hasInc) {
                        int old_pos = pos;
                        pos = inc_start;
                        ASS_ST(); // generate increment quad(s)
                        pos = old_pos;
                    }
                    quads.push_back({"goto", "", "", begin_label}); // (goto, , , L0)
                    quads.push_back({"label", "", "", false_label}); // L2
                    return true;
                }
            } else if (STATM()) {
                quads.push_back({"label", "", "", true_label});
                if (hasInc) {
                    int old_pos = pos;
                    pos = inc_start;
                    ASS_ST();
                    pos = old_pos;
                }
                quads.push_back({"goto", "", "", begin_label});
                quads.push_back({"label", "", "", false_label});
                return true;
            }
        }
    }
    pos = start;
    return false;
}
bool PARAM_LIST() {
    update_furthest();
    int start = pos;
    if (TYPE() && id()) {
        string paramType = tokens[pos - 2].value;
        string paramName = tokens[pos - 1].value;
        currentFuncTable->parameterTypes.push_back(paramType);

        auto paramEntry = make_shared<SymbolTableEntry>();
        paramEntry->name = paramName;
        paramEntry->type = paramType;
        paramEntry->typePointer = nullptr;
        paramEntry->scope = 2; // function scope
        paramEntry->size = getTypeSize(paramType);
        currentFuncTable->parameters.push_back(paramEntry);
        currentFuncSymTable->entries.push_back(paramEntry);
        while (true) {
            int temp = pos;
            if (match(",")) {
                if (!(TYPE() && id())) {
                    pos = start; // Fail the whole PARAM_LIST if a parameter is invalid after comma
                    return false;
                }
                paramType = tokens[pos - 2].value;
                paramName = tokens[pos - 1].value;
                currentFuncTable->parameterTypes.push_back(paramType);

                paramEntry = make_shared<SymbolTableEntry>();
                paramEntry->name = paramName;
                paramEntry->type = paramType;
                paramEntry->typePointer = nullptr;
                paramEntry->scope = 2;
                paramEntry->size = getTypeSize(paramType);
                currentFuncTable->parameters.push_back(paramEntry);
                currentFuncSymTable->entries.push_back(paramEntry);
            } else {
                break;
            }
        }
        currentFuncTable->parameterNumber = currentFuncTable->parameterTypes.size();
        return true;
    }
    // ɛ (empty) is allowed (no parameters at all)
    pos = start;
    return true;
}

// FUNC_DEF → TYPE id ( PARAM_LIST? ) { BLOCK_ST }
bool FUNC_DEF() {
    update_furthest();
    int start = pos;
    string funcName = tokens[pos+1].value; // Store the function name
    current_type = tokens[pos].value; // Store the return type
    if (TYPE() && id() && match("(")) {
        // Optionally parse PARAM_LIST
        PARAM_LIST(); // If PARAM_LIST returns false, that's okay (it's optional)
        if (match(")") && match("{")) {
            // --- Symbol Table Construction ---
            // Create function symbol table for this function
            auto funcSymTable = make_shared<FunctionSymbolTable>();
            auto prevFuncSymTable = currentFuncSymTable;
            currentFuncSymTable = funcSymTable;

            // Create function table and symbol table entry
            auto funcTable = make_shared<FunctionTable>();
            funcTable->returnType = current_type; // Store the return type
            currentFuncTable = funcTable;


            auto funcEntry = make_shared<SymbolTableEntry>();
            funcEntry->name = funcName;
            funcEntry->type = "funct";
            funcEntry->typePointer = funcTable;
            funcEntry->scope = 1; // global
            funcEntry->size = -1; // Size is not fixed for functions
            globalSymbolTable.entries.push_back(funcEntry);

            // Now parse the function body, adding variables to funcSymTable
            bool block_ok = BLOCK_ST();

            funcTable->localVariables = funcSymTable->entries; 

            // Restore previous function symbol table
            currentFuncSymTable = prevFuncSymTable;
            currentFuncTable = nullptr;

            if (block_ok && match("}")) {
                return true;
            }
        }
    }
    pos = start;
    return false;
}

bool DECLA() {
    update_furthest();
    int start = pos;
    current_type = tokens[pos].value; // Store the type for later use
    if (TYPE() && VAR_LIST()) {
        if (match(";")) {
            return true;
        }
    }
    pos = start;
    return false;
}

bool VAR_LIST() {
    update_furthest();
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
    update_furthest();
    int start = pos;
    if (id()) {
        string varName = tokens[pos - 1].value; // Store the variable name
        if (match("[")) {
            if (pos < tokens.size() && (tokens[pos].type_num == 1 || tokens[pos].type_num == 0)) { // intc or id
                int arrLen = stoi(tokens[pos].value);
                pos++;
                if (match("]")) {
                    if (INITIAL()) {
                        auto arrTable = make_shared<ArrayTable>();
                        arrTable->type = current_type;
                        arrTable->length = arrLen;
                        arrTable->size = arrLen * getTypeSize(current_type);
                        auto arrEntry = make_shared<SymbolTableEntry>();
                        arrEntry->name = varName;
                        arrEntry->type = "array";
                        arrEntry->typePointer = arrTable;
                        arrEntry->scope = current_scope;
                        arrEntry->size = arrTable->size;
                        if (currentFuncSymTable) {
                            currentFuncSymTable->entries.push_back(arrEntry);
                        } else {
                            globalSymbolTable.entries.push_back(arrEntry);
                        }
                        return true;
                    }
                }
            }
        } else if (INITIAL()) {
            auto entry = make_shared<SymbolTableEntry>();
            entry->name = varName;
            entry->type = current_type;
            entry->typePointer = nullptr; // No pointer for basic types
            entry->scope = current_scope;
            entry->size = getTypeSize(current_type);
            if (currentFuncSymTable) {
                currentFuncSymTable->entries.push_back(entry);
            } else {
                globalSymbolTable.entries.push_back(entry);
            }
            if (tokens[start+1].value == "=") {
                // The INITIAL() call would have parsed EP() and set last_expr_result
                quads.push_back({"=", last_expr_result, "", varName});
            } else {
                quads.push_back({"=", "0", "", varName}); // Initialize to 0 if no initializer
            }
            return true;
        }
    }
    pos = start;
    return false;
}

bool INITIAL() {
    update_furthest();
    int start = pos;
    if (match("=")) {
        if (EP()) return true;
        pos = start;
        return false;
    }
    // ɛ (empty) is allowed
    pos = start;
    return true;
}

bool POSTFIX_OP() {
    update_furthest();
    int start = pos;
    if (match("++") || match("--")) {
        return true;
    }
    // ε (empty)
    return true;
}

// Placeholder for expression parsing
bool EP() {
    update_furthest();
    int start = pos;
    // LOGC_EP 
    if (LOGC_EP()) return true;
    pos = start;
    // MATH_EP
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
    update_furthest();
    int start = pos;
    if (LVALUE()) {
        last_expr_result = tokens[pos - 1].value; // <-- Set last_expr_result for id
        POSTFIX_OP();
        return true;
    }
    if (REAL()) {
        last_expr_result = tokens[pos - 1].value; // <-- Set last_expr_result for real
        return true;
    }
    if (pos < tokens.size() && tokens[pos].type_num == 1) { // intc
        last_expr_result = tokens[pos].value; // <-- Set last_expr_result for intc
        pos++;
        return true;
    }
    pos = start;
    return false;
}

// E -> F > F | F < F | F >= F | F <= F
bool rel_expr() {
    update_furthest();
    int start = pos;
    int left_pos = pos;
    if (logic_primary()) {
        string left_result = last_expr_result;
        int temp = pos;
        string op = tokens[pos].value;
        if (match(">") || match("<") || match(">=") || match("<=") || match("!=")) {
            int right_pos = pos;
            if (logic_primary()) {
                string right_result = last_expr_result;
                string left_type = get_token_type(tokens[left_pos]);
                string right_type = get_token_type(tokens[right_pos]);
                if ((left_type != "int" && left_type != "float" && left_type != "double") ||
                    (right_type != "int" && right_type != "float" && right_type != "double")) {
                    cout << "Type error in relational expression at token " << right_pos << endl;
                }
                string temp_var = new_temp();
                quads.push_back({op, left_result, right_result, temp_var});
                last_expr_result = temp_var;
                return true;
            }
            pos = temp;
        }
    }
    pos = start;
    return false;
}

// C -> not C | E
bool not_expr() {
    update_furthest();
    int start = pos;
    if (match("!")) {
        if (not_expr()) {
            string temp_var = new_temp();
            quads.push_back({"!", last_expr_result, "", temp_var});
            last_expr_result = temp_var;
            return true;
        }
        pos = start;
        return false;
    }
    if (rel_expr()) return true;
    pos = start;
    return false;
}

// B -> C | C and B
bool and_expr() {
    update_furthest();
    int start = pos;
    int left_pos = pos;
    if (not_expr()) {
        string left_result = last_expr_result;
        int temp = pos;
        if (match("&&")) {
            int right_pos = pos;
            if (and_expr()) {
                string right_result = last_expr_result;
                string left_type = get_token_type(tokens[left_pos]);
                string right_type = get_token_type(tokens[right_pos]);
                if (left_type != "int" || right_type != "int") {
                    cout << "Type error in logical AND at token " << right_pos << endl;
                }
                string temp_var = new_temp();
                quads.push_back({"&&", left_result, right_result, temp_var});
                last_expr_result = temp_var;
                return true;
            }
            pos = temp;
        }
        last_expr_result = left_result;
        return true;
    }
    pos = start;
    return false;
}

// A -> B | B or A
bool or_expr() {
    update_furthest();
    int start = pos;
    int left_pos = pos;
    if (and_expr()) {
        string left_result = last_expr_result;
        int temp = pos;
        if (match("||")) {
            int right_pos = pos;
            if (or_expr()) {
                string right_result = last_expr_result;
                string left_type = get_token_type(tokens[left_pos]);
                string right_type = get_token_type(tokens[right_pos]);
                if (left_type != "int" || right_type != "int") {
                    cout << "Type error in logical OR at token " << right_pos << endl;
                }
                string temp_var = new_temp();
                quads.push_back({"||", left_result, right_result, temp_var});
                last_expr_result = temp_var;
                return true;
            }
            pos = temp;
        }
        last_expr_result = left_result;
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
    update_furthest();
    int start = pos;
    int left_pos = pos;
    if (!TD()) {
        pos = start;
        return false;
    }
    string left_result = last_expr_result;
    string left_type = get_token_type(tokens[left_pos]); 
    while (true) {
        int temp = pos;
        string op  = tokens[pos].value;
        if (match("+") || match("-")) {
            int right_pos = pos;
            if (!TD()) {
                pos = temp;
                break;
            }
            string right_result = last_expr_result;
            string right_type = get_token_type(tokens[right_pos]); 
            if ((left_type != "int" && left_type != "float" && left_type != "double" && left_type != "char") ||
                (right_type != "int" && right_type != "float" && right_type != "double" && right_type != "char")) {
                cout << "Type error in math expression at token " << right_pos << tokens[right_pos].value << endl;
            }
            if (left_type == "double" || right_type == "double")
                left_type = "double";
            else if (left_type == "float" || right_type == "float")
                left_type = "float";
            else
                left_type = "int";
            string temp_var = new_temp();
            quads.push_back({op, left_result, right_result, temp_var});
            last_expr_result = temp_var; // Update the result to the new temporary variable
            left_result = temp_var; // Update left_result for the next iteration
        } else {
            break;
        }
    }
    return true;
}

// TD → TERM { ( * | / ) TERM }
bool TD() {
    update_furthest();
    int start = pos;
    int left_pos = pos;
    if (!TERM()) {
        pos = start;
        return false;
    }
    string left_result = last_expr_result;
    string left_type = get_token_type(tokens[left_pos]); // Store the left operand type
    while (true) {
        int temp = pos;
        string op  = tokens[pos].value;
        if (match("*") || match("/")) {
            int right_pos = pos;
            if (!TERM()) {
                pos = temp;
                break;
            }
            string right_result = last_expr_result;
            string right_type = get_token_type(tokens[right_pos]); // Store the right operand type
            if ((left_type != "int" && left_type != "float" && left_type != "double" && left_type != "char") ||
                (right_type != "int" && right_type != "float" && right_type != "double" && right_type != "char")) {
                cout << "Type error in math expression at token " << right_pos << endl;
            }
            if (left_type == "double" || right_type == "double")
                left_type = "double";
            else if (left_type == "float" || right_type == "float")
                left_type = "float";
            else
                left_type = "int";
            string temp_var = new_temp();
            quads.push_back({op, left_result, right_result, temp_var});
            last_expr_result = temp_var; // Update the result to the new temporary variable
            left_result = temp_var; // Update left_result for the next iteration
        } else {
            break;
        }
    }
    return true;
}

bool REAL() {
    update_furthest();
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
    update_furthest();
    int start = pos;
    if (match("(")) {
        if (MATH_EP() && match(")")) return true;
        pos = start;
        return false;
    }
    // id: type_num == 0
    if (LVALUE()) {
        // Check for optional POSTFIX_OP
        last_expr_result = tokens[pos - 1].value; // Store the variable name
        POSTFIX_OP();
        return true;
    }
    // real
    if (REAL()) {
        last_expr_result = tokens[pos - 1].value; // Store the real number
        return true;
    }
    // intc: type_num == 1
    if (pos < tokens.size() && tokens[pos].type_num == 1) {
        last_expr_result = tokens[pos].value; // Store the integer constant
        pos++;
        return true;
    }
    pos = start;
    return false;
}


bool EX_DECLA() {
    update_furthest();
    int start = pos;
    if (FUNC_DEF()) return true;
    pos = start;
    if (DECLA()) return true;
    pos = start;
    return false;
}

bool START() {    
    update_furthest();
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

int main() {
    read_tokens("tokens.txt");

    if (START() && pos == tokens.size()) {
        cout << "Parsing succeeded!" << endl;
        for (const auto& q : quads) {
            cout << "(" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", " << q.result << ")" << endl;
        }
    } else {
        cout << "Parsing failed at token " << furthest_pos;
        if (furthest_pos < tokens.size()) {
            cout << " (value: \"" << tokens[furthest_pos].value << "\")";
        } else {
            cout << " (end of input)";
        }
        cout << endl;
    }
    return 0;
}
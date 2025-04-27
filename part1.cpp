#include <iostream>
#include <fstream>
#include <string>
#include <cctype>

using namespace std;

const int KEYNUM = 20;
const int OPNUM = 16;
const int PUNCNUM = 8;
string keywords[KEYNUM] = {"int", "const", "char", "for", "if", 
                           "else", "return", "while", "do", "break",
                            "continue", "switch", "case", "default",
                            "float", "double", "long", "short", "void"};

string operators[OPNUM] = {"=", "==", ">=", "<=", ">", 
                           "<", "+", "-", "*", "/",
                            "++", "--", "&&", "||", "!",
                            "!="};

string punctuation[PUNCNUM] = {"{", "}", ",", ";", "(",
                               ")", "[", "]"};

struct Token {
    string value;
    string type;
    int type_num;
};

bool isKeyword(const string& s, int& idx) {
    for (int i = 0; i < KEYNUM; ++i) {
        if (s == keywords[i]) {
            idx = i;
            return true;
        }
    }
    return false;
}

bool isOperator(const string& s, int& idx) {
    for (int i = 0; i < OPNUM; ++i) {
        if (s == operators[i]) {
            idx = i;
            return true;
        }
    }
    return false;
}

bool isPunctuation(const string& s, int& idx) {
    for (int i = 0; i < PUNCNUM; ++i) {
        if (s == punctuation[i]) {
            idx = i;
            return true;
        }
    }
    return false;
}

Token processNumber(ifstream& fin, char first) {
    string num(1, first);
    char c;
    while (fin.get(c) && isdigit(c)) num += c;
    if (fin && !isdigit(c)) fin.unget();
    return {num, "integer constant", 1};
}

Token processString(ifstream& fin) {
    string str;
    char c;
    while (fin.get(c)) {
        if (c == '"') break;
        str += c;
    }
    return {str, "string", 2};
}

Token processIdOrKeyword(ifstream& fin, char first) {
    string word(1, first);
    char c;
    while (fin.get(c) && (isalnum(c) || c == '_')) word += c;
    if (fin && !(isalnum(c) || c == '_')) fin.unget();
    int idx;
    if (isKeyword(word, idx))
        return {word, "keyword", idx + 10};
    else
        return {word, "identifier", 0};
}

Token processOperatorOrPunc(ifstream& fin, char first) {
    string op(1, first);
    char c;
    fin.get(c);
    if (fin) {
        op += c;
        int idx;
        if (isOperator(op, idx))
            return {op, "operator", idx + 30};
        if (isPunctuation(op, idx))
            return {op, "punctuation", idx + 50};
        op.pop_back();
        fin.unget();
    }
    int idx;
    if (isOperator(op, idx))
        return {op, "operator", idx + 30};
    if (isPunctuation(op, idx))
        return {op, "punctuation", idx + 50};
    return {op, "unknown", -1};
}

int main() {
    ifstream fin("input.txt");
    ofstream fout("tokens.txt");
    char c;
    while (fin.get(c)) {
        if (isspace(c)) continue;
        if (isdigit(c)) {
            Token t = processNumber(fin, c);
            fout << "<\"" << t.value << "\", " << t.type_num << ">, ";
            cout << "<\"" << t.value << "\", " << t.type << ">, ";
        } else if (c == '"') {
            Token t = processString(fin);
            fout << "<\"" << t.value << "\", 2>, ";
            cout << "<\"" << t.value << "\", string>, ";
        } else if (isalpha(c) || c == '_') {
            Token t = processIdOrKeyword(fin, c);
            fout << "<\"" << t.value << "\", " << t.type_num << ">, ";
            cout << "<\"" << t.value << "\", " << t.type << ">, ";
        } else {
            Token t = processOperatorOrPunc(fin, c);
            if (t.type_num != -1) {
                fout << "<\"" << t.value << "\", " << t.type_num << ">, ";
                cout << "<\"" << t.value << "\", " << t.type << ">, ";
            }
        }
    }
    fin.close();
    fout.close();
    return 0;
}
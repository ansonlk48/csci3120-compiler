#pragma once
#include <string>
#include <vector>
#include <memory>
using namespace std;

// Forward declarations
struct ArrayTable;
struct FunctionTable;
struct StructTable;
struct SymbolTableEntry;

// Type pointer can point to one of these
using TypePointer = shared_ptr<void>;

// Symbol Table Entry
struct SymbolTableEntry {
    string name;
    string type;
    shared_ptr<void> typePointer; // Points to ArrayTable, FunctionTable, StructTable, or nullptr for base types
    int scope;
    int size;
};

struct StructTable {
    string name;
    string type; // STRUCT
    shared_ptr<void> typePointer; // For nested structs, else nullptr
    int offset;
    vector<SymbolTableEntry> members;
};

struct ArrayTable {
    string type; // Base type of array elements
    shared_ptr<void> typePointer; // For arrays of arrays, else nullptr
    int length;
    int size;
};

struct FunctionTable {
    string returnType;
    vector<string> parameterTypes;
    int parameterNumber;
    vector<shared_ptr<SymbolTableEntry>> parameters; // Pointers to parameter entries in function symbol table
    vector<shared_ptr<SymbolTableEntry>> localVariables; // Pointers to local variable entries
};

struct FunctionSymbolTable {
    vector<shared_ptr<SymbolTableEntry>> entries;
};

struct GlobalSymbolTable {
    vector<shared_ptr<SymbolTableEntry>> entries;
};
// Simple decompiler to examine generated opcodes in TorqueScript
//   (e.g. to identify potential performance issues)
// 
// NOTE
//   Only decompiles bytecode which has just been compiled, i.e.
//   this is not intended to decompile .dso's.
// 
// Refer to LICENSE for license details.
//

static const char *evalStringReference(CompilerStringTable::Entry *list, U32 offset, U32 *idx, bool *isTag)
{
    CompilerStringTable::Entry *walk = list;
    U32 count=0;
    while (walk != NULL) {
        if (walk->start == offset) {
            *isTag = walk->tag;
            *idx = count;
            return walk->string;
        }
        walk = walk->next;
        count++;
    }
}

#define PRINTSTR(ip) printStringReference(idents.list, strings.list, functions.list, code[ip], ip, inFunc)
#define PRINTFLT(idx) printFloatReference(iFloats, iFuncfloats, idx)
#define PRINTFUNC(argc) printFuncArgs(idents.list, strings.list, functions.list, argc, code+6, ip+6)


static char psBuffer[4096];

static const char *printStringReference(CompilerIdentTable::Entry *ident, 
                                        CompilerStringTable::Entry *global,  
                                        CompilerStringTable::Entry *local, 
                                        U32 idx, U32 ip, bool inFunc)
{
    bool isTag;
    bool isFunc = true;
    const char *str = NULL;
    U32 count = 0;
    
    // First, look in the ident table for ip
    CompilerIdentTable::Entry *iwalk = ident;
    CompilerIdentTable::Entry *parent = NULL;
    while (iwalk != NULL) {
        if (iwalk->ip == ip) {
            break;
        }
        
        if (iwalk->nextIdent) {
            if (parent == NULL)
                parent = iwalk;
            iwalk = iwalk->nextIdent;
        } else if (parent) {
            iwalk = parent->next;
            parent = NULL;
        } else
            iwalk = iwalk->next;
    }
    
    if (iwalk != NULL) {
        // Found the string
        isFunc = false;
        str = evalStringReference(global, iwalk->offset, &count, &isTag);
    }
    
    if (str == NULL && inFunc) {
        // Must be in the local table
        str = evalStringReference(local, idx, &count, &isTag);
        isFunc = (str != NULL);
    }
    
    if (str == NULL && !inFunc) {
        // Only thing for it: a global!
        str = evalStringReference(global, idx, &count, &isTag);
        isFunc = false;
    }
    
    if (str == NULL) {
        // Nowhere!
        dSprintf(psBuffer, 4096, "NULL[&%u]", idx);
    } else {
        if (isTag) {
            if (isFunc)
                dSprintf(psBuffer, 4096, "L'%s'@%u", str, count, idx);
            else
                dSprintf(psBuffer, 4096, "'%s'@%u", str, count, idx);
        } else {
            if (isFunc)
                dSprintf(psBuffer, 4096, "L\"%s\"@%u", str, count, idx);
            else
                dSprintf(psBuffer, 4096, "\"%s\"@%u", str, count, idx);
        }
    }
    
    return psBuffer;
}

static char pfBuffer[4096];
static const char *printFloatReference(F64 *global, F64 *local, U32 idx)
{
    dSprintf(pfBuffer, 4096, "(%f OR L%f)@%u", global[idx], local[idx], idx);
    return pfBuffer;
}

static char pcBuffer[4096];
static const char *printCallType(U32 id)
{
    switch (id)
    {
        case FuncCallExprNode::FunctionCall:
            dSprintf(pcBuffer, 4096, "FUNCTION");
            break;
        case FuncCallExprNode::MethodCall:
            dSprintf(pcBuffer, 4096, "METHOD");
            break;
        case FuncCallExprNode::ParentCall:
            dSprintf(pcBuffer, 4096, "PARENT");
            break;
        default:
            dSprintf(pcBuffer, 4096, "UNKNOWN");
    }
    
    return pcBuffer;
}

static char pfcBuffer[4096];
static const char *printFuncArgs(CompilerIdentTable::Entry *ident, 
                                 CompilerStringTable::Entry *global,  
                                 CompilerStringTable::Entry *local,
                                 U32 argc, U32 *argv, U32 ip)
{
    pfcBuffer[0] = '\0';
    
    U32 i = 0;
    for (i=0; i<argc; i++) {
        dStrcat(pfcBuffer, printStringReference(ident, global, local, argv[i], ip+i, false));
        if (i != argc-1)
            dStrcat(pfcBuffer, ", ");
    }
    
    return pfcBuffer;
}

void CodeBlock::decompile()
{
    CompilerStringTable strings = getGlobalStringTable();
    CompilerStringTable functions = getFunctionStringTable();
    CompilerFloatTable floats = getGlobalFloatTable();
    CompilerFloatTable funcfloats = getFunctionFloatTable();
    CompilerIdentTable idents = getIdentTable();
    
    Con::printf("Decompiling block %s...\n--------------------\n", getCurrentCodeBlockName());
    Con::printf("Global strings...");
    
    U32 count = 0;
    CompilerStringTable::Entry *stentry = strings.list;
    while (stentry != NULL) {
        if (stentry->tag)
            Con::printf("%5u '%s'", count, stentry->string);
        else
            Con::printf("%5u \"%s\"", count, stentry->string);
        stentry = stentry->next;
        count++;
    }
    
    Con::printf("\nLocal strings...");
    count = 0;
    stentry = functions.list;
    while (stentry != NULL) {
        if (stentry->tag)
            Con::printf("%5u '%s'", count, stentry->string);
        else
            Con::printf("%5u \"%s\"", count, stentry->string);
        stentry = stentry->next;
        count++;
    }
    
    Con::printf("\nGlobal floats...");
    count = 0;
    CompilerFloatTable::Entry *fentry = floats.list;
    while (fentry != NULL) {
        Con::printf("%5u %f", count++, fentry->val);
        fentry = fentry->next;
    }
    
    Con::printf("\nLocal floats...");
    count = 0;
    fentry = funcfloats.list;
    while (fentry != NULL) {
        Con::printf("%5u %f", count++, fentry->val);
        fentry = fentry->next;
    }
    
    // NOTE:
    // CompilerStringTable -> Offset
    // CompilerFloatTable -> Index
    
    F64 *iFloats = floats.build();
    F64 *iFuncfloats = funcfloats.build();
    
    Con::printf("\nBEGIN BYTECODE\n");
    
    char fnName[4096];
    char fnNamespace[4096];
    char fnPackage[4096];
    char objParent[4096];
    char var[4096];
    char curField[4096];
    
    bool hasBody;
    bool isDataBlock;
    U32 failJump;
    U32 unknown;
    U32 endFunc = 0;
    U32 argc;
    bool inFunc = false;
    
    // Read bytecode
    U32 ip=0;
    U32 addr;
    while (ip < codeSize)
    {
        addr = ip;
        U32 opcode = code[ip++];
        inFunc = (addr < endFunc);
        switch (opcode) {
            case OP_FUNC_DECL:
                dStrcpy(fnName,      PRINTSTR(ip));
                dStrcpy(fnNamespace, PRINTSTR(ip+1));
                dStrcpy(fnPackage,   PRINTSTR(ip+2));
                hasBody      = bool(code[ip+3]); //stmts
                endFunc      = code[ip+4];       // start + endOffset
                argc         = code[ip+5];       // argc
                
                inFunc = true;
                if (argc > 40) {
                    Con::printf("INVALID CODE BLOCK\n");
                    break;
                }
                Con::printf("%5u FUNCTION %s, %s, %s, %s, $%u, (%s)", 
                            addr, 
                            fnName, fnNamespace, fnPackage, 
                            hasBody ? "true" : "false", 
                            code[ip+4],
                            PRINTFUNC(argc));
                
                ip+= 6 + argc;
                break;
                
            case OP_CREATE_OBJECT:
            {
                // Read some useful info.
                dStrcpy(objParent, PRINTSTR(ip));
                bool isDataBlock =     code[ip + 1];
                failJump         =     code[ip + 2];
                
                if (isDataBlock)
                    Con::printf("%5u CREATE_OBJECT DATABLOCK %s, $%u", addr, objParent, failJump);
                else
                    Con::printf("%5u CREATE_OBJECT OBJECT %s, $%u", addr, objParent, failJump);
                
                ip += 3;
                break;
            }
                
            case OP_ADD_OBJECT:
            {
                bool placeAtRoot = code[ip++];
                
                Con::printf("%5u ADD_OBJECT %s", addr, placeAtRoot ? "true" : "false");
                
                break;
            }
                
            case OP_END_OBJECT:
            {
                bool placeAtRoot = code[ip++];
                
                Con::printf("%5u END_OBJECT %s", addr, placeAtRoot ? "true" : "false");
                
                break;
            }
                
            case OP_JMPIFFNOT:
                Con::printf("%5u JMPIFFNOT $%u", addr, code[ip++]);
                break;
            case OP_JMPIFNOT:
                Con::printf("%5u JMPIFNOT $%u", addr, code[ip++]);
                break;
            case OP_JMPIFF:
                Con::printf("%5u JMPIFF $%u", addr, code[ip++]);
                break;
            case OP_JMPIF:
                Con::printf("%5u JMPIF $%u", addr, code[ip++]);
                break;
            case OP_JMPIFNOT_NP:
                Con::printf("%5u JMPIFNOT_NP $%u", addr, code[ip++]);
                break;
            case OP_JMPIF_NP:
                Con::printf("%5u JMPIF_NP $%u", addr, code[ip++]);
                break;
            case OP_JMP:
                Con::printf("%5u JMP $%u", addr, code[ip++]);
                break;
            case OP_RETURN:
                Con::printf("%5u RETURN", addr);
                break;
            case OP_CMPEQ:
                Con::printf("%5u CMPEQ", addr);
                break;
                
            case OP_CMPGR:
                Con::printf("%5u CMPGR", addr);
                break;
                
            case OP_CMPGE:
                Con::printf("%5u CMPGE", addr);
                break;
                
            case OP_CMPLT:
                Con::printf("%5u CMPLT", addr);
                break;
                
            case OP_CMPLE:
                Con::printf("%5u CMPLE", addr);
                break;
                
            case OP_CMPNE:
                Con::printf("%5u CMPNE", addr);
                break;
                
            case OP_XOR:
                Con::printf("%5u XOR", addr);
                break;
                
            case OP_MOD:
                Con::printf("%5u MOD", addr);
                break;
                
            case OP_BITAND:
                Con::printf("%5u BITAND", addr);
                break;
                
            case OP_BITOR:
                Con::printf("%5u BITOR", addr);
                break;
                
            case OP_NOT:
                Con::printf("%5u NOT", addr);
                break;
                
            case OP_NOTF:
                Con::printf("%5u NOTF", addr);
                break;
                
            case OP_ONESCOMPLEMENT:
                Con::printf("%5u ONESCOMPLEMENT", addr);
                break;
                
            case OP_SHR:
                Con::printf("%5u SHR", addr);
                break;
                
            case OP_SHL:
                Con::printf("%5u SHL", addr);
                break;
                
            case OP_AND:
                Con::printf("%5u AND", addr);
                break;
                
            case OP_OR:
                Con::printf("%5u OR", addr);
                break;
                
            case OP_ADD:
                Con::printf("%5u ADD", addr);
                break;
                
            case OP_SUB:
                Con::printf("%5u SUB", addr);
                break;
                
            case OP_MUL:
                Con::printf("%5u MUL", addr);
                break;
            case OP_DIV:
                Con::printf("%5u DIV", addr);
                break;
            case OP_NEG:
                Con::printf("%5u NEG", addr);
                break;
                
            case OP_SETCURVAR:
                dStrcpy(var, PRINTSTR(ip));
                ip++;
                Con::printf("%5u SETCURVAR %s", addr, var);
                break;
                
            case OP_SETCURVAR_CREATE:
                dStrcpy(var, PRINTSTR(ip));
                ip++;
                Con::printf("%5u SETCURVAR_CREATE %s", addr, var);
                break;
                
            case OP_SETCURVAR_ARRAY:
                Con::printf("%5u SETCURVAR_ARRAY", addr);
                break;
                
            case OP_SETCURVAR_ARRAY_CREATE:
                Con::printf("%5u SETCURVAR_ARRAY_CREATE", addr);
                break;
                
            case OP_LOADVAR_UINT:
                Con::printf("%5u LOADVAR_UINT", addr);
                break;
                
            case OP_LOADVAR_FLT:
                Con::printf("%5u LOADVAR_FLT", addr);
                break;
                
            case OP_LOADVAR_STR:
                Con::printf("%5u LOADVAR_STR", addr);
                break;
                
            case OP_SAVEVAR_UINT:
                Con::printf("%5u SAVEVAR_UINT", addr);
                break;
                
            case OP_SAVEVAR_FLT:
                Con::printf("%5u SAVEVAR_FLT", addr);
                break;
                
            case OP_SAVEVAR_STR:
                Con::printf("%5u SAVEVAR_STR", addr);
                break;
                
            case OP_SETCUROBJECT:
                Con::printf("%5u SETCUROBJECT", addr);
                break;
                
            case OP_SETCUROBJECT_NEW:
                Con::printf("%5u SETCUROBJECT_NEW", addr);
                break;
                
            case OP_SETCURFIELD:
                dStrcpy(curField, PRINTSTR(ip));
                Con::printf("%5u SETCURFIELD %s", addr, curField);
                ip++;
                break;
                
            case OP_SETCURFIELD_ARRAY:
                Con::printf("%5u SETCURFIELD_ARRAY", addr);
                break;
                
            case OP_LOADFIELD_UINT:
                Con::printf("%5u LOADFIELD_UINT", addr);
                break;
                
            case OP_LOADFIELD_FLT:
                Con::printf("%5u LOADFIELD_FLT", addr);
                break;
                
            case OP_LOADFIELD_STR:
                Con::printf("%5u LOADFIELD_UINT", addr);
                break;
                
            case OP_SAVEFIELD_UINT:
                Con::printf("%5u SAVEFIELD_UINT", addr);
                break;
                
            case OP_SAVEFIELD_FLT:
                Con::printf("%5u SAVEFIELD_FLT", addr);
                break;
                
            case OP_SAVEFIELD_STR:
                Con::printf("%5u SAVEFIELD_STR", addr);
                break;
                
            case OP_STR_TO_UINT:
                Con::printf("%5u STR_TO_UINT", addr);
                break;
                
            case OP_STR_TO_FLT:
                Con::printf("%5u STR_TO_FLT", addr);
                break;
                
            case OP_STR_TO_NONE:
                Con::printf("%5u STR_TO_NONE", addr);
                break;
                
            case OP_FLT_TO_UINT:
                Con::printf("%5u FLT_TO_UINT", addr);
                break;
                
            case OP_FLT_TO_STR:
                Con::printf("%5u FLT_TO_STR", addr);
                break;
                
            case OP_FLT_TO_NONE:
                Con::printf("%5u FLT_TO_NONE", addr);
                break;
                
            case OP_UINT_TO_FLT:
                Con::printf("%5u UINT_TO_FLT", addr);
                break;
                
            case OP_UINT_TO_STR:
                Con::printf("%5u UINT_TO_STR", addr);
                break;
                
            case OP_UINT_TO_NONE:
                Con::printf("%5u UINT_TO_NONE", addr);
                break;
                
            case OP_LOADIMMED_UINT:
                Con::printf("%5u LOADIMMED_UINT %i", addr, (S64*)code[ip++]);
                break;
                
            case OP_LOADIMMED_FLT:
                Con::printf("%5u LOADIMMED_FLT %s", addr, PRINTFLT(code[ip++]));
                break;
            case OP_TAG_TO_STR:
                Con::printf("%5u TAG_TO_STR %s", addr, PRINTSTR(ip));
                break;
            case OP_LOADIMMED_STR:
                Con::printf("%5u OP_LOADIMMED_STR %s", addr, PRINTSTR(ip));
                ip++;
                break;
                
            case OP_LOADIMMED_IDENT:
                Con::printf("%5u OP_LOADIMMED_IDENT %s", addr, PRINTSTR(ip));
                ip++;
                break;
                
            case OP_CALLFUNC_RESOLVE:
                dStrcpy(fnNamespace, PRINTSTR(ip+1));
                dStrcpy(fnName, PRINTSTR(ip));
                U32 callType = code[ip+2];
                
                Con::printf("%5u CALLFUNC_RESOLVE %s, %s, %s", addr, fnName, fnNamespace, printCallType(callType));
                ip += 3;
            break;
            case OP_CALLFUNC:
            {
                dStrcpy(fnName, PRINTSTR(ip));
                U32 unknown = code[ip+1];
                U32 callType = code[ip+2];
                
                Con::printf("%5u CALLFUNC %s, $%u, %s", addr, fnName, unknown, printCallType(callType));
                ip += 3;
                
                break;
            }
            case OP_ADVANCE_STR:
                Con::printf("%5u ADVANCE_STR", addr);
                break;
            case OP_ADVANCE_STR_APPENDCHAR:
                Con::printf("%5u ADVANCE_STR_APPENDCHAR '%c'", addr, code[ip++]);
                break;
                
            case OP_ADVANCE_STR_COMMA:
                Con::printf("%5u ADVANCE_STR_COMMA", addr);
                break;
                
            case OP_ADVANCE_STR_NUL:
                Con::printf("%5u ADVANCE_STR_NUL", addr);
                break;
                
            case OP_REWIND_STR:
                Con::printf("%5u REWIND_STR", addr);
                break;
                
            case OP_TERMINATE_REWIND_STR:
                Con::printf("%5u TERMINATE_REWIND_STR", addr);
                break;
                
            case OP_COMPARE_STR:
                Con::printf("%5u COMPARE_STR", addr);
                break;
            case OP_PUSH:
                Con::printf("%5u PUSH", addr);
                break;
                
            case OP_PUSH_FRAME:
                Con::printf("%5u PUSH_FRAME", addr);
                break;
            case OP_BREAK:
                Con::printf("%5u BREAK", addr);
            case OP_INVALID:
            default:
                Con::printf("%5u INVALID", addr);
            break;
        }
    }
    
    Con::printf("--------------------");
        
    delete [] iFuncfloats;
    delete [] iFloats;
}

bool CodeBlock::compile(const char *codeFileName, StringTableEntry fileName, const char *script)
{
   // ...
   // after getIdentTable().write(st);
    
   // Last chance to dump tables
   decompile();

   //
}

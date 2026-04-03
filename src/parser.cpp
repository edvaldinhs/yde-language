#include "../include/parser.h"
#include <memory>

extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> TheBuilder;

int CurTok;
std::map<char, int> BinopPrecedence;

int getNextToken() { return CurTok = gettok(); }

std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

static int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

// --- Logic for Primary Expressions ---
static TypeKind ParseType() {
  if (CurTok == tok_int) {
    getNextToken();
    return TypeKind::Int;
  }
  if (CurTok == tok_double) {
    getNextToken();
    return TypeKind::Double;
  }
  return TypeKind::Double;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  getNextToken();

  if (CurTok == ':') {
    getNextToken();
    TypeKind Ty = ParseType();

    std::unique_ptr<ExprAST> Init;
    if (CurTok == '=') {
      getNextToken();
      Init = ParseExpression();
    }
    return std::make_unique<VarExprAST>(IdName, Ty, std::move(Init));
  }

  if (CurTok != '(')
    return std::make_unique<VariableExprAST>(IdName);

  getNextToken();
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;
      if (CurTok != ',')
        return nullptr;
      getNextToken();
    }
  }
  getNextToken();
  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<GlobalVarAST> ParseGlobal() {
  TypeKind Ty = (CurTok == tok_int) ? TypeKind::Int : TypeKind::Double;
  getNextToken();

  if (CurTok != tok_identifier)
    return nullptr;

  std::string Name = IdentifierStr;
  getNextToken();

  double Val = 0;
  if (CurTok == '=') {
    getNextToken();
    if (CurTok == tok_number) {
      Val = NumVal;
      getNextToken();
    }
  }

  if (CurTok == ';')
    getNextToken();

  return std::make_unique<GlobalVarAST>(Name, Ty, Val);
}

static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken();
  return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseVarExpr() {
  TypeKind Ty = (CurTok == tok_int) ? TypeKind::Int : TypeKind::Double;
  getNextToken();

  if (CurTok != tok_identifier)
    return LogError("expected identifier after type");

  std::string Name = IdentifierStr;
  getNextToken();

  std::unique_ptr<ExprAST> Init;
  if (CurTok == '=') {
    getNextToken();
    Init = ParseExpression();
    if (!Init)
      return nullptr;
  }

  return std::make_unique<VarExprAST>(Name, Ty, std::move(Init));
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken();
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return nullptr;
  getNextToken();
  return V;
}

static std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken();

  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (CurTok != tok_then)
    return nullptr;
  getNextToken();

  auto Then = ParseExpression();
  if (!Then)
    return nullptr;

  if (CurTok != tok_else)
    return nullptr;
  getNextToken();

  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                     std::move(Else));
}
static std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken();

  if (CurTok != tok_identifier)
    return nullptr;
  std::string IdName = IdentifierStr;
  getNextToken();

  if (CurTok != '=')
    return nullptr;
  getNextToken();

  auto Start = ParseExpression();
  if (!Start)
    return nullptr;
  if (CurTok != ',')
    return nullptr;
  getNextToken();

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',') {
    getNextToken();
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }

  if (CurTok != tok_in)
    return nullptr;
  getNextToken();

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End),
                                      std::move(Step), std::move(Body));
}

static std::unique_ptr<ExprAST> ParseBlockExpr() {
  getNextToken();
  std::vector<std::unique_ptr<ExprAST>> Exprs;

  while (CurTok != '}' && CurTok != tok_eof) {
    auto E = ParseExpression();
    if (!E)
      return nullptr;
    Exprs.push_back(std::move(E));

    if (CurTok == ';')
      getNextToken();
  }

  if (CurTok != '}') {
    fprintf(stderr, "Expected '}' at end of block\n");
    return nullptr;
  }

  getNextToken();
  return std::make_unique<BlockExprAST>(std::move(Exprs));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default:
    return nullptr;
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case tok_if:
    return ParseIfExpr();
  case tok_for:
    return ParseForExpr();
  case tok_int:
  case tok_double:
    return ParseVarExpr();
  case '{':
    return ParseBlockExpr();
  case '(':
    return ParseParenExpr();
  }
}

// --- Logic for Binary Expressions ---
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  while (true) {
    int TokPrec = GetTokPrecedence();

    if (TokPrec < ExprPrec)
      return LHS;

    int BinOp = CurTok;
    getNextToken();

    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      // TokPrec +1 para 1 + 2 + 3 = (1 + 2) + 3
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    // Merge LHS/RHS.
    LHS =
        std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;
  return ParseBinOpRHS(0, std::move(LHS));
}

static ArgInfo ParseArgument() {
  TypeKind SelectedType = TypeKind::Double;
  std::string SelectedName;

  if (CurTok == tok_int || CurTok == tok_double) {
    SelectedType = (CurTok == tok_int) ? TypeKind::Int : TypeKind::Double;
    getNextToken();

    if (CurTok != tok_identifier)
      return {};
    SelectedName = IdentifierStr;
    getNextToken();
  } else if (CurTok == tok_identifier) {
    SelectedName = IdentifierStr;
    getNextToken();

    if (CurTok == ':') {
      getNextToken();
      if (CurTok == tok_int)
        SelectedType = TypeKind::Int;
      else if (CurTok == tok_double)
        SelectedType = TypeKind::Double;
      getNextToken();
    }
  }

  return {SelectedName, SelectedType};
}

// --- Logic for Functions/Prototypes ---
static std::unique_ptr<PrototypeAST> ParsePrototype() {
  TypeKind RetType = TypeKind::Double;
  std::string FnName;

  if (CurTok == tok_int || CurTok == tok_double) {
    RetType = (CurTok == tok_int) ? TypeKind::Int : TypeKind::Double;
    getNextToken();
  }

  if (CurTok != tok_identifier)
    return nullptr;

  FnName = IdentifierStr;
  getNextToken();

  if (CurTok != '(')
    return nullptr;

  getNextToken();

  std::vector<ArgInfo> Args;
  if (CurTok != ')') {
    while (true) {
      auto Arg = ParseArgument();
      if (Arg.Name.empty())
        return nullptr;
      Args.push_back(Arg);

      if (CurTok == ')')
        break;
      if (CurTok != ',')
        return nullptr;
      getNextToken();
    }
  }
  getNextToken();

  return std::make_unique<PrototypeAST>(FnName, std::move(Args), RetType);
}

std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto E = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    std::vector<ArgInfo> EmptyArgs;
    auto Proto = std::make_unique<PrototypeAST>(
        "__anon_expr", std::move(EmptyArgs), TypeKind::Double);
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

std::unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken();
  return ParsePrototype();
}

void SetupPrecedence() {
  // BinopPrecedence[','] = 1;
  BinopPrecedence['='] = 5;
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;
}

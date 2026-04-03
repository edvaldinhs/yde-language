#ifndef AST_H
#define AST_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <llvm/IR/Type.h>
#include <map>

#include <memory>
#include <string>
#include <vector>

enum class TypeKind { Double, Int };

struct VarType {
  TypeKind Kind;
};

struct ArgInfo {
  std::string Name;
  TypeKind Type;
};

namespace llvm {
class Value;
class Function;
} // namespace llvm

// Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual llvm::Value *codegen() = 0;
};

// Expression class for numeric literals.
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
  llvm::Value *codegen() override;
};

// Expression class for global variables.
class GlobalVarAST : public ExprAST {
  std::string Name;
  TypeKind Ty;
  double InitVal;

public:
  GlobalVarAST(const std::string &Name, TypeKind Ty, double InitVal)
      : Name(Name), Ty(Ty), InitVal(InitVal) {}
  llvm::Value *codegen() override;
};

// Expression class for creating variables
class VarExprAST : public ExprAST {
  std::string Name;
  TypeKind Ty;
  std::unique_ptr<ExprAST> Init;

public:
  VarExprAST(std::string Name, TypeKind Ty, std::unique_ptr<ExprAST> Init)
      : Name(std::move(Name)), Ty(std::move(Ty)), Init(std::move(Init)) {}
  llvm::Value *codegen() override;
};

// Expression class for referencing variables.
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
  const std::string &getName() const { return Name; }
  llvm::Value *codegen() override;
};

// Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
      : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  llvm::Value *codegen() override;
};

// Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}
  llvm::Value *codegen() override;
};

class PrototypeAST {
  std::string Name;
  std::vector<ArgInfo> Args;
  TypeKind RetType;

public:
  PrototypeAST(const std::string &Name, std::vector<ArgInfo> Args,
               TypeKind RetType)
      : Name(Name), Args(std::move(Args)), RetType(RetType) {}

  const std::string &getName() const { return Name; }
  llvm::Function *codegen();

  TypeKind getArgType(size_t i) const { return Args[i].Type; }
  TypeKind getRetType() const { return RetType; }
};

class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
  llvm::Function *codegen();
};

class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

  llvm::Value *codegen() override;
};

class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
      : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body)) {}

  llvm::Value *codegen() override;
};

class BlockExprAST : public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Expressions;

public:
  BlockExprAST(std::vector<std::unique_ptr<ExprAST>> Expressions)
      : Expressions(std::move(Expressions)) {}

  llvm::Value *codegen() override;
};

#endif // !AST_H

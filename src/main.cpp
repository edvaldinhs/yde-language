#include "../include/ast.h"
#include "../include/lexer.h"
#include "../include/parser.h"

#include "llvm/Support/raw_ostream.h"
#include <iostream>

extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> TheBuilder;

int main() {
  SetupPrecedence();

  TheContext = std::make_unique<llvm::LLVMContext>();
  TheModule = std::make_unique<llvm::Module>("MyLangJIT", *TheContext);
  TheBuilder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

  std::cout << "Ready! Type 'def' to start a function (e.g., def foo(x) x+1) "
               "or press Ctrl+D."
            << std::endl;
  std::cout << ">> ";

  getNextToken();

  while (true) {
    switch (CurTok) {
    case tok_eof:
      return 0;
    case ';':
      getNextToken();
      break;
    case tok_def: {
      if (auto FnAST = ParseDefinition()) {
        if (auto *FnIR = FnAST->codegen()) {
          std::cout << "Read function definition:" << std::endl;
          FnIR->print(llvm::errs());
          std::cerr << "\n";
        }
      } else {
        getNextToken();
      }
      break;
    }
    default:
      if (auto ExprAST = ParseExpression()) {
        if (auto *ExprIR = ExprAST->codegen()) {
          std::cout << "Read expression:" << std::endl;
          ExprIR->print(llvm::errs());
          std::cerr << "\n";
        }
      } else {
        getNextToken();
      }
      break;
    }
    std::cout << ">> ";
  }

  return 0;
}

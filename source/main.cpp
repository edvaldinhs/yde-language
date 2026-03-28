#include "../include/lexer.h"
#include <iostream>

int main() {
  std::cout
      << "Ready! Type some code (e.g., 'def foo 5') or press Ctrl+D to exit."
      << std::endl;
  std::cout << ">> ";

  while (true) {
    int tok = gettok();
    if (tok == tok_eof)
      break;

    switch (tok) {
    case tok_def:
      std::cout << "Parsed a [DEF] keyword" << std::endl;
      break;
    case tok_extern:
      std::cout << "Parsed an [EXTERN] keyword" << std::endl;
      break;
    case tok_identifier:
      std::cout << "Parsed an [IDENTIFIER]: " << IdentifierStr << std::endl;
      break;
    case tok_number:
      std::cout << "Parsed a [NUMBER]: " << NumVal << std::endl;
      break;
    default:
      std::cout << "Parsed a [CHARACTER]: " << (char)tok << std::endl;
      break;
    }
    std::cout << ">> ";
  }

  return 0;
}

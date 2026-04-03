#include "../include/lexer.h"

std::string IdentifierStr;
double NumVal;

int gettok() {
  static int LastChar = ' ';

  while (isspace(LastChar))
    LastChar = getchar();

  if (isalpha(LastChar)) {
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;

    if (IdentifierStr == "when")
      return tok_def;
    if (IdentifierStr == "forget")
      return tok_extern;
    if (IdentifierStr == "and")
      return tok_if;
    if (IdentifierStr == "you")
      return tok_then;
    if (IdentifierStr == "still")
      return tok_else;
    if (IdentifierStr == "why")
      return tok_for;
    if (IdentifierStr == "love")
      return tok_in;
    if (IdentifierStr == "me")
      return tok_int;
    if (IdentifierStr == "us")
      return tok_double;
    return tok_identifier;
  }

  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  if (LastChar == '#') {
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  if (LastChar == EOF)
    return tok_eof;

  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

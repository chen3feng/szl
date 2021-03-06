// Copyright 2010 Google Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------

#include <string>

namespace sawzall {

// Terminal symbols according to language spec. The symbols are global
// because they are used frequently in the parser and it is very tedious
// to fully qualify them every single time. Also, they are likely not to
// escape parser.cc.

// Order may become relevant!

enum Symbol {
  // errors
  SCANEOF, ILLEGAL,

  // literals
  BYTES, CHAR, INT, FINGERPRINT, TIME, FLOAT, STRING, UINT, IDENT,

  // special char sequences
  PLUS, MINUS, TIMES, DIV, MOD, BITAND, BITOR, BITXOR,
  SHL, SHR, EQL, NEQ, LSS, LEQ, GTR, GEQ, AT,
  LPAREN, RPAREN, LBRACK, RBRACK, LBRACE, RBRACE, CONDAND, CONDOR,
  BITNOT, PERIOD, COMMA, SEMICOLON, COLON, ASSIGN, LARROW, RARROW, DOLLAR, QUERY,
  INC, DEC,

  // keywords
  ALL, AND, ARRAY, BREAK, CASE, CONTINUE, DEFAULT, DO, EACH,
  ELSE, EMIT, FILE_, FOR, FORMAT, FUNCTION, IF, INCLUDE, JOB, KEYBY, MAP, MERGE,
  MILL, MILLMERGE, NOT, OF, OR, PARSEDMESSAGE, PIPELINE, PROC, PROTO, REST,
  RETURN, SKIP, SOME, STATIC, SUBMATCH, SWITCH, TABLE, TYPE, WEIGHT, WHEN,
  WHILE,

  // result is a keyword only in statement expressions.  Leave this constant
  // out of the range [FIRST_KEYWORD,LAST_KEYWORD] because it is handled in
  // the parser rather than the scanner.
  RESULT,

  FIRST_KEYWORD = ALL,
  LAST_KEYWORD = WHILE

};


// returns true if ident matches a keyword, returns false otherwise
// (may be used by protocol-compiler to disambiguate field names)
extern bool IsKeyword(const char* ident);
inline bool IsKeyword(Symbol sym) { return (sym >= ALL && sym <= WHILE); }

// for debugging (global because Symbol is global)
extern const char* Symbol2String(Symbol sym);


// Source handling

struct SourceFile {
  const char* path;  // file name passed in
  const char* dir;   // directory in which to search  for relative includes
};


// Represents a file that the Scanner reads from.
class Source {
 public:
  // Construction
  // If src is NULL, open the file(s) in files and read them in order.
  // If src is non-NULL, just use that string as the contents of the file.
  // src is used for code specified with -e on the command line and code
  // generated by the protocol compiler.
  Source(const SourceFile* files, int num_files, const char* src);
  ~Source();

  // Accessors
  const char* file_name() const  { return file_name_ != NULL ? file_name_ : files_[file_num_].path; }
  const char* source_dir() const  { return files_[file_num_].dir; }
  int line() const  { return line_; }

  // Test that the constructor succeeded
  bool IsValid() const  { return file_ != NULL || src_ != NULL; }

  // Reading
  int NextChar();
  int ReadChar();
  int LastChar() const  { return ch_; }
  int Lookahead(int n) const  { return (src_ != NULL) ? src_[n] : 0; }

  // File and line directive processing
  void SetFileLine(const char* file, int line);

  // to report file changes.  TODO: fix this unaesthetic plumbing
  void SetScanner(class Scanner* sc) { scanner_ = sc; }

  // Error reporting - failure to open file
  int error_count() const  { return error_count_; }

 private:
  const SourceFile* files_;  // array of source files
  int num_files_;  // number of source files
  int file_num_;  // which source file we're processing
  FILE* file_;
  char bytes_[UTFmax];
  int nbytes_;
  int ch_;
  const char* pos_;
  int line_;
  const char* src_;
  const char* file_name_;  // current file name as specified by a line directive
  class Scanner* scanner_;
  int error_count_;

  int ReadByte();
  void UnloadBytes(int n);
  int FileGetc();
  void OpenNextFile();
};


// Sawzall Scanner

class Scanner {
 public:
  Scanner(Proc* proc, Source* source);
  ~Scanner();

  // get the next symbol
  Symbol Scan();
  // check if next symbol is COLON (does not span files)
  bool NextSymbolIsColon() const  { return (FirstByteNextSymbol() == ':'); }

  // literal values
  // valid only if the symbol scanned is INT, FLOAT,
  // STRING, or IDENT respectively (idents are returned as strings)
  szl_int int_value() const  { return int_value_; }  // INT, FINGERPRINT, TIME
  szl_float float_value() const  { return float_value_; }  // FLOAT
  int illegal_value() const  { return illegal_value_; }  // ILLEGAL
  void negate_int_value();
  void negate_float_value();
  szl_string string_value() const  { return string_value_; }  // STRING
  int string_len() const  { return string_len_; }  // Number of bytes
  char* bytes_value() const  { return string_value_; }  // BYTES
  int bytes_len() const  { return string_len_; }  // Number of bytes

  // location information
  // valid for the symbol scanned immediately before
  // (i.e., only valid after a call of Scan())
  const char* file_name() const  { return file_name_; }
  int line() const  { return line_; }
  int offset() const  { return offset_; }
  int last_end_offset() const  { return last_end_offset_; }
  int end_offset() const  { return current_offset_; }

  // include files
  int ScanProto();
  bool IsOpenInclude(const char* file_name, int include_level);
  // add special #line marker for this file to the source
  void RegisterFile(const char* file_name);
  // insert the file source so next call to Scan() advances to its 1st character
  bool IncludeFile(const char* file_name);
  static const char* FindIncludeFile(Proc* proc, const char* file_name,
                                     const char* source_dir);

  // formatted print
  static int SzlFileLineFmt(Fmt::State*);
  static int SzlSourceLineFmt(Fmt::State*);
  static int SymbolFmt(Fmt::State*);
  const char* PrintSymbol(Symbol sym);

  // error handling
  void Error(const char* fmt, ...);
  void Errorv(bool is_warning, const char* fmt, va_list* args);
  int error_count() const;

  // raw source code for logging, available while the scanner is alive
  List<char>* source()  { return &source_; }

  // name of the source file currently being scanned
  const char* current_file_name() { return current_->file_name(); }

 private:
  Proc* proc_;

  // source
  // states_ represents the stack of nested includes, where states_[0] is the
  // top-level file and states_[include_level_ - 1] is the file that included
  // current_.
  // If states_[i] represents code generated by protocol_compiler, then
  // generated_sources_[i] will be the string that backs the Source object
  // in states_[i], so that the generated source is live as long as the Source
  // object is.
  enum { MAX_INCLUDE_LEVEL = 32 };
  Source* states_[MAX_INCLUDE_LEVEL];  // saved states dep. on include level
  string generated_proto_sources_[MAX_INCLUDE_LEVEL];
  int include_level_;

  // current scan file/position
  Source* current_;
  int current_offset_;
  int ch_;  // one unicode char look-ahead
  List<char> source_;  // the UTF-8 encoded raw source
  const char* file_name_;  // the file name for the current symbol
  int line_;  // the line number for the current symbol
  int offset_;  // the offset in source_ (in bytes) for the current symbol
  int last_end_offset_;  // offset at end of previous symbol

  // errors
  int last_error_line_;

  // literal values
  szl_int  int_value_;
  szl_float  float_value_;
  int allocated_len_;  // max. length of string_value_
  int illegal_value_;  // the illegal char in case of ILLEGAL symbol
  // The string_value is a buffer that holds a variety of things depending
  // on the type: a UTF-8 string constant, a file name, a variable name, the
  // input representation of a numeric constant, the raw (non-UTF-8) bytes
  // of a bytes constant, etc. The interpretation depends on the Symbol type.
  char* string_value_;  // expanded on demand
  int string_len_;  // string_len_ <= allocated_len_; also used for bytes

  // helpers for source collection
  void AddSourceChar(Rune ch);
  void AddSourceString(const char* s);
  void EraseToEnd(int pos);

  // helpers for idents/strings
  void StartString()  { string_len_ = 0; }
  void AddStringChar(Rune ch);
  void AddBytesChar(int ch);
  void TerminateString()  { AddStringChar('\0'); }
  void EnsureStringSpace(int n);

  // scan helpers
  void next();
  int HexChar(int quote);
  Symbol IfNextThenElse(int ch, Symbol then, Symbol else_);
  void ScanLineDirective();
  void SkipWhitespaceAndComments();
  Rune ScanEscape(int base, int num_digits, bool exact_count);
  int ScanDigits(int base, int max_digit);
  Symbol ScanNumber(bool negative, bool seen_period);
  Rune ScanUnicode();
  void ScanByteString();
  void ScanChar();
  void ScanHexByteString();
  void ScanIdent();
  void ScanString();
  void ScanTime();
  void OpenInclude(const char* file_name, const char* generated_source);
  void CloseInclude();
  Symbol ScanInclude();
  Rune ValidUnicode(Rune r);
  Rune ValidByte(Rune b);
  int FirstByteNextSymbol() const;  // limited lookahead; does not span files
};

}  // namespace sawzall

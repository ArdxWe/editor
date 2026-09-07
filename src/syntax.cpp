#include "syntax.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string_view>

namespace sdl {

namespace {

constexpr std::uint8_t kNormal = 0;
constexpr std::uint8_t kBlockComment = 1;
constexpr std::uint8_t kTripleDouble = 2;
constexpr std::uint8_t kTripleSingle = 3;
constexpr std::uint8_t kMdFence = 4;

constexpr std::string_view kCpp[] = {
    "alignas",       "alignof",     "and",
    "and_eq",        "asm",         "auto",
    "bitand",        "bitor",       "bool",
    "break",         "case",        "catch",
    "char",          "char8_t",     "char16_t",
    "char32_t",      "class",       "compl",
    "concept",       "const",       "consteval",
    "constexpr",     "constinit",   "const_cast",
    "continue",      "co_await",    "co_return",
    "co_yield",      "decltype",    "default",
    "delete",        "do",          "double",
    "dynamic_cast",  "else",        "enum",
    "explicit",      "export",      "extern",
    "false",         "float",       "for",
    "friend",        "goto",        "if",
    "inline",        "int",         "long",
    "mutable",       "namespace",   "new",
    "noexcept",      "not",         "not_eq",
    "nullptr",       "operator",    "or",
    "or_eq",         "private",     "protected",
    "public",        "register",    "reinterpret_cast",
    "requires",      "return",      "short",
    "signed",        "sizeof",      "static",
    "static_assert", "static_cast", "struct",
    "switch",        "template",    "this",
    "thread_local",  "throw",       "true",
    "try",           "typedef",     "typeid",
    "typename",      "union",       "unsigned",
    "using",         "virtual",     "void",
    "volatile",      "wchar_t",     "while",
    "xor",           "xor_eq",      "override",
    "final",
};

constexpr std::string_view kPython[] = {
    "False",  "None",   "True",    "and",      "as",       "assert", "async",
    "await",  "break",  "class",   "continue", "def",      "del",    "elif",
    "else",   "except", "finally", "for",      "from",     "global", "if",
    "import", "in",     "is",      "lambda",   "nonlocal", "not",    "or",
    "pass",   "raise",  "return",  "try",      "while",    "with",   "yield",
};

constexpr std::string_view kJs[] = {
    "async",    "await",    "break",    "case",    "catch",      "class",
    "const",    "continue", "debugger", "default", "delete",     "do",
    "else",     "export",   "extends",  "false",   "finally",    "for",
    "function", "if",       "import",   "in",      "instanceof", "let",
    "new",      "null",     "return",   "static",  "super",      "switch",
    "this",     "throw",    "true",     "try",     "typeof",     "undefined",
    "var",      "void",     "while",    "with",    "yield",      "of",
};

constexpr std::string_view kJson[] = {"false", "null", "true"};

constexpr std::string_view kRust[] = {
    "as",   "async",  "await",  "break", "const",  "continue", "crate", "dyn",
    "else", "enum",   "extern", "false", "fn",     "for",      "if",    "impl",
    "in",   "let",    "loop",   "match", "mod",    "move",     "mut",   "pub",
    "ref",  "return", "self",   "Self",  "static", "struct",   "super", "trait",
    "true", "type",   "unsafe", "use",   "where",  "while",
};

constexpr std::string_view kGo[] = {
    "break",  "case",        "chan", "const",   "continue", "default", "defer",
    "else",   "fallthrough", "for",  "func",    "go",       "goto",    "if",
    "import", "interface",   "map",  "package", "range",    "return",  "select",
    "struct", "switch",      "type", "var",     "true",     "false",   "nil",
};

constexpr std::string_view kCMake[] = {
    "if",
    "endif",
    "else",
    "elseif",
    "foreach",
    "endforeach",
    "while",
    "endwhile",
    "function",
    "endfunction",
    "macro",
    "endmacro",
    "return",
    "break",
    "continue",
    "set",
    "option",
    "include",
    "project",
    "message",
    "add_executable",
    "add_library",
    "target_link_libraries",
};

constexpr std::string_view kShell[] = {
    "if",       "then",     "else",   "elif",     "fi",     "for",
    "in",       "do",       "done",   "while",    "until",  "case",
    "esac",     "function", "return", "exit",     "export", "local",
    "readonly", "shift",    "break",  "continue", "true",   "false",
};

constexpr std::string_view kCppTypes[] = {
    "int8_t",        "int16_t",    "int32_t",      "int64_t",  "uint8_t",
    "uint16_t",      "uint32_t",   "uint64_t",     "size_t",   "ssize_t",
    "ptrdiff_t",     "intptr_t",   "uintptr_t",    "string",   "wstring",
    "u16string",     "u32string",  "string_view",  "vector",   "array",
    "deque",         "list",       "map",          "set",      "unordered_map",
    "unordered_set", "optional",   "variant",      "span",     "tuple",
    "pair",          "unique_ptr", "shared_ptr",   "weak_ptr", "function",
    "mutex",         "thread",     "ifstream",     "ofstream", "fstream",
    "istream",       "ostream",    "stringstream", "path",     "error_code",
    "byte",          "nullptr_t",
};

constexpr std::string_view kPythonTypes[] = {
    "bool", "int",   "float", "str",    "bytes",    "list",      "dict",
    "set",  "tuple", "type",  "object", "NoneType", "Exception",
};

constexpr std::string_view kJsTypes[] = {
    "Array", "Object", "String",   "Number", "Boolean", "Promise", "Map",
    "Set",   "Error",  "Function", "Date",   "RegExp",  "Symbol",  "BigInt",
};

constexpr std::string_view kRustTypes[] = {
    "i8",   "i16",  "i32", "i64",     "i128",    "isize",  "u8",
    "u16",  "u32",  "u64", "u128",    "usize",   "f32",    "f64",
    "bool", "char", "str", "String",  "Vec",     "Option", "Result",
    "Box",  "Rc",   "Arc", "HashMap", "HashSet",
};

constexpr std::string_view kGoTypes[] = {
    "bool",    "byte",    "rune",      "string",     "int",
    "int8",    "int16",   "int32",     "int64",      "uint",
    "uint8",   "uint16",  "uint32",    "uint64",     "uintptr",
    "float32", "float64", "complex64", "complex128", "error",
};

constexpr std::string_view kConstants[] = {
    "true", "false", "null", "nullptr",   "NULL", "None",
    "True", "False", "nil",  "undefined", "NaN",  "Infinity",
};

constexpr std::string_view kTypeIntroducers[] = {
    "class", "struct",    "enum",  "union", "typename",
    "using", "namespace", "def",   "fn",    "func",
    "type",  "interface", "trait", "impl",  "new",
};

constexpr std::string_view kOps3[] = {"<<=", ">>=", "<=>", "..."};
constexpr std::string_view kOps2[] = {
    "==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "::", "->", "+=", "-=",
    "*=", "/=", "%=", "&=", "|=", "^=", "++", "--", "=>", "??", "?.",
};

bool is_ident_start(unsigned char c) { return std::isalpha(c) || c == '_'; }

bool is_ident(unsigned char c) { return std::isalnum(c) || c == '_'; }

bool is_digit(unsigned char c) { return std::isdigit(c); }

template <std::size_t N>
bool contains_word(const std::string_view (&table)[N], std::string_view word) {
  return std::find(std::begin(table), std::end(table), word) != std::end(table);
}

bool is_keyword(Language lang, std::string_view word) {
  switch (lang) {
    case Language::Cpp:
      return contains_word(kCpp, word);
    case Language::Python:
      return contains_word(kPython, word);
    case Language::JavaScript:
      return contains_word(kJs, word);
    case Language::Json:
      return contains_word(kJson, word);
    case Language::Rust:
      return contains_word(kRust, word);
    case Language::Go:
      return contains_word(kGo, word);
    case Language::CMake:
      return contains_word(kCMake, word);
    case Language::Shell:
      return contains_word(kShell, word);
    default:
      return false;
  }
}

bool is_constant(std::string_view word) {
  return contains_word(kConstants, word);
}

bool is_known_type(Language lang, std::string_view word) {
  switch (lang) {
    case Language::Cpp:
      return contains_word(kCppTypes, word);
    case Language::Python:
      return contains_word(kPythonTypes, word);
    case Language::JavaScript:
      return contains_word(kJsTypes, word);
    case Language::Rust:
      return contains_word(kRustTypes, word);
    case Language::Go:
      return contains_word(kGoTypes, word);
    default:
      return false;
  }
}

bool looks_like_type(std::string_view word) {
  if (word.size() < 2 || !std::isupper(static_cast<unsigned char>(word[0]))) {
    return false;
  }
  bool has_lower = false;
  for (char ch : word) {
    if (std::islower(static_cast<unsigned char>(ch))) {
      has_lower = true;
      break;
    }
  }
  return has_lower;
}

bool looks_like_macro(std::string_view word) {
  if (word.size() < 2) {
    return false;
  }
  bool has_letter = false;
  for (char ch : word) {
    const unsigned char c = static_cast<unsigned char>(ch);
    if (c == '_' || std::isdigit(c)) {
      continue;
    }
    if (!std::isupper(c)) {
      return false;
    }
    has_letter = true;
  }
  return has_letter;
}

bool looks_like_cpp_type_suffix(std::string_view word) {
  return word.size() > 2 && word.ends_with("_t");
}

void push_token(std::vector<Token>& out, std::size_t begin, std::size_t end,
                TokenKind kind) {
  if (end > begin) {
    out.push_back(Token{begin, end, kind});
  }
}

std::size_t scan_ident(const std::string& line, std::size_t i) {
  while (i < line.size() && is_ident(static_cast<unsigned char>(line[i]))) {
    ++i;
  }
  return i;
}

std::size_t skip_space(const std::string& line, std::size_t i) {
  while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) {
    ++i;
  }
  return i;
}

std::size_t scan_number(const std::string& line, std::size_t i) {
  if (i + 1 < line.size() && line[i] == '0') {
    const char base = static_cast<char>(
        std::tolower(static_cast<unsigned char>(line[i + 1])));
    if (base == 'x' || base == 'b' || base == 'o') {
      i += 2;
      while (i < line.size()) {
        const unsigned char c = static_cast<unsigned char>(line[i]);
        if (base == 'x' && std::isxdigit(c)) {
          ++i;
        } else if (base == 'b' && (c == '0' || c == '1')) {
          ++i;
        } else if (base == 'o' && c >= '0' && c <= '7') {
          ++i;
        } else if (c == '\'') {
          ++i;
        } else {
          break;
        }
      }
    }
  }
  while (i < line.size() &&
         (is_digit(static_cast<unsigned char>(line[i])) || line[i] == '\'')) {
    ++i;
  }
  if (i < line.size() && line[i] == '.') {
    ++i;
    while (i < line.size() && is_digit(static_cast<unsigned char>(line[i]))) {
      ++i;
    }
  }
  if (i < line.size() &&
      (line[i] == 'e' || line[i] == 'E' || line[i] == 'p' || line[i] == 'P')) {
    ++i;
    if (i < line.size() && (line[i] == '+' || line[i] == '-')) {
      ++i;
    }
    while (i < line.size() && is_digit(static_cast<unsigned char>(line[i]))) {
      ++i;
    }
  }
  while (i < line.size()) {
    const unsigned char c = static_cast<unsigned char>(line[i]);
    if (std::isalpha(c) || c == '_') {
      ++i;
    } else {
      break;
    }
  }
  return i;
}

std::size_t match_operator(const std::string& line, std::size_t i) {
  const std::string_view rest{line.data() + i, line.size() - i};
  for (auto op : kOps3) {
    if (rest.starts_with(op)) {
      return op.size();
    }
  }
  for (auto op : kOps2) {
    if (rest.starts_with(op)) {
      return op.size();
    }
  }
  constexpr std::string_view kOps1 = "+-*/%=<>!&|^~?:.,;()[]{}@#";
  if (!rest.empty() && kOps1.find(rest[0]) != std::string_view::npos) {
    return 1;
  }
  return 0;
}

bool starts_with_at(const std::string& line, std::size_t i,
                    std::string_view needle) {
  return i + needle.size() <= line.size() &&
         std::string_view{line}.substr(i, needle.size()) == needle;
}

struct Rules {
  bool slash_slash = false;
  bool slash_star = false;
  bool hash_comment = false;
  bool preprocessor = false;
  bool triple_quotes = false;
  bool backticks = false;
};

Rules rules_for(Language lang) {
  switch (lang) {
    case Language::Cpp:
    case Language::JavaScript:
    case Language::Rust:
    case Language::Go:
      return Rules{true,  true,
                   false, lang == Language::Cpp,
                   false, lang == Language::JavaScript};
    case Language::Python:
      return Rules{false, false, true, false, true, false};
    case Language::CMake:
    case Language::Shell:
      return Rules{false, false, true, false, false, false};
    case Language::Json:
      return Rules{};
    default:
      return Rules{};
  }
}

std::uint8_t tokenize_line(Language lang, const std::string& line,
                           std::uint8_t state, std::vector<Token>* out) {
  const Rules rules = rules_for(lang);
  std::size_t i = 0;

  auto emit = [&](std::size_t begin, std::size_t end, TokenKind kind) {
    if (out) {
      push_token(*out, begin, end, kind);
    }
  };

  bool expect_type = false;
  bool expect_include = false;
  bool after_dot = false;

  if (state == kBlockComment) {
    const std::size_t start = 0;
    while (i + 1 < line.size() && !(line[i] == '*' && line[i + 1] == '/')) {
      ++i;
    }
    if (i + 1 < line.size()) {
      i += 2;
      emit(start, i, TokenKind::Comment);
      state = kNormal;
    } else {
      emit(start, line.size(), TokenKind::Comment);
      return kBlockComment;
    }
  } else if (state == kTripleDouble || state == kTripleSingle) {
    const char q = state == kTripleDouble ? '"' : '\'';
    const char needle[3] = {q, q, q};
    const std::size_t found = line.find(std::string_view{needle, 3}, 0);
    if (found == std::string::npos) {
      emit(0, line.size(), TokenKind::String);
      return state;
    }
    emit(0, found + 3, TokenKind::String);
    i = found + 3;
    state = kNormal;
  } else if (state == kMdFence) {
    if (starts_with_at(line, 0, "```")) {
      emit(0, line.size(), TokenKind::String);
      return kNormal;
    }
    emit(0, line.size(), TokenKind::String);
    return kMdFence;
  }

  while (i < line.size()) {
    const std::size_t start = i;
    const unsigned char c = static_cast<unsigned char>(line[i]);

    if (std::isspace(c)) {
      while (i < line.size() &&
             std::isspace(static_cast<unsigned char>(line[i]))) {
        ++i;
      }
      emit(start, i, TokenKind::Text);
      continue;
    }

    if (rules.slash_slash && starts_with_at(line, i, "//")) {
      emit(i, line.size(), TokenKind::Comment);
      return kNormal;
    }
    if (rules.slash_star && starts_with_at(line, i, "/*")) {
      i += 2;
      while (i + 1 < line.size() && !(line[i] == '*' && line[i + 1] == '/')) {
        ++i;
      }
      if (i + 1 < line.size()) {
        i += 2;
        emit(start, i, TokenKind::Comment);
        continue;
      }
      emit(start, line.size(), TokenKind::Comment);
      return kBlockComment;
    }
    if (rules.hash_comment && c == '#') {
      if (lang == Language::Markdown) {
        emit(i, line.size(), TokenKind::Keyword);
        return kNormal;
      }
      emit(i, line.size(), TokenKind::Comment);
      return kNormal;
    }
    if (rules.preprocessor && c == '#') {
      ++i;
      while (i < line.size() &&
             std::isspace(static_cast<unsigned char>(line[i]))) {
        ++i;
      }
      const std::size_t ident_at = i;
      i = scan_ident(line, i);
      expect_include =
          lang == Language::Cpp &&
          std::string_view{line.data() + ident_at, i - ident_at} == "include";
      emit(start, i, TokenKind::Macro);
      continue;
    }
    if (rules.triple_quotes &&
        (starts_with_at(line, i, "\"\"\"") || starts_with_at(line, i, "'''"))) {
      const char q = line[i];
      i += 3;
      const char needle[3] = {q, q, q};
      const std::size_t found = line.find(std::string_view{needle, 3}, i);
      if (found == std::string::npos) {
        emit(start, line.size(), TokenKind::String);
        return q == '"' ? kTripleDouble : kTripleSingle;
      }
      i = found + 3;
      emit(start, i, TokenKind::String);
      continue;
    }
    if (c == '"' || c == '\'' || (rules.backticks && c == '`')) {
      const char quote = static_cast<char>(c);
      std::size_t run = i;
      ++i;
      while (i < line.size()) {
        if (line[i] == '\\' && i + 1 < line.size()) {
          emit(run, i, TokenKind::String);
          emit(i, i + 2, TokenKind::Escape);
          i += 2;
          run = i;
          continue;
        }
        if (line[i] == quote) {
          ++i;
          break;
        }
        ++i;
      }
      emit(run, i, TokenKind::String);
      continue;
    }
    if (lang == Language::Cpp && c == '<' && expect_include) {
      const std::size_t close = line.find('>', i + 1);
      if (close != std::string::npos) {
        emit(i, close + 1, TokenKind::String);
        i = close + 1;
        expect_include = false;
        continue;
      }
    }
    if (is_digit(c) || (c == '.' && i + 1 < line.size() &&
                        is_digit(static_cast<unsigned char>(line[i + 1])))) {
      i = scan_number(line, i);
      emit(start, i, TokenKind::Number);
      continue;
    }
    if ((c == '@' && lang == Language::Python) ||
        (c == '$' && lang == Language::Shell)) {
      const std::size_t mark = i;
      ++i;
      if (lang == Language::Shell && i < line.size() && line[i] == '{') {
        const std::size_t close = line.find('}', i);
        i = close == std::string::npos ? line.size() : close + 1;
      } else {
        i = scan_ident(line, i);
      }
      emit(mark, i, TokenKind::Macro);
      continue;
    }
    if (is_ident_start(c)) {
      i = scan_ident(line, i);
      const std::string_view word{line.data() + start, i - start};
      TokenKind kind = TokenKind::Text;
      const std::size_t after = skip_space(line, i);
      const bool call = after < line.size() && line[after] == '(';
      if (is_constant(word)) {
        kind = TokenKind::Constant;
      } else if (is_keyword(lang, word)) {
        kind = TokenKind::Keyword;
        expect_type = contains_word(kTypeIntroducers, word);
        expect_include = lang == Language::Cpp && word == "include";
      } else if (call) {
        kind = TokenKind::Function;
        expect_type = false;
      } else if (expect_type || is_known_type(lang, word) ||
                 looks_like_type(word) ||
                 (lang == Language::Cpp && looks_like_cpp_type_suffix(word))) {
        kind = TokenKind::Type;
        expect_type = false;
      } else if (after_dot) {
        kind = TokenKind::Property;
        expect_type = false;
      } else if (looks_like_macro(word)) {
        kind = TokenKind::Macro;
        expect_type = false;
      } else {
        expect_type = false;
      }
      emit(start, i, kind);
      after_dot = false;
      continue;
    }
    if (const std::size_t n = match_operator(line, i); n > 0) {
      after_dot = (n == 1 && line[i] == '.') || starts_with_at(line, i, "->") ||
                  starts_with_at(line, i, "::");
      emit(i, i + n, TokenKind::Operator);
      i += n;
      continue;
    }

    ++i;
    emit(start, i, TokenKind::Text);
    after_dot = false;
  }
  return kNormal;
}

std::uint8_t tokenize_markdown(const std::string& line, std::uint8_t state,
                               std::vector<Token>* out) {
  if (state == kMdFence) {
    if (out) {
      push_token(*out, 0, line.size(), TokenKind::String);
    }
    if (starts_with_at(line, 0, "```")) {
      return kNormal;
    }
    return kMdFence;
  }
  if (starts_with_at(line, 0, "```")) {
    if (out) {
      push_token(*out, 0, line.size(), TokenKind::Keyword);
    }
    return kMdFence;
  }
  std::size_t i = 0;
  while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) {
    ++i;
  }
  if (i < line.size() && line[i] == '#') {
    if (out) {
      if (i > 0) {
        push_token(*out, 0, i, TokenKind::Text);
      }
      push_token(*out, i, line.size(), TokenKind::Keyword);
    }
    return kNormal;
  }
  if (i < line.size() && (line[i] == '-' || line[i] == '*' || line[i] == '+') &&
      i + 1 < line.size() && line[i + 1] == ' ') {
    if (out) {
      if (i > 0) {
        push_token(*out, 0, i, TokenKind::Text);
      }
      push_token(*out, i, i + 1, TokenKind::Operator);
      push_token(*out, i + 1, line.size(), TokenKind::Text);
    }
    return kNormal;
  }
  if (out) {
    std::size_t at = 0;
    while (at < line.size()) {
      const std::size_t tick = line.find_first_of("`*[", at);
      if (tick == std::string::npos) {
        push_token(*out, at, line.size(), TokenKind::Text);
        break;
      }
      push_token(*out, at, tick, TokenKind::Text);
      if (line[tick] == '`') {
        const std::size_t end = line.find('`', tick + 1);
        if (end == std::string::npos) {
          push_token(*out, tick, line.size(), TokenKind::String);
          break;
        }
        push_token(*out, tick, end + 1, TokenKind::String);
        at = end + 1;
        continue;
      }
      if (line[tick] == '*' && tick + 1 < line.size() &&
          line[tick + 1] == '*') {
        const std::size_t end = line.find("**", tick + 2);
        if (end == std::string::npos) {
          push_token(*out, tick, line.size(), TokenKind::Keyword);
          break;
        }
        push_token(*out, tick, end + 2, TokenKind::Keyword);
        at = end + 2;
        continue;
      }
      if (line[tick] == '[') {
        const std::size_t close = line.find(']', tick + 1);
        if (close != std::string::npos && close + 1 < line.size() &&
            line[close + 1] == '(') {
          const std::size_t url = line.find(')', close + 2);
          if (url != std::string::npos) {
            push_token(*out, tick, close + 1, TokenKind::Function);
            push_token(*out, close + 1, url + 1, TokenKind::String);
            at = url + 1;
            continue;
          }
        }
      }
      push_token(*out, tick, tick + 1, TokenKind::Text);
      at = tick + 1;
    }
  }
  return kNormal;
}

}  // namespace

Language language_from_path(std::string_view path) {
  const std::filesystem::path file{path};
  const std::string name = file.filename().string();
  std::string ext = file.extension().string();
  for (char& c : ext) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  if (name == "CMakeLists.txt" || ext == ".cmake") {
    return Language::CMake;
  }
  if (ext == ".c" || ext == ".cc" || ext == ".cpp" || ext == ".cxx" ||
      ext == ".h" || ext == ".hh" || ext == ".hpp" || ext == ".hxx" ||
      ext == ".ipp") {
    return Language::Cpp;
  }
  if (ext == ".py") {
    return Language::Python;
  }
  if (ext == ".js" || ext == ".jsx" || ext == ".ts" || ext == ".tsx" ||
      ext == ".mjs") {
    return Language::JavaScript;
  }
  if (ext == ".json") {
    return Language::Json;
  }
  if (ext == ".rs") {
    return Language::Rust;
  }
  if (ext == ".go") {
    return Language::Go;
  }
  if (ext == ".sh" || ext == ".bash" || ext == ".zsh") {
    return Language::Shell;
  }
  if (ext == ".md" || ext == ".markdown") {
    return Language::Markdown;
  }
  return Language::None;
}

SDL_Color token_color(TokenKind kind) {
  switch (kind) {
    case TokenKind::Keyword:
      return SDL_Color{47, 92, 168, 255};
    case TokenKind::Constant:
      return SDL_Color{150, 70, 40, 255};
    case TokenKind::String:
      return SDL_Color{163, 72, 51, 255};
    case TokenKind::Escape:
      return SDL_Color{196, 112, 42, 255};
    case TokenKind::Comment:
      return SDL_Color{112, 128, 96, 255};
    case TokenKind::Number:
      return SDL_Color{47, 122, 98, 255};
    case TokenKind::Type:
      return SDL_Color{32, 122, 132, 255};
    case TokenKind::Function:
      return SDL_Color{140, 92, 36, 255};
    case TokenKind::Property:
      return SDL_Color{92, 72, 148, 255};
    case TokenKind::Macro:
      return SDL_Color{130, 64, 150, 255};
    case TokenKind::Operator:
      return SDL_Color{120, 116, 108, 255};
    case TokenKind::Text:
    default:
      return SDL_Color{36, 34, 30, 255};
  }
}

void Highlighter::set_language(Language lang) {
  lang_ = lang;
  state_before_.clear();
  dirty_from_ = 0;
}

void Highlighter::invalidate(int row) {
  dirty_from_ = std::min(dirty_from_, std::max(row, 0));
}

void Highlighter::resync(const std::vector<std::string>& lines) {
  const int n = static_cast<int>(lines.size());
  if (n + 1 != static_cast<int>(state_before_.size())) {
    state_before_.assign(static_cast<std::size_t>(n) + 1, kNormal);
    dirty_from_ = 0;
  }
  int from = std::clamp(dirty_from_, 0, n);
  std::uint8_t state =
      from == 0 ? kNormal : state_before_[static_cast<std::size_t>(from)];
  for (int i = from; i < n; ++i) {
    state_before_[static_cast<std::size_t>(i)] = state;
    std::uint8_t next = kNormal;
    if (lang_ == Language::Markdown) {
      next =
          tokenize_markdown(lines[static_cast<std::size_t>(i)], state, nullptr);
    } else if (lang_ != Language::None) {
      next = tokenize_line(lang_, lines[static_cast<std::size_t>(i)], state,
                           nullptr);
    }
    if (i + 1 < n && state_before_[static_cast<std::size_t>(i) + 1] == next &&
        i > from) {
      dirty_from_ = n;
      return;
    }
    state = next;
  }
  state_before_[static_cast<std::size_t>(n)] = state;
  dirty_from_ = n;
}

std::vector<Token> Highlighter::tokens(int row, const std::string& line,
                                       const std::vector<std::string>& lines) {
  resync(lines);
  if (lang_ == Language::None || row < 0 ||
      row >= static_cast<int>(lines.size())) {
    return {Token{0, line.size(), TokenKind::Text}};
  }
  const std::uint8_t state = state_before_[static_cast<std::size_t>(row)];
  std::vector<Token> out;
  if (lang_ == Language::Markdown) {
    tokenize_markdown(line, state, &out);
  } else {
    tokenize_line(lang_, line, state, &out);
  }
  if (out.empty() && !line.empty()) {
    out.push_back(Token{0, line.size(), TokenKind::Text});
  }
  return out;
}

}  // namespace sdl

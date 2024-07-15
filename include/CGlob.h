#ifndef CGLOB_H
#define CGLOB_H

#include <string>
#include <vector>

class CGlob {
 public:
  explicit CGlob(const std::string &pattern="");

  virtual ~CGlob();

  bool isPattern() const;

  bool isValid() const;

  bool compare(const std::string &match_str) const;

  const std::string &getPattern() const { return pattern_; }

  //---

  // options

  void setCaseSensitive(bool flag) { options_.case_sensitive = flag; }
  bool getCaseSensitive() const { return options_.case_sensitive; }

  void setAllowSave(bool flag) { options_.allow_save = flag; }
  bool getAllowSave() const { return options_.allow_save; }

  void setAllowOr(bool flag) { options_.allow_or = flag; }
  bool getAllowOr() const { return options_.allow_or; }

  void setAllowNonPrintable(bool flag) { options_.allow_non_printable = flag; }
  bool getAllowNonPrintable() const { return options_.allow_non_printable; }

  void setAllowEscape(bool flag) { options_.allow_escape = flag; }
  bool getAllowEscape() const { return options_.allow_escape; }

  //---

  int getNumMatchStrings() const {
    if (! getAllowSave())
      return 0;

    return int(match_strings_.size());
  }

  const std::string &getMatchString(int i) const {
    if (i < 0 || i >= getNumMatchStrings())
      throw "Invalid Index";

    return match_strings_[size_t(i)];
  }

  virtual bool isValidChar(int c) const;

  static bool compare(const std::string &pattern, const std::string &match_str);

 private:
  void compile();
  bool compareStrings(const std::string &compile_str, const std::string &match_str) const;
  bool compareChars(char c1, char c2) const;

 private:
  using StringList = std::vector<std::string>;

  struct OptionsData {
    bool case_sensitive      { true };
    bool allow_save          { false };
    bool allow_or            { true };
    bool allow_non_printable { true };
    bool allow_escape        { true };
  };

  // pattern
  std::string pattern_;

  // compile state
  std::string compile_;
  bool        compiled_ { false };
  bool        valid_ { false };

  // options
  OptionsData options_;

  // match
  mutable int        match_start_ { -1 };
  mutable StringList match_strings_;
};

//------

#define CGLOB_THROW(pattern,message,pos) \
  throw CGlobError(pattern,message,pos)

namespace CGlobUtil {
  using StringList = std::vector<std::string>;

  bool parse(const std::string &str, const std::string &pattern);

  bool parse(const std::string &str, const std::string &pattern, StringList &match_strs);
}

struct CGlobError {
  CGlobError(const std::string &pattern1, const std::string &message1, int pos1=0) :
   pattern(pattern1), message(message1), pos(pos1) {
  }

  std::string format();

  std::string pattern;
  std::string message;
  int         pos { 0 };
};

#endif

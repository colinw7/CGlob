#ifndef CGLOB_H
#define CGLOB_H

#include <string>
#include <vector>

class CGlob {
 private:
  typedef std::vector<std::string> StringList;

  std::string        pattern_;
  std::string        compile_;
  bool               compiled_;
  bool               valid_;
  bool               case_sensitive_;
  bool               allow_save_;
  bool               allow_or_;
  bool               allow_non_printable_;
  mutable int        match_start_;
  mutable StringList match_strings_;

 public:
  CGlob(const std::string &pattern);

  virtual ~CGlob();

  bool isPattern() const;

  bool isValid() const;

  bool compare(const std::string &match_str) const;

  const std::string &getPattern() const { return pattern_; }

  void setCaseSensitive(bool flag) { case_sensitive_ = flag; }
  bool getCaseSensitive() const { return case_sensitive_; }

  void setAllowSave(bool flag) { allow_save_ = flag; }
  bool getAllowSave() const { return allow_save_; }

  void setAllowOr(bool flag) { allow_or_ = flag; }
  bool getAllowOr() const { return allow_or_; }

  void setAllowNonPrintable(bool flag) { allow_non_printable_ = flag; }
  bool getAllowNonPrintable() const { return allow_non_printable_; }

  int getNumMatchStrings() const {
    if (! allow_save_)
      return 0;

    return match_strings_.size();
  }

  const std::string &getMatchString(int i) const {
    if (i < 0 || i >= getNumMatchStrings())
      throw "Invalid Index";

    return match_strings_[i];
  }

  virtual bool isValidChar(int c) const;

  static bool compare(const std::string &pattern, const std::string &match_str);

 private:
  void compile();
  bool compareStrings(const std::string &compile_str, const std::string &match_str) const;
  bool compareChars(char c1, char c2) const;
};

#define CGLOB_THROW(pattern,message,pos) \
  throw CGlobError(pattern,message,pos)

namespace CGlobUtil {
  typedef std::vector<std::string> StringList;

  bool parse(const std::string &str, const std::string &pattern);

  bool parse(const std::string &str, const std::string &pattern, StringList &match_strs);
}

struct CGlobError {
  std::string pattern;
  std::string message;
  int         pos;

  CGlobError(const std::string &pattern1, const std::string &message1, int pos1 = 0) :
   pattern(pattern1), message(message1), pos(pos1) {
  }

  std::string format();
};

#endif

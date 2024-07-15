#include <CGlob.h>
#include <sys/types.h>

#define CGLOB_MATCH_ANY     '\001'
#define CGLOB_MATCH_SINGLE  '\002'
#define CGLOB_MATCH_ONE_OF  '\003'
#define CGLOB_MATCH_NONE_OF '\004'
#define CGLOB_MATCH_OR      '\005'
#define CGLOB_MATCH_START   '\006'
#define CGLOB_MATCH_END     '\007'

CGlob::
CGlob(const std::string &pattern) :
 pattern_(pattern)
{
}

CGlob::
~CGlob()
{
}

bool
CGlob::
isPattern() const
{
  bool in_brackets = false;

  auto pattern_str_len = uint(pattern_.length());

  for (uint i = 0; i < pattern_str_len; i++) {
    switch (pattern_[i]) {
      case '*':
        return true;
      case '?':
        return true;
      case '[': {
        i++;

        /* Find Closing Brace */

        while (i < pattern_str_len) {
          if      (pattern_[i] == '\\' && getAllowEscape()) {
            i++;

            if (i < pattern_str_len)
              i++;
          }
          else if (pattern_[i] == ']')
            break;
          else
            i++;
        }

        if (pattern_[i] != ']')
          return false;

        return true;
      }
      case '|': {
        if (getAllowOr()) {
          if (i == 0)
            return false;

          return true;
        }

        break;
      }
      case '(': {
        if (getAllowSave()) {
          if (in_brackets)
            return false;

          in_brackets = true;
        }

        break;
      }
      case ')': {
        if (getAllowSave()) {
          if (! in_brackets)
            return false;

          in_brackets = false;

          return true;
        }

        break;
      }
      case '\\': {
        if (getAllowEscape()) {
          if (i < pattern_str_len - 1)
            i++;
        }

        break;
      }
      default: {
        break;
      }
    }
  }

  return false;
}

bool
CGlob::
isValid() const
{
  CGlob *th = const_cast<CGlob *>(this);

  if (! compiled_)
    th->compile();

  return valid_;
}

bool
CGlob::
compare(const std::string &str) const
{
  CGlob *th = const_cast<CGlob *>(this);

  if (! compiled_)
    th->compile();

  th->match_start_ = -1;

  th->match_strings_.clear();

  bool flag = compareStrings(compile_, str);

  return flag;
}

void
CGlob::
compile()
{
  compile_ = "";

  valid_ = true;

  bool in_brackets = false;

  auto pattern_str_len = uint(pattern_.length());

  for (uint i = 0; i < pattern_str_len; i++) {
    switch (pattern_[i]) {
      case '*': {
        compile_ += CGLOB_MATCH_ANY;
        break;
      }
      case '?': {
        compile_ += CGLOB_MATCH_SINGLE;
        break;
      }
      case '[': {
        i++;

        /* Check for Leading Circumflex */

        int type;

        if (pattern_[i] == '^') {
          type = CGLOB_MATCH_NONE_OF;

          i++;
        }
        else
          type = CGLOB_MATCH_ONE_OF;

        /* Save Start of String inside Braces */

        auto i1 = i;

        /* Find Closing Brace */

        while (i < pattern_str_len) {
          if      (pattern_[i] == '\\' && getAllowEscape()) {
            i++;

            if (i < pattern_str_len)
              i++;
          }
          else if (pattern_[i] == ']')
            break;
          else
            i++;
        }

        if (pattern_[i] != ']') {
          CGLOB_THROW(pattern_, "No closing square bracket", int(i));
          valid_ = false;
        }

        /* Save End of String inside Braces */

        auto i2 = i - 1;

        /* If Empty Braces then Error */

        if (i1 > i2) {
          CGLOB_THROW(pattern_, "Empty square brackets", int(i2));
          valid_ = false;
        }

        /* Initialise Character Flags */

        bool cflags[256];

        for (int k = 0; k < 256; k++)
          cflags[k] = false;

        /* Set Character Flags for Characters inside the
           braces, this will ensure the list of characters
           stored in the compile string will be minimised. */

        for (uint k = i1; k <= i2; k++) {
          /* Check for escaped character */

          if      (pattern_[k] == '\\' && getAllowEscape()) {
            if (k <= i2 - 1)
              k++;

            if (! isValidChar(pattern_[k])) {
              CGLOB_THROW(pattern_, "Invalid character", int(k));
              valid_ = false;
            }

            auto c = static_cast<unsigned int>(pattern_[k]);

            cflags[c] = true;
          }

          /* Check for Range of Characters */

          else if (k <= i2 - 2 && pattern_[k + 1] == '-') {
            int start_char = pattern_[k + 0];
            int end_char   = pattern_[k + 2];

            k += 2;

            /* Ensure End is greater than Start */

            if (start_char >= end_char) {
              CGLOB_THROW(pattern_, "Range out of order", int(k));
              valid_ = false;
            }

            for (int l = start_char; l <= end_char; l++) {
              /* Ensure Character is Printable */

              if (! isValidChar(l)) {
                CGLOB_THROW(pattern_, "Invalid character", l);
                valid_ = false;
              }

              cflags[l] = true;
            }
          }

          /* Otherwise just insert Character */

          else {
            if (! isValidChar(pattern_[k])) {
              CGLOB_THROW(pattern_, "Invalid character", int(k));
              valid_ = false;
            }

            auto c = static_cast<unsigned int>(pattern_[k]);

            cflags[c] = true;
          }
        }

        /* Count Number of Characters selected */

        int num_chars = 0;

        for (int k = 0; k < 256; k++)
          if (cflags[k])
            num_chars++;

        /* Add match type, number of characters and characters
           to compile string */

        compile_ += char(type);
        compile_ += char(num_chars);

        for (int k = 0; k < 256; k++)
          if (cflags[k])
            compile_ += char(k);

        break;
      }
      case '|': {
        if (getAllowOr()) {
          if (i == 0) {
            CGLOB_THROW(pattern_, "Invalid Or", int(i));
            valid_ = false;
          }

          compile_ += CGLOB_MATCH_OR;
        }
        else
          compile_ += pattern_[i];

        break;
      }
      case '(': {
        if (getAllowSave()) {
          if (in_brackets) {
            CGLOB_THROW(pattern_, "Invalid bracket nesting", int(i));
            valid_ = false;
          }

          compile_ += CGLOB_MATCH_START;

          in_brackets = true;
        }
        else
          compile_ += pattern_[i];

        break;
      }
      case ')': {
        if (getAllowSave()) {
          if (! in_brackets) {
            CGLOB_THROW(pattern_, "Invalid bracket nesting", int(i));
            valid_ = false;
          }

          compile_ += CGLOB_MATCH_END;

          in_brackets = false;
        }
        else
          compile_ += pattern_[i];

        break;
      }
      case '\\': {
        if (getAllowEscape()) {
          if (i < pattern_str_len - 1)
            i++;
        }

        if (! isValidChar(pattern_[i])) {
          CGLOB_THROW(pattern_, "Invalid character", int(i));
          valid_ = false;
        }

        compile_ += pattern_[i];

        break;
      }
      default: {
        if (! isValidChar(pattern_[i])) {
          CGLOB_THROW(pattern_, "Invalid character", int(i));
          valid_ = false;
        }

        compile_ += pattern_[i];

        break;
      }
    }
  }

  if (getAllowSave()) {
    if (in_brackets)
      compile_ += CGLOB_MATCH_END;
  }

  compiled_ = true;
}

bool
CGlob::
compareStrings(const std::string &compile_str, const std::string &match_str) const
{
  if (getAllowOr()) {
    std::string::size_type pos = compile_str.find(CGLOB_MATCH_OR);

    if (pos != std::string::npos) {
      std::string compile_str1 = compile_str.substr(0, pos);

      if (compareStrings(compile_str1, match_str))
        return true;

      std::string compile_str2 = compile_str.substr(pos + 1);

      if (compareStrings(compile_str2, match_str))
        return true;

      return false;
    }
  }

  uint i = 0;
  uint j = 0;

  auto compile_str_len = uint(compile_str.length());
  auto match_str_len   = uint(match_str  .length());

  while (i < compile_str_len) {
    switch (compile_str[i]) {
      case CGLOB_MATCH_ANY: {
        i++;

        if (getAllowSave()) {
          /* Compress multiple *'s into one */

          bool match_end = false;

          while (i < compile_str_len &&
                 (compile_str[i] == CGLOB_MATCH_START ||
                  compile_str[i] == CGLOB_MATCH_END)) {
            if (compile_str[i] == CGLOB_MATCH_END)
              match_end = true;

            i++;
          }

          while (i < compile_str_len && compile_str[i] == CGLOB_MATCH_ANY) {
            i++;

            while (i < compile_str_len &&
                   (compile_str[i] == CGLOB_MATCH_START ||
                    compile_str[i] == CGLOB_MATCH_END)) {
              if (compile_str[i] == CGLOB_MATCH_END)
                match_end = true;

              i++;
            }
          }

          /* If end of string then we have a Match */

          if (i >= compile_str_len) {
            if (match_start_ >= 0) {
              auto *th = const_cast<CGlob *>(this);

              std::string match_string = match_str.substr(uint(match_start_));

              th->match_strings_.push_back(match_string);
            }

            return true;
          }

          /* Check rest of Pattern against any terminating
             sub-string of the Check String for Match */

          auto length = uint(match_str.length() - j);

          for (uint k = 0; k < length; k++) {
            StringList savematch_strings = match_strings_;

            CGlob *th = const_cast<CGlob *>(this);

            if (! match_end)
              th->match_start_ = 0;
            else
              th->match_start_ = -1;

            th->match_strings_.clear();

            if (compareStrings(compile_str.substr(i), match_str.substr(j + k))) {
              if (! match_end) {
                std::string match_string = match_str.substr(j, k);

                for (auto p = match_strings_.begin(); p != match_strings_.end(); ++p)
                  savematch_strings.push_back(match_string + (*p));
              }
              else {
                auto match_string = match_str.substr(j, k);

                savematch_strings.push_back(match_string);
              }

              th->match_strings_ = savematch_strings;

              return true;
            }

            th->match_strings_ = savematch_strings;
          }

          /* No Match so Fail */

          return false;
        }
        else {
          /* Compress multiple *'s into one */

          while (i < compile_str_len && compile_str[i] == CGLOB_MATCH_ANY)
            i++;

          /* If end of string then we have a Match */

          if (i >= compile_str_len)
            return true;

          /* Check rest of Pattern against any terminating
             sub-string of the Check String for Match */

          auto length = uint(match_str.length() - j);

          for (uint k = 0; k < length; k++)
            if (compareStrings(compile_str.substr(i), match_str.substr(j + k)))
              return true;

          /* No Match so Fail */

          return false;
        }

        break;
      }
      case CGLOB_MATCH_SINGLE: {
        /* If End of String then Fail */

        if (j >= match_str_len)
          return false;

        /* Consume '?' and one Character from Check String */

        i++;

        j++;

        break;
      }
      case CGLOB_MATCH_ONE_OF:
      case CGLOB_MATCH_NONE_OF: {
        /* If End of String then Fail */

        if (j >= match_str_len)
          return false;

        /* Get and Skip type */

        int type = compile_str[i++];

        /* Get and Skip Number of Characters to Check Against */

        auto num_chars = uint(compile_str[i++]);

        /* Check Each Character for Match */

        uint k;

        for (k = 0; k < num_chars; k++)
          if (compareChars(match_str[j], compile_str[i + k]))
            break;

        /* If no match found then Fail for CGLOB_MATCH_ONE_OF */

        if (type == CGLOB_MATCH_ONE_OF && k == num_chars)
          return false;

        /* If match found then Fail for CGLOB_MATCH_NONE_OF */

        if (type == CGLOB_MATCH_NONE_OF && k < num_chars)
          return false;

        /* Consume Match Characters and one Character from Check String */

        i += num_chars;

        j++;

        break;
      }
      case CGLOB_MATCH_START: {
        if (getAllowSave()) {
          CGlob *th = const_cast<CGlob *>(this);

          th->match_start_ = int(j);

          i++;
        }
        else {
          if (! compareChars(match_str[j], compile_str[i]))
            return false;

          i++;

          j++;
        }

        break;
      }
      case CGLOB_MATCH_END: {
        if (getAllowSave()) {
          auto match_string = match_str.substr(uint(match_start_), uint(int(j) - match_start_));

          CGlob *th = const_cast<CGlob *>(this);

          th->match_strings_.push_back(match_string);

          th->match_start_ = -1;

          i++;
        }
        else {
          if (! compareChars(match_str[j], compile_str[i]))
            return false;

          i++;

          j++;
        }

        break;
      }
      default: {
        /* If End of String then Fail */

        if (j >= match_str_len)
          return false;

        /* Fail if not Exact Match between Compile String and Check String */

        if (! compareChars(match_str[j], compile_str[i]))
          return false;

        /* Consume one Character from Compile String and Check String */

        i++;

        j++;

        break;
      }
    }
  }

  /* If we have Consumed all the Check String characters then
     we have a Match */

  if (j >= match_str_len)
    return true;

  /* Unmatched characters in Check String remain so Fail */

  return false;
}

bool
CGlob::
compareChars(char c1, char c2) const
{
  if (! getCaseSensitive()) {
    if (isupper(c1))
      c1 = char(tolower(c1));

    if (isupper(c2))
      c2 = char(tolower(c2));
  }

  return (c1 == c2);
}

bool
CGlob::
isValidChar(int c) const
{
  if (! getAllowNonPrintable() && ! isprint(c))
    return false;

  return true;
}

bool
CGlob::
compare(const std::string &pattern, const std::string &str)
{
  CGlob glob(pattern);

  return glob.compare(str);
}

std::string
CGlobError::
format()
{
  std::string fmt_message = message + " : " + pattern.substr(0, uint(pos));

  fmt_message += ">" + pattern.substr(uint(pos), 1) + "<";

  auto len = uint(pattern.length());

  if (pos < int(len - 1))
    fmt_message += pattern.substr(uint(pos + 1));

  return fmt_message;
}

//----------

bool
CGlobUtil::
parse(const std::string &str, const std::string &pattern)
{
  CGlob glob(pattern);

  if (! glob.compare(str))
    return false;

  return true;
}

bool
CGlobUtil::
parse(const std::string &str, const std::string &pattern, StringList &match_strs)
{
  CGlob glob(pattern);

  if (! glob.compare(str))
    return false;

  auto num = glob.getNumMatchStrings();

  match_strs.clear();

  for (int i = 0; i < num; ++i)
    match_strs.push_back(glob.getMatchString(i));

  return true;
}

#ifndef VSRTL_VCDUTILS_H
#define VSRTL_VCDUTILS_H

#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>

namespace vsrtl {

struct Defer {
  Defer(const std::function<void()> f) : m_f(f) {}
  ~Defer() { m_f(); }

private:
  std::function<void()> m_f;
};

class VCDFile {
public:
  VCDFile(const std::string &filename);
  ~VCDFile();
  Defer writeHeader();
  Defer scopeDef(const std::string &name);
  Defer dumpVars();
  void flush() { m_file.flush(); }

  // Defines the variable @p name within the current scope, and returns a unique
  // identifier associated with the variable.
  std::string varDef(const std::string &name, unsigned width);
  void writeTime(uint64_t time);
  void writeVarChange(const std::string &ref, uint64_t value);
  void varInitVal(const std::string &ref, uint64_t value) {
    m_dumpVars[ref] = value;
  }

private:
  std::string genId();
  void writeLine(const std::string &line);
  std::ofstream m_file;
  std::map<std::string, unsigned> m_varWidths;
  std::map<std::string, uint64_t> m_dumpVars;

  unsigned m_varCntr = 0;
  unsigned m_scopeLevel = 0;
};

} // namespace vsrtl

#endif // VSRTL_VCDUTILS_H

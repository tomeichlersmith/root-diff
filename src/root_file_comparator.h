#ifndef ROOT_DIFF_FILE_COMPARATOR
#define ROOT_DIFF_FILE_COMPARATOR

#include <cstring>
#include <ctime>
#include <fstream>
#include <set>

#include "Bytes.h"
#include "RtypesCore.h"
#include "TDatime.h"
#include "root_obj_comparator.h"
#include "timer.h"
#include "unistd.h"

/**
 * Header length of a TFile.
 */
#define HEADER_LEN 100

namespace rootdiff {

/**
 * Four agreement levels:
 * 1. NOT EQUAL - None of the below
 * 2. LOGICALLY EQUAL - Key, Cycle, and Name are Equal
 * 3. STRICTLY EQUAL - LOGICALLY and Byte-Level Content Equal
 * 4. EXACTLLY EQUAL - STRICTLY and Timestamps equal
 */
typedef enum AgreeLevel_enum { Not_eq, Logic_eq, Strict_eq, Exact_eq } AgreeLevel;

/**
 * The root file comparator class
 */
class FileComparer {
 public:
  /**
   * Constructor
   * Set whether or not to print debug statements.
   */
  FileComparer(bool debug) : debug_(debug) {}

  /*
   * Compare two root files and return the agreement level of the
   * comparsion
   *
   * @param[in] f_1 Name of file 1
   * @param[in] f_2 Name of file 2
   * @param[in] mode Mode of comparison
   * @param[in] log_fn Name of log file
   * @param[in] ignored_classes set of class names to ignore during comparison
   */
  AgreeLevel comp(char *f_1, char *f_2, const char *mode,
                  const char *log_fn,
                  std::set<std::string> ignored_classes) const;

 private:
  ///should we print debug messages?
  bool debug_;
};

}  // namespace rootdiff

#endif

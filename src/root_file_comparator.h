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

/**
 * Four agreement levels:
 * 1. NOT EQUAL - None of the below
 * 2. LOGICALLY EQUAL - Key, Cycle, and Name are Equal
 * 3. STRICTLY EQUAL - LOGICALLY and Byte-Level Content Equal
 * 4. EXACTLLY EQUAL - STRICTLY and Timestamps equal
 */
typedef enum Agree_lv { Not_eq, Logic_eq, Strict_eq, Exact_eq } Agree_lv;

/**
 * The root file comparator class
 */
class Rootfile_comparator {
 public:
  /**
   * Constructor
   * Set whether or not to print debug statements.
   */
  Rootfile_comparator(bool debug) : debug_(debug) {}

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
  Agree_lv root_file_cmp(char *f_1, char *f_2, const char *mode,
                         const char *log_fn,
                         std::set<std::string> ignored_classes);

 private:
  ///should we print debug messages?
  bool debug_;
};

#endif

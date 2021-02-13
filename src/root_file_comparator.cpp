#include "root_file_comparator.h"

#include <exception>

namespace rootdiff {

/**
 * Get the next string from the input header
 */
std::string get_next(char *header) {
  char str_len;
  frombuf(header, &str_len);
  char* str = new char[str_len];
  for (int i = 0; i < str_len; i++) {
    frombuf(header, &str[i]);
  }
  std::string ret(str);
  delete[] str;
  return ret;
}

/**
 * Get object information from the header (i.e. TKey)
 *
 * @param[in] header_array bytes in the header of the file
 * @param[in] cur current index of header
 * @param[in] f Pointer to open TFile
 */
static ObjectInfo get_obj_info(char *header_array, Long64_t cur,
                               const TFile &f, bool debug) {
  UInt_t datime;
  ObjectInfo obj_info;
  char *header;
  char class_name_len;
  char obj_name_len;

  header = header_array;
  frombuf(header, &(obj_info.nbytes));
  if (!obj_info.nbytes) {
    std::cerr << "The size of the object buffer is unaccessible." << std::endl;
    throw std::exception();
  }

  Version_t version_key;
  frombuf(header, &version_key);
  frombuf(header, &(obj_info.obj_len));
  frombuf(header, &datime);
  frombuf(header, &(obj_info.key_len));
  frombuf(header, &(obj_info.cycle));

  if (version_key > 1000) {
    // for large file the type of seek_key and seek_pdir is long
    frombuf(header, &(obj_info.seek_key));
    frombuf(header, &(obj_info.seek_pdir));
  } else {
    Int_t s_key, s_dir;
    frombuf(header, &s_key);
    frombuf(header, &s_dir);
    obj_info.seek_key = (Long64_t)s_key;
    obj_info.seek_pdir = (Long64_t)s_dir;
  }

  // Get the class name of object
  obj_info.class_name = get_next(header);

  if (cur == f.GetSeekFree()) {
    obj_info.class_name = "FreeSegments";
  }
  if (cur == f.GetSeekInfo()) {
    obj_info.class_name = "StreamerInfo";
  }
  if (cur == f.GetSeekKeys()) {
    obj_info.class_name = "KeysList";
  }

  obj_info.obj_name = get_next(header);

  obj_info.date = 0;
  obj_info.time = 0;

  if (debug) {
    std::cout << "============ '" << obj_info.class_name << "' obj info=============" << std::endl;
    std::cout << "name: " << obj_info.obj_name << std::endl;
    std::cout << "class: " << obj_info.class_name << std::endl;
    std::cout << "seek_key: " << obj_info.seek_key << std::endl;
    std::cout << "version: " << version_key << std::endl;
    std::cout << "nbytes: " << obj_info.nbytes << std::endl;
    std::cout << "object len: " << obj_info.obj_len << std::endl;
    std::cout << "datime: " << datime << std::endl;
    std::cout << "key len: " << obj_info.key_len << std::endl;
    std::cout << "# of cycles: " << obj_info.cycle << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::endl;
  }

  TDatime::GetDateTime(datime, obj_info.date, obj_info.time);
  return std::move(obj_info);
}

AgreeLevel FileComparer::comp(const std::string &fn_1, 
                              const std::string &fn_2,
                              const std::string &mode,
                              const std::string &log_fn,
                              std::set<std::string> ignored_classes) const {
  int num_obj_in_f1 = 0, num_obj_in_f2 = 0, num_logical_equal = 0,
      num_exact_equal = 0, num_strict_equal = 0;

  // Get comparison mode
  bool compressed{false};
  if (mode == "CC") {
    compressed = true;
  } else if (mode != "UC") {
    std::cerr << "Unrecognized comparison mode '" << mode << "'" << std::endl;
    throw std::exception();
  }
  ObjectComparer obj_comp(debug_, compressed);

  // Check if input files are accessible
  if (access(fn_1.c_str(), F_OK) == -1) {
    std::cout << fn_1 << " does not exist." << std::endl;
    throw std::exception();
  }

  if (access(fn_2.c_str(), F_OK) == -1) {
    std::cout << fn_2 << " does not exist." << std::endl;
    throw std::exception();
  }

  bool logic_eq = true, strict_eq = true, exact_eq = true;

  // Create log file
  std::ofstream log_f;
  if (!log_f) {
    std::cout << "cannot create log file" << std::endl;
  }
  log_f.open(log_fn);

  Timer tmr;
  double t = tmr.elapsed();

  // Scan file 1 and generate object information array

  TFile f_1(fn_1.c_str());

  Int_t nwheader;
  nwheader = 64;
  Int_t nread_1 = nwheader;

  Long64_t cur_1 = HEADER_LEN, f1_end = f_1.GetEND();

  char header_1[HEADER_LEN];

  ObjectInfo obj_info_1;

  std::vector<ObjectInfo> objs_info;

  while (cur_1 < f1_end) {
    num_obj_in_f1++;
    f_1.Seek(cur_1);

    if (cur_1 + nread_1 >= f1_end) {
      nread_1 = f1_end - cur_1 - 1;
    }

    if (f_1.ReadBuffer(header_1, nread_1)) {
      std::cerr << "Failed to read the object header from "
        << fn_1 << "from disk at " << cur_1 << std::endl;
      return AgreeLevel::Not_eq;
    }

    obj_info_1 = get_obj_info(header_1, cur_1, f_1, debug_);
    obj_info_1.obj_index = num_obj_in_f1;

    if (obj_info_1.nbytes < 0) {
      cur_1 -= obj_info_1.nbytes;
      continue;
    }

    if (debug_) {
      std::cout << "Ignored classes are: ";
      for (auto const& c : ignored_classes) {
        std::cout << c << " ";
      }
      std::cout << std::endl;
    }

    if (ignored_classes.find(obj_info_1.class_name) == ignored_classes.end()) {
      objs_info.push_back(obj_info_1);
    } else {
      log_f << obj_info_1.class_name << " in file 1 with index "
            << obj_info_1.obj_index << " and object name "
            << obj_info_1.obj_name << " is ignored" << std::endl;
    }

    cur_1 += obj_info_1.nbytes;
  }

  // For each object in file 2, find if there exists an object which
  // has same information in file 1. construct an table whose entry is
  // the pair of objects share same information from file 1 and file 2.
  // If there exists an object in file 2 which does not has matched
  // object in file 1, we say file 1 is not equal to file 2

  TFile f_2(fn_2.c_str());

  Int_t nread_2 = nwheader;

  Long64_t cur_2 = HEADER_LEN, f2_end = f_2.GetEND();

  char header_2[HEADER_LEN];

  ObjectInfo obj_info_2;

  std::vector<std::pair<ObjectInfo , ObjectInfo >> objs_pair;

  while (cur_2 < f2_end) {
    num_obj_in_f2++;
    f_2.Seek(cur_2);

    if (cur_2 + nread_2 >= f2_end) {
      nread_2 = f2_end - cur_2 - 1;
    }

    if (f_2.ReadBuffer(header_2, nread_2)) {
      std::cerr << "Failed to read the object header from "
       << fn_2 << " from disk at " << cur_2 << std::endl;
      return AgreeLevel::Not_eq;
    }

    obj_info_2 = get_obj_info(header_2, cur_2, f_2, debug_);
    obj_info_2.obj_index = num_obj_in_f2;

    if (obj_info_2.nbytes < 0) {
      cur_2 -= obj_info_2.nbytes;
      continue;
    }

    if (debug_) {
      std::cout << "Ignored classes are: ";
      for (auto const& c : ignored_classes) {
        std::cout << c << " ";
      }
      std::cout << std::endl;
    }

    if (ignored_classes.find(obj_info_2.class_name) == ignored_classes.end()) {
      // If current class is not in the ignored classes list
      bool found_match{false};
      for (auto info_it = objs_info.begin(); info_it != objs_info.end(); ++info_it) {
        ObjectInfo& info = *info_it;
        if (obj_comp.logic_cmp(info, obj_info_2)) {
          num_logical_equal++;
          std::pair<ObjectInfo , ObjectInfo> obj_pair(info, obj_info_2);
          // every obj_info can only be used once
          log_f << info.class_name << " with index "
                << info.obj_index << " with object name "
                << info.obj_name << " in file 1 is structual-equal to "
                << obj_info_2.class_name << " with index "
                << obj_info_2.obj_index << " and object name "
                << obj_info_2.obj_name << " in file 2 " << std::endl;

          objs_info.erase(info_it);
          objs_pair.push_back(obj_pair);
          found_match = true;
          break;
        } //found logical match
      } //loop over object info

      if (not found_match) {
        // does not found matched object in file 1
        log_f << "Cannot find matched object for the instance of "
              << obj_info_2.class_name << " in file 2 with index "
              << obj_info_2.obj_index << std::endl;

        logic_eq = false;
        strict_eq = false;
        exact_eq = false;
      }
    } else {
      log_f << obj_info_2.class_name << " in file 2 with index "
            << obj_info_2.obj_index << " and object name "
            << obj_info_2.obj_name << " is ignored" << std::endl;
    }

    cur_2 += obj_info_2.nbytes;
  }

  // After iterating all objects in file 2, if there are obj_info left in file
  // 1, file 1 is not logically equal to file 2

  if (!objs_info.empty()) {
    for (auto const& info : objs_info) {
      log_f << "Cannot find matched object for the instance of "
            << info.class_name << " in file 1 with index "
            << info.obj_index << " with size " << info.nbytes
            << ", cycle number " << info.cycle << " and object name "
            << info.obj_name << std::endl;
    }
    logic_eq = false;
    strict_eq = false;
    exact_eq = false;
  }

  // Compare the two objects in same entry. If the two objects are
  // strictly/exactly equal to each other, we say the entry is
  // strictly/exactly agreed. If every entry is strictly/exactly agreed,
  // we say that file 1 is strictly/exactly equal to file 2.

  for (auto const [first, second] : objs_pair) {
    if (!obj_comp.strict_cmp(first, f_1, second, f_2)) {
      log_f << first.class_name << " in file 1 with index "
            << first.obj_index << " and object name "
            << first.obj_name << " is NOT CONTENT-EQUAL to "
            << second.class_name << " in file 2 with index "
            << second.obj_index << " and object name "
            << second.obj_name << std::endl;

      strict_eq = false;
      exact_eq = false;

    } else {
      num_strict_equal++;
      if (!obj_comp.exact_cmp(first, second)) {
        log_f << first.class_name << " in file 1 with index "
              << first.obj_index << " and object name "
              << first.obj_name << " is NOT BITWISE-EQUAL to "
              << second.class_name << " in file 2 with index "
              << second.obj_index << " and object name "
              << second.obj_name << std::endl;

        exact_eq = false;
      } else {
        num_exact_equal++;
      }
    }
  }

  tmr.reset();
  t = tmr.elapsed();
  log_f << std::endl;
  log_f << "================= Comparison summary =================" << std::endl;
  log_f << "Time elapsed: " << t << std::endl;

  log_f << "Number of objects in file 1 is: " << num_obj_in_f1 << std::endl;
  log_f << "Number of objects in file 2 is: " << num_obj_in_f2 << std::endl;
  log_f << "Number of structural equivalent: " << num_logical_equal << std::endl;
  log_f << "Number of content equivalent: " << num_strict_equal << std::endl;
  log_f << "Number of bitwise equivalent: " << num_exact_equal << std::endl;

  log_f.close();

  if (exact_eq) {
    return AgreeLevel::Exact_eq;
  } else if (strict_eq) {
    return AgreeLevel::Strict_eq;
  } else if (logic_eq) {
    return AgreeLevel::Logic_eq;
  } else {
    return AgreeLevel::Not_eq;
  }
}

}  // namespace rootdiff

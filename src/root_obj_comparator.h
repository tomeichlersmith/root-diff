#ifndef __ROOT_OBJ_COMP_H__
#define __ROOT_OBJ_COMP_H__

#include <iostream>

#include "RZip.h"
#include "TBuffer.h"
#include "TBufferFile.h"
#include "TCollection.h"
#include "TFile.h"
#include "TKey.h"
#include "TObject.h"

#include <string>

#define ROOT_DIR "TDirectoryFile"

namespace rootdiff {

/**
 * Struct storing the object information
 */
struct ObjectInfo {
  /// Length of the TKey for this object
  Short_t key_len;
  /// Cycle number for this object
  Short_t cycle;
  /// Number of bytes in this object
  Int_t nbytes;
  /// Date object was created
  Int_t date;
  /// Time object was created
  Int_t time;
  /// Length of this object in bytes
  Int_t obj_len;
  /// Index of this object in the TFile
  Int_t obj_index;
  /// Key to look for this object inside of its directory
  Long64_t seek_key;
  /// Key to look for this object's directory
  Long64_t seek_pdir;
  /// Name of the class of this object
  std::string class_name;
  /// Name of the object
  std::string obj_name;
};  // ObjectInfo

class ObjectComparer {
 public:
  ObjectComparer(bool debug, bool comp_compressed) : 
    debug_(debug), compare_compressed_(comp_compressed) {}
  bool logic_cmp(const ObjectInfo &obj_info_1, const ObjectInfo &obj_info_2) const;
  bool exact_cmp(const ObjectInfo &obj_info_1, const ObjectInfo &obj_info_2) const;
  bool strict_cmp(const ObjectInfo &obj_info_1, TFile *f1, const ObjectInfo &obj_info_2, TFile *f2) const {
    // Since TDirectoryFile class has fUUID attribute,
    // we could not compare two TDirectoryFile objects
    if (obj_info_1.class_name == ROOT_DIR or obj_info_2.class_name == ROOT_DIR) return true;

    if (compare_compressed_) { return compressed_cmp(obj_info_1,f1,obj_info_2,f2); }
    else                     { return uncompressed_cmp(obj_info_1,f1,obj_info_2,f2); }
  }
 private:
  bool compressed_cmp(const ObjectInfo &obj_info_1, TFile *f1, const ObjectInfo &obj_info_2, TFile *f2) const;
  bool uncompressed_cmp(const ObjectInfo &obj_info_1, TFile *f1, const ObjectInfo &obj_info_2, TFile *f2) const;
 private:
  bool compare_compressed_;
  bool debug_;
};

}  // namespace rootdiff

#endif

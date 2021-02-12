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

#define NAME_LEN 512

namespace rootdiff {

/*
 * Struct storing the object information
 */
typedef struct Obj_info {
  Short_t key_len, cycle;

  Int_t nbytes, date, time, obj_len, obj_index;

  Long64_t seek_key, seek_pdir;

  char class_name[NAME_LEN];
  char obj_name[NAME_LEN];

} Obj_info;

class ObjectComparer {
 public:
  ObjectComparer(bool debug, bool comp_compressed) : 
    debug_(debug), compare_compressed_(comp_compressed) {}
  bool logic_cmp(Obj_info *obj_info_1, Obj_info *obj_info_2) const;
  bool exact_cmp(Obj_info *obj_info_1, Obj_info *obj_info_2) const;
  bool strict_cmp(Obj_info *obj_info_1, TFile *f1, Obj_info *obj_info_2, TFile *f2) const {
    if (compare_compressed_) { return compressed_cmp(obj_info_1,f1,obj_info_2,f2); }
    else                     { return uncompressed_cmp(obj_info_1,f1,obj_info_2,f2); }
  }
 private:
  bool compressed_cmp(Obj_info *obj_info_1, TFile *f1, Obj_info *obj_info_2, TFile *f2) const;
  bool uncompressed_cmp(Obj_info *obj_info_1, TFile *f1, Obj_info *obj_info_2, TFile *f2) const;
 private:
  bool compare_compressed_;
  bool debug_;
};

}  // namespace rootdiff

#endif

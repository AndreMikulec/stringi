/* This file is part of the 'stringi' library.
 * 
 * Copyright 2013 Marek Gagolewski, Bartek Tartanus, Marcin Bujarski
 * 
 * 'stringi' is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * 'stringi' is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with 'stringi'. If not, see <http://www.gnu.org/licenses/>.
 */
 
 
#include "stringi.h"





/** 
 * Get substring
 * 
 * 
 * @param str character vector
 * @param from integer vector (possibly with negative indices)
 * @param to integer vector (possibly with negative indices) or NULL
 * @param length integer vector or NULL
 * @return character vector
 * 
 * @version 0.1 (Bartek Tartanus)  stri_sub
 * @version 0.2 (Marek Gagolewski) use StriContainerUTF8 and stri__UChar32_to_UTF8_index
 */
SEXP stri_sub(SEXP str, SEXP from, SEXP to, SEXP length)
{
   str = stri_prepare_arg_string(str, "str");
   R_len_t str_n = LENGTH(str);
   
   int* from_tab    = 0;
   int* to_tab      = 0;
   int* length_tab  = 0;
   R_len_t from_n   = 0;
   R_len_t to_n     = 0;
   R_len_t length_n = 0;
   R_len_t nmax     = 0; 
   
   bool from_ismatrix = isMatrix(from);
   if (from_ismatrix) {
      SEXP t = getAttrib(from, R_DimSymbol);
      if (INTEGER(t)[1] == 1) from_ismatrix = false; // don't treat this as matrix
      else if (INTEGER(t)[1] > 2)
         error(MSG__ARG_EXPECTED_MATRIX_WITH_GIVEN_COLUMNS, "from", 2);
   }
   from = stri_prepare_arg_integer(from, "from");
   
   if (from_ismatrix) {
      from_n = to_n = LENGTH(from)/2;
      from_tab = INTEGER(from);
      to_tab = from_tab+from_n;
      nmax = stri__recycling_rule(true, 2, str_n, from_n);
   }
   else if (isNull(length)) {
      to = stri_prepare_arg_integer(to, "to");
      from_n = LENGTH(from);
      from_tab = INTEGER(from);
      to_n = LENGTH(to);
      to_tab = INTEGER(to);
      nmax = stri__recycling_rule(true, 3, str_n, from_n, to_n);
   }
   else {
      length = stri_prepare_arg_integer(length, "length");
      from_n = LENGTH(from);
      from_tab = INTEGER(from);
      length_n = LENGTH(length);
      length_tab = INTEGER(length);
      nmax = stri__recycling_rule(true, 3, str_n, from_n, length_n);
   }

   
   if (nmax <= 0)
      return allocVector(STRSXP,0);
      
   StriContainerUTF8* se = new StriContainerUTF8(str, nmax);
   
   // args prepared, let's go
   SEXP ret;
   PROTECT(ret = allocVector(STRSXP, nmax));
   
   const char* last_s = 0;
   R_len_t last_ind_fwd_codepoint = -1;
   R_len_t last_ind_fwd_utf8 = -1;
   R_len_t last_ind_back_codepoint = -1;
   R_len_t last_ind_back_utf8 = -1;
   
   for (R_len_t i = se->vectorize_init();
         i != se->vectorize_end();
         i = se->vectorize_next(i))
   {
      R_len_t cur_from     = from_tab[i % from_n];
      R_len_t cur_to       = (to_tab)?to_tab[i % to_n]:length_tab[i % length_n];
      if (se->isNA(i) || cur_from == NA_INTEGER || cur_to == NA_INTEGER) {
         SET_STRING_ELT(ret, i, NA_STRING);
         continue;
      }
      
      if (length_tab) {
         if (cur_to <= 0) {
            SET_STRING_ELT(ret, i, mkCharLen(NULL, 0));
            continue;
         }
         cur_to = cur_from + cur_to - 1;
         if (cur_from < 0 && cur_to >= 0) cur_to = -1;
      }
      
      const char* cur_s = se->get(i).c_str();
      R_len_t cur_n = se->get(i).length();
      
      // allows to continue search in the same string
      // speeds up if we search near the place where were recently
      if (last_s != cur_s) {
         last_s = cur_s;
         last_ind_fwd_codepoint = 0;
         last_ind_fwd_utf8 = 0;
         last_ind_back_codepoint = 0;
         last_ind_back_utf8 = cur_n;
      }
      


      R_len_t cur_from2; // UTF-8 byte incices
      R_len_t cur_to2;   // UTF-8 byte incices
      
      if (cur_from >= 0) {
         cur_from--; // 1-based -> 0-based index
         cur_from2 = stri__UChar32_to_UTF8_index_fwd(cur_s, cur_n,
            cur_from, last_ind_fwd_codepoint, last_ind_fwd_utf8);
            
         last_ind_fwd_codepoint = cur_from;
         last_ind_fwd_utf8 = cur_from2;
      }
      else {
         cur_from = -cur_from;
         cur_from2 = stri__UChar32_to_UTF8_index_back(cur_s, cur_n,
            cur_from, last_ind_back_codepoint, last_ind_back_utf8);
            
         last_ind_back_codepoint = cur_from;
         last_ind_back_utf8 = cur_from2;
      }
         
      if (cur_to >= 0) {
         ; // do nothing with cur_to // 1-based -> 0-based index but +1 as we need the next one (bound)
         cur_to2 = stri__UChar32_to_UTF8_index_fwd(cur_s, cur_n,
            cur_to, last_ind_fwd_codepoint, last_ind_fwd_utf8);
            
         last_ind_fwd_codepoint = cur_to;
         last_ind_fwd_utf8 = cur_to2;
      }
      else {
         cur_to = -cur_to - 1;
         cur_to2 = stri__UChar32_to_UTF8_index_back(cur_s, cur_n,
            cur_to, last_ind_back_codepoint, last_ind_back_utf8);
            
         last_ind_back_codepoint = cur_to;
         last_ind_back_utf8 = cur_to2;
      }
      
      if (cur_to2 >= cur_from2) { // just copy
         SET_STRING_ELT(ret, i, mkCharLenCE(cur_s+cur_from2, cur_to2-cur_from2, CE_UTF8));  
      }
      else {
         // maybe a warning here?
         SET_STRING_ELT(ret, i, mkCharLen(NULL, 0));
      }
   }
   
   delete se;
   UNPROTECT(1); 
   return ret;
}


/** Converts codepoint-based index to utf8-byte index [left-based index]
 * 
 * @param s string
 * @param n length of \code{s}, in bytes
 * @param i codepoint-based index to look for is \code{s}, 0-based
 * @param lasti (allow to continue last search - last codepoint-based index)
 * @param lastres (allow to continue last search - last search result)
 * 
 * @return utf8-base translated index \code{i}, 0-based
 * 
 * @version 0.1 (Bartek Tartanus)  stri_sub
 * @version 0.2 (Marek Gagolewski) stri__UChar32_to_UTF8_index
 */
R_len_t stri__UChar32_to_UTF8_index_fwd(const char* s, R_len_t n, R_len_t i,
                                    R_len_t lasti, R_len_t lastres)
{
   if (i <= 0) return 0;
   
   R_len_t j = 0;
   R_len_t jres = 0;
   
   if (lasti >= 0) {
      if (i < lasti) {
         // check if it makes sense to go backwards, or to start from scratch
         if ((lasti-i) < (i-0)) {
            // less code points will be considered when going backwards
            j    = lasti;
            jres = lastres;
            while (j > i && jres > 0) {
               U8_BACK_1((const uint8_t*)s, 0, jres);
               --j;
            }
            return jres; // stop right now
         }
         // else 
      }
      else if (i >= lasti) { // continue last search
         j    = lasti;
         jres = lastres;
      }
   }
   
   // go forward
   while (j < i && jres < n) {
      U8_FWD_1((const uint8_t*)s, jres, n);
      ++j;
   }
      
   return jres;
}



/** Converts codepoint-based index to utf8-byte index [right-based index]
 * 
 * @param s string
 * @param n length of \code{s}, in bytes
 * @param i codepoint-based index to look for is \code{s}, 0-based
 *    [0 == end, 1 == last char, 2 == second-to-last, etc.]
 * @param lasti (allow to continue last search - last codepoint-based index)
 * @param lastres (allow to continue last search - last search result)
 * 
 * @return utf8-base translated index \code{i}, 0-based
 * 
 * @version 0.1 (Bartek Tartanus)  stri_sub
 * @version 0.2 (Marek Gagolewski) stri__UChar32_to_UTF8_index
 */
R_len_t stri__UChar32_to_UTF8_index_back(const char* s, R_len_t n, R_len_t i,
                                    R_len_t lasti, R_len_t lastres)
{
   if (i <= 0) return n;
   
   R_len_t j = 0;
   R_len_t jres = n;
   
   if (lasti >= 0) {
      if (i < lasti) {
         // check if it makes sense to go forward, or to start from scratch
         if ((lasti-i) < (i-0)) {
            // less code points will be considered when going backwards
            j    = lasti;
            jres = lastres;
            while (j > i && jres < n) {
               U8_FWD_1((const uint8_t*)s, jres, n);
               --j;
            }
            return jres; // stop right now
         }
         // else 
      }
      else if (i >= lasti) { // continue last search
         j    = lasti;
         jres = lastres;
      }
   }
   
   // go backward
   while (j < i && jres > 0) {
      U8_BACK_1((const uint8_t*)s, 0, jres);
      ++j;
   }
      
   return jres;
}



/** 
 * Substring replacement function
 * 
 * @param s character vector ...
 * @param from integer vector ...
 * @param to integer vector ...
 * @param value string vector ...
 * @return character vector
 * 
 * @version 0.1 (Bartek Tartanus)
 */
SEXP stri_sub_op(SEXP s, SEXP from, SEXP to, SEXP value)
{
   s = stri_prepare_arg_string(s, "str");
   from = stri_prepare_arg_integer(from, "from");
   to = stri_prepare_arg_integer(to, "to");
   value = stri_prepare_arg_string(value, "value");
   R_len_t ns = LENGTH(s);
   R_len_t nfrom = LENGTH(from);
   R_len_t nto = LENGTH(to);
   R_len_t nval = LENGTH(value);
   
   if(ns == 0 || nfrom == 0 || nto == 0 || nval == 0)
      return allocVector(STRSXP,0);
   
   R_len_t nmax = stri__recycling_rule(true, 4, ns, nfrom, nto, nval);
   //idea to improve performance if ns << nmax
   //first - check every element of s and save position of every utf char
   //into where and now you can easliy get substring by where[from[i]]
   //int* where = (int*)R_alloc(curslen, sizeof(int));
   UChar32 c;
   SEXP e, curs, curval, count;
   //this line can be the first one to replace to improve performance 
   count = stri_length(s);
   int curfrom, curto, curslen, curvallen;
   PROTECT(e = allocVector(STRSXP, nmax));
   int j=0,k=0,lastk=0,st=0,curcount=0;
   for (int i = 0; i < nmax; ++i)
   {
      curs = STRING_ELT(s, i % ns);
      curslen = LENGTH(curs);
      curfrom = INTEGER(from)[i % nfrom];
      curto = INTEGER(to)[i % nto];
      curcount = INTEGER(count)[i % ns];
      curval = STRING_ELT(value, i % nval);
      curvallen = LENGTH(curval);
      //if string is NA, return NA
      if(curs == NA_STRING || curfrom == NA_INTEGER || curto == NA_INTEGER
         || curval == NA_STRING){
         SET_STRING_ELT(e, i, NA_STRING);
         continue;
      }
      //if from or to <0 then count from the end 
      if(curfrom < 0)
         curfrom += curcount + 1;
      if(curto < 0)
         curto += curcount + 1;
      //if from is greater than to or count then replace nothing - return
      //string with no change
      if(curfrom > curto || curfrom > curcount){
         SET_STRING_ELT(e, i, curs);
         continue;
      }
      j = 0; 
      lastk = 0;
      for(k=0; lastk < curslen; ++j){
         if(j==curfrom){
         //lastk is here, bacause without it you dont know if the last 
         //char is one or two byte long so k-1 doesnt work every time
            st=lastk;
            //if replace to the last char then copy now, dont waste time
            //if to > count replace the rest of a string
            if(curto >= curcount){
               char* buf = R_alloc(lastk+curvallen, sizeof(char)); 
               char* buf2 = buf;
               memcpy(buf2, CHAR(curs),lastk);
               buf2 += lastk;
               memcpy(buf2, CHAR(curval),curvallen);
               buf2 += curvallen;
               SET_STRING_ELT(e, i, mkCharLen(buf, lastk+curvallen));
               break;
            }
         }
         if(j==curto){
            char* buf = R_alloc(st+curvallen+curslen-k, sizeof(char)); 
            char* buf2 = buf;
            //copy the beginning of a string
            memcpy(buf2, CHAR(curs),st);
            buf2 += st;
            //now copy the new value in the middle
            memcpy(buf2, CHAR(curval),curvallen);
            buf2 += curvallen;
            //and copy the rest of a string
            memcpy(buf2, CHAR(curs)+k,curslen-k);
            buf2 += curslen-k;
            SET_STRING_ELT(e, i, mkCharLen(buf, st+curvallen+curslen-k));
            break;
         }
         lastk = k;
         U8_NEXT(CHAR(curs), k, curslen, c);
      }
   }
   UNPROTECT(1);
   return e;
}

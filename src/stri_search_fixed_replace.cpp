/* This file is part of the 'stringi' package for R.
 * Copyright (c) 2013-2014, Marek Gagolewski and Bartek Tartanus
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "stri_stringi.h"
#include "stri_container_utf8.h"
#include "stri_container_bytesearch.h"
#include "stri_string8buf.h"
#include "stri_interval.h"
#include <deque>
#include <queue>
#include <algorithm>
using namespace std;


/**
 * Replace all/first/last occurences of a fixed pattern
 *
 * @param str character vector
 * @param pattern character vector
 * @param replacement character vector
 * @return character vector
 *
 * @version 0.1-?? (Bartek Tartanus)
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-26)
 *          StriException friendly & Use StriContainers
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-07-10)
 *          BUGFIX: wrong behavior on empty str
 *
 * @version 0.2-3 (Marek Gagolewski, 2014-05-08)
 *          stri_replace_fixed now uses byte search only
 */
SEXP stri__replace_allfirstlast_fixed(SEXP str, SEXP pattern, SEXP replacement, int type)
{
   str          = stri_prepare_arg_string(str, "str");
   pattern      = stri_prepare_arg_string(pattern, "pattern");
   replacement  = stri_prepare_arg_string(replacement, "replacement");
   R_len_t vectorize_length = stri__recycling_rule(true, 3, LENGTH(str), LENGTH(pattern), LENGTH(replacement));

   STRI__ERROR_HANDLER_BEGIN
   StriContainerUTF8 str_cont(str, vectorize_length);
   StriContainerUTF8 replacement_cont(replacement, vectorize_length);
   StriContainerByteSearch pattern_cont(pattern, vectorize_length);

   SEXP ret;
   STRI__PROTECT(ret = Rf_allocVector(STRSXP, vectorize_length));

   String8buf buf(0); // @TODO: calculate buf len a priori?

   for (R_len_t i = pattern_cont.vectorize_init();
         i != pattern_cont.vectorize_end();
         i = pattern_cont.vectorize_next(i))
   {
      STRI__CONTINUE_ON_EMPTY_OR_NA_STR_PATTERN(str_cont, pattern_cont,
         SET_STRING_ELT(ret, i, NA_STRING);,
         SET_STRING_ELT(ret, i, Rf_mkCharLenCE(NULL, 0, CE_UTF8));)

      if (replacement_cont.isNA(i)) {
         SET_STRING_ELT(ret, i, NA_STRING);
         continue;
      }

      R_len_t start;
      if (type >= 0) { // first or all
         pattern_cont.setupMatcherFwd(i, str_cont.get(i).c_str(), str_cont.get(i).length());
         start = pattern_cont.findFirst();
      } else {
         pattern_cont.setupMatcherBack(i, str_cont.get(i).c_str(), str_cont.get(i).length());
         start = pattern_cont.findLast();
      }

      if (start == USEARCH_DONE) {
         SET_STRING_ELT(ret, i, str_cont.toR(i));
         continue;
      }

      R_len_t len = pattern_cont.getMatchedLength();
      R_len_t sumbytes = len;
      deque< pair<R_len_t, R_len_t> > occurences;
      occurences.push_back(pair<R_len_t, R_len_t>(start, start+len));

      if (type == 0) {
         while (USEARCH_DONE != pattern_cont.findNext()) { // all
            start = pattern_cont.getMatchedStart();
            len = pattern_cont.getMatchedLength();
            occurences.push_back(pair<R_len_t, R_len_t>(start, start+len));
            sumbytes += len;
         }
      }

      R_len_t str_cur_n     = str_cont.get(i).length();
      const char* str_cur_s = str_cont.get(i).c_str();
      R_len_t     replacement_cur_n = replacement_cont.get(i).length();
      const char* replacement_cur_s = replacement_cont.get(i).c_str();
      R_len_t buf_need =
         str_cur_n+replacement_cur_n*(R_len_t)occurences.size()-sumbytes;
      buf.resize(buf_need, false/*destroy contents*/);

      R_len_t jlast = 0;
      char* curbuf = buf.data();
      deque< pair<R_len_t, R_len_t> >::iterator iter = occurences.begin();
      for (; iter != occurences.end(); ++iter) {
         pair<R_len_t, R_len_t> match = *iter;
         memcpy(curbuf, str_cur_s+jlast, (size_t)match.first-jlast);
         curbuf += match.first-jlast;
         jlast = match.second;
         memcpy(curbuf, replacement_cur_s, (size_t)replacement_cur_n);
         curbuf += replacement_cur_n;
      }
      memcpy(curbuf, str_cur_s+jlast, (size_t)str_cur_n-jlast);
      SET_STRING_ELT(ret, i, Rf_mkCharLenCE(buf.data(), buf_need, CE_UTF8));
   }

   STRI__UNPROTECT_ALL
   return ret;
   STRI__ERROR_HANDLER_END(;/* nothing special to be done on error */)
}


/**
 * Replace all occurences of a fixed pattern; vectorize_all=FALSE
 *
 * @param str character vector
 * @param pattern character vector
 * @param replacement character vector
 * @return character vector
 * 
 * @version 0.3-1 (Marek Gagolewski, 2014-11-01)
 */
SEXP stri__replace_all_fixed_no_vectorize_all(SEXP str, SEXP pattern, SEXP replacement)
{
   str          = stri_prepare_arg_string(str, "str");
   pattern      = stri_prepare_arg_string(pattern, "pattern");
   replacement  = stri_prepare_arg_string(replacement, "replacement");

   R_len_t str_n = LENGTH(str);
   R_len_t pattern_n = LENGTH(pattern);
   R_len_t replacement_n = LENGTH(replacement);
   if (pattern_n < replacement_n || pattern_n <= 0 || replacement_n <= 0)
      Rf_error(MSG__WARN_RECYCLING_RULE2);
   if (pattern_n % replacement_n != 0)
      Rf_warning(MSG__WARN_RECYCLING_RULE);
   
   // if str_n is 0, then return an empty vector
   if (str_n <= 0)
      return stri__vector_empty_strings(0);

   STRI__ERROR_HANDLER_BEGIN
   StriContainerUTF8 str_cont(str, str_n);
   StriContainerUTF8 replacement_cont(replacement, pattern_n);
   StriContainerByteSearch pattern_cont(pattern, pattern_n);
   
   // if any of the patterns is missing, then return an NA vector
   // if a pattern is empty, throw an error
   for (R_len_t i=0; i<pattern_n; ++i) {
      if (pattern_cont.isNA(i))
         return stri__vector_NA_strings(str_n);
      if (pattern_cont.get(i).length() <= 0)
         throw StriException(MSG__EMPTY_SEARCH_PATTERN_UNSUPPORTED);
   }

   vector< deque< StriInterval<R_len_t> > > queues(str_n); // matches


   // get indices at which we have a pattern match
   // for each pattern, for each search string
   for (R_len_t i = pattern_cont.vectorize_init();
         i != pattern_cont.vectorize_end();
         i = pattern_cont.vectorize_next(i))
   {
      // current pattern is not NA and is not empty
      
      for (R_len_t j=0; j<str_n; ++j) {
         
         if (str_cont.isNA(j) || str_cont.get(j).length() <= 0)
            continue;
            
         R_len_t start;
         pattern_cont.setupMatcherFwd(i, str_cont.get(j).c_str(), str_cont.get(j).length());
         start = pattern_cont.findFirst();
         if (start == USEARCH_DONE)
            continue;
         R_len_t len = pattern_cont.getMatchedLength();
         queues[j].push_back(StriInterval<R_len_t>(start, start+len, i));
         
         while (USEARCH_DONE != pattern_cont.findNext()) {
            start = pattern_cont.getMatchedStart();
            len = pattern_cont.getMatchedLength();
            queues[j].push_back(StriInterval<R_len_t>(start, start+len, i));
         }
      }
   }
   
   // check if there are overlapping patterns,
   // determine max buf size
   R_len_t bufsize = 0;
   for (R_len_t i=0; i<str_n; ++i) {
      if (str_cont.isNA(i) || str_cont.get(i).length() <= 0)
            continue;
      if (!queues[i].size())
         continue; // no matches
      
      sort(queues[i].begin(), queues[i].end());
      
      R_len_t bufsize_cur = str_cont.get(i).length();
      deque< StriInterval<R_len_t> >::iterator iter = queues[i].begin();
      
      StriInterval<R_len_t> last_int = *(iter++);
      if (replacement_cont.isNA(last_int.data))
         continue; // an NA replacement found
         
      bufsize_cur = bufsize_cur - pattern_cont.get(last_int.data).length()
                                 + replacement_cont.get(last_int.data).length();
                                 
      bool willBeNA = false;
      for (; iter != queues[i].end(); ++iter) {
         StriInterval<R_len_t> cur_int = *iter;
         
         if (cur_int.a < last_int.b)
            throw StriException(MSG__OVERLAPPING_PATTERN_UNSUPPORTED);
         
         if (replacement_cont.isNA(cur_int.data)) {
            willBeNA = true;
            continue; // an NA replacement found
         }
         bufsize_cur = bufsize_cur - pattern_cont.get(cur_int.data).length() 
                                    + replacement_cont.get(cur_int.data).length();
         last_int = cur_int;
      }

      if (!willBeNA && bufsize < bufsize_cur) bufsize = bufsize_cur;
   }
     
   
   // construct the resulting vector
   SEXP ret;
   STRI__PROTECT(ret = Rf_allocVector(STRSXP, str_n));
   String8buf buf(bufsize);
   for (R_len_t i=0; i<str_n; ++i) {
      if (str_cont.isNA(i)) {
         SET_STRING_ELT(ret, i, NA_STRING);
         continue;
      }
      
      if (str_cont.get(i).length() <= 0 || !queues[i].size()) {
         SET_STRING_ELT(ret, i, str_cont.toR(i));
         continue;
      }
      
      // all right, at least one match - replace, captain!
      R_len_t bufused = 0;
      char* curbuf = buf.data();
      const char* str_cur_s = str_cont.get(i).c_str();
      R_len_t str_cur_n = str_cont.get(i).length();
     
      R_len_t last_b = 0;
      for (deque< StriInterval<R_len_t> >::iterator iter = queues[i].begin();
               iter != queues[i].end(); ++iter) {
         StriInterval<R_len_t> cur_int = *iter;
         if (replacement_cont.isNA(cur_int.data)) {
            SET_STRING_ELT(ret, i, NA_STRING);
            break;
         }
         
         memcpy(curbuf+bufused, str_cur_s+last_b, cur_int.a-last_b);
         bufused += (cur_int.a-last_b);
         memcpy(curbuf+bufused, replacement_cont.get(cur_int.data).c_str(), replacement_cont.get(cur_int.data).length());
         bufused += replacement_cont.get(cur_int.data).length();
         last_b = cur_int.b;
      }
      
      if (STRING_ELT(ret, i) != NA_STRING) {
         memcpy(curbuf+bufused, str_cur_s+last_b, str_cur_n-last_b);
         bufused += (str_cur_n-last_b);
         SET_STRING_ELT(ret, i, Rf_mkCharLenCE(buf.data(), bufused, CE_UTF8));
      }
   }

   STRI__UNPROTECT_ALL
   return ret;
   STRI__ERROR_HANDLER_END(;/* nothing special to be done on error */)
}


/**
 * Replace all occurences of a fixed pattern
 *
 * @param str character vector
 * @param pattern character vector
 * @param replacement character vector
 * @param vectorize_all single logical value
 * @return character vector
 *
 * @version 0.1-?? (Bartek Tartanus)
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-26)
 *          use stri__replace_allfirstlast_fixed
 *
 * @version 0.2-3 (Marek Gagolewski, 2014-05-08)
 *          stri_replace_fixed now uses byte search only
 * 
 * @version 0.3-1 (Marek Gagolewski, 2014-11-01)
 *          vectorize_all argument added
 */
SEXP stri_replace_all_fixed(SEXP str, SEXP pattern, SEXP replacement, SEXP vectorize_all)
{
   if (stri__prepare_arg_logical_1_notNA(vectorize_all, "vectorize_all"))
      return stri__replace_allfirstlast_fixed(str, pattern, replacement, 0);
   else
      return stri__replace_all_fixed_no_vectorize_all(str, pattern, replacement);
}


/**
 * Replace last occurence of a fixed pattern
 *
 * @param str character vector
 * @param pattern character vector
 * @param replacement character vector
 * @return character vector
 *
 * @version 0.1-?? (Bartek Tartanus)
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-26)
 *          use stri__replace_allfirstlast_fixed
 *
 * @version 0.2-3 (Marek Gagolewski, 2014-05-08)
 *          stri_replace_fixed now uses byte search only
 */
SEXP stri_replace_last_fixed(SEXP str, SEXP pattern, SEXP replacement)
{
   return stri__replace_allfirstlast_fixed(str, pattern, replacement, -1);
}


/**
 * Replace first occurence of a fixed pattern
 *
 * @param str character vector
 * @param pattern character vector
 * @param replacement character vector
 * @return character vector
 *
 * @version 0.1-?? (Bartek Tartanus)
 *
 * @version 0.1-?? (Marek Gagolewski, 2013-06-26)
 *          use stri__replace_allfirstlast_fixed
 *
 * @version 0.2-3 (Marek Gagolewski, 2014-05-08)
 *          stri_replace_fixed now uses byte search only
 */
SEXP stri_replace_first_fixed(SEXP str, SEXP pattern, SEXP replacement)
{
   return stri__replace_allfirstlast_fixed(str, pattern, replacement, 1);
}

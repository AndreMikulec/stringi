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
   vectorized over s
   if s is NA the result will be NA
   
   TO DO: Encoding marking!
*/
SEXP stri_trim(SEXP s)
{
   s = stri_prepare_arg_string(s); // prepare string argument
   
   R_len_t ns = LENGTH(s);
   SEXP e;
   PROTECT(e = allocVector(STRSXP, ns));
   int j=0,k=0;
   
   for (int i=0; i<ns; ++i)
   {
      SEXP ss = STRING_ELT(s, i);
      if (ss == NA_STRING)
         SET_STRING_ELT(e, i, NA_STRING);
      else {
         const char* string = CHAR(ss);
         int nstring = LENGTH(ss);
         for(j=0; j < nstring ; ++j){
            if(string[j] != ' ')
               break;
         }
         for(k=0; k < nstring ; ++k){
            if(string[nstring-1-k] != ' ')
               break;
         }
         //if string contains only space, then k+j > nstring and mkCharLen
         //throws an error (negative len). That's why max() is needed here
         SET_STRING_ELT(e, i, mkCharLen(string+j, max(0,nstring-k-j)));
      }
   }
   UNPROTECT(1);
   return e;
}




/** 
   vectorized over s
   if s is NA the result will be NA
   
*/
SEXP stri_ltrim(SEXP s)
{   
   s = stri_prepare_arg_string(s); // prepare string argument
   
   R_len_t ns = LENGTH(s);
   SEXP e;
   PROTECT(e = allocVector(STRSXP, ns));
   int j=0;
   
   for (int i=0; i<ns; ++i)
   {
      SEXP ss = STRING_ELT(s, i);
      if (ss == NA_STRING)
         SET_STRING_ELT(e, i, NA_STRING);
      else {
         const char* string = CHAR(ss);
         int nstring = LENGTH(ss);
         for(j=0; j < nstring ; ++j){
            if(string[j] != ' ')
               break;
         }
         SET_STRING_ELT(e, i, mkCharLen(string+j,nstring-j));
      }
   }
   UNPROTECT(1);
   return e;
}


/** 
   vectorized over s
   if s is NA the result will be NA
   
*/
SEXP stri_rtrim(SEXP s)
{   
   s = stri_prepare_arg_string(s); // prepare string argument
   
   R_len_t ns = LENGTH(s);
   SEXP e;
   PROTECT(e = allocVector(STRSXP, ns));
   int j=0;
   
   for (int i=0; i<ns; ++i)
   {
      SEXP ss = STRING_ELT(s, i);
      if (ss == NA_STRING)
         SET_STRING_ELT(e, i, NA_STRING);
      else {
         const char* string = CHAR(ss);
         int nstring = LENGTH(ss);
         for(j=0; j < nstring ; ++j){
            if(string[nstring-1-j] != ' ')
               break;
         }
         SET_STRING_ELT(e, i, mkCharLen(string,nstring-j));
      }
   }
   UNPROTECT(1);
   return e;
}



/** 
   vectorized over s
   if s is NA the result will be NA
   
*/
SEXP stri_trim_all(SEXP s)
{
   s = stri_prepare_arg_string(s); // prepare string argument
   R_len_t ns = LENGTH(s);
   SEXP e,subs, curs, temp, temp2, white, space;
   PROTECT(e = allocVector(STRSXP, ns));
   PROTECT(white = allocVector(STRSXP, 1));
   PROTECT(space = allocVector(STRSXP, 1));
   SET_STRING_ELT(white,0,mkCharLen("^WHITE_SPACE",12));
   SET_STRING_ELT(space,0,mkCharLen(" ",1));
   subs = stri_locate_all_class(s, stri_char_getpropertyid(white));
   int n;
   for(int i=0; i < ns; ++i){
      curs = STRING_ELT(s, i);
      if(curs == NA_STRING){
         SET_STRING_ELT(e, i, NA_STRING);
         continue;
      }
      temp = VECTOR_ELT(subs, 0);
      n = LENGTH(temp)/2;
      int* fromto = INTEGER(temp);
      //if from==NA then string contains only white space -> return empty
      if(fromto[0] == NA_INTEGER){
         SET_STRING_ELT(e, i, mkCharLen("",0));
         continue;
      }
      temp2 = stri__split_pos(CHAR(curs), fromto, fromto + n,LENGTH(curs),n);
      SET_STRING_ELT(e, i,STRING_ELT(stri_flatten(temp2, space),0));
   }
   UNPROTECT(3);
   return e;
}


/** 
   vectorized over s, width and side
   if s is NA the result will be NA
   
*/

SEXP stri_pad(SEXP s, SEXP width, SEXP side, SEXP pad)
{
   s = stri_prepare_arg_string(s); // prepare string argument
   width = stri_prepare_arg_integer(width);
   pad = stri_prepare_arg_string(pad);
   
   R_len_t ns = LENGTH(s);
   R_len_t nside = LENGTH(side);
   R_len_t nwidth = LENGTH(width);
   
   if(INTEGER(stri_length(pad))[0] != 1) 
      error("pad must be single character");
   
   R_len_t nmax = stri__recycling_rule(true, 3, ns, nside, nwidth);
   
   int needed=0;
   SEXP e, curs, slen;
   PROTECT(e = allocVector(STRSXP, nmax));
   
   const char* p = CHAR(STRING_ELT(pad,0));
   
   slen = stri_length(s);
   int* iwidth = INTEGER(width);
   int* islen  = INTEGER(slen);
   int* iside  = INTEGER(side);
   
   for (int i=0; i<nmax; ++i){
      curs = STRING_ELT(s, i % ns);
      if(curs == NA_STRING || iwidth[i % nwidth] == NA_INTEGER){
         SET_STRING_ELT(e, i, NA_STRING);
         continue;
      }
      
      needed = max(0, iwidth[i % nwidth] - islen[i % ns]);
      if(needed == 0){
         SET_STRING_ELT(e, i, curs);
         continue;
      }
      char* buf = R_alloc(iwidth[i % nwidth], sizeof(char)); 
      char* buf2 = buf;
      switch(iside[i % nside]){
         //left
         case 1:
         for(int j=0; j<needed; ++j){
            memcpy(buf2, p, 1);
            buf2 += 1;
         }
         memcpy(buf2, CHAR(curs), islen[i % ns]);
         break;
         //right
         case 2:
         memcpy(buf2, CHAR(curs), islen[i % ns]);
         buf2 += islen[i % ns];
         for(int j=0; j<needed; ++j){
            memcpy(buf2, p, 1);
            buf2 += 1;
         }
         break;
         //both
         case 3:
         for(int j=0; j<floor(needed/2); ++j){
            memcpy(buf2, p, 1);
            buf2 += 1;
         }
         memcpy(buf2, CHAR(curs), islen[i % ns]);
         buf2 += islen[i % ns];
         for(int j=0; j<ceil(double(needed)/2); ++j){
            memcpy(buf2, p, 1);
            buf2 += 1;
         }
         break;
      }
      SET_STRING_ELT(e, i, mkCharLen(buf, iwidth[i % nwidth]));
   }
   
   UNPROTECT(1);
   return e;
}
/* This file is part of the 'stringi' library.
 * 
 * Copyright 2013 Marek Gagolewski, Bartek Tartanus
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
   
   TO DO: Encoding marking!!
   TO DO: USE C API (no UnicodeString....)
*/
SEXP stri_unicode_normalization(SEXP s, SEXP type)
{
   s = stri_prepare_arg_string(s);        // prepare string argument
   type = stri_prepare_arg_integer(type); // prepare int argument
   
   UErrorCode err = U_ZERO_ERROR;
   const Normalizer2* nfc = NULL;
   int _type = INTEGER(type)[0];
   switch (_type) {
      case 10:
         nfc = Normalizer2::getNFCInstance(err);
         break;
      case 20:
         nfc = Normalizer2::getNFDInstance(err);
         break;
      case 11:
         nfc = Normalizer2::getNFKCInstance(err);
         break;
      case 21:
         nfc = Normalizer2::getNFKDInstance(err);
         break;
      case 12:
         nfc = Normalizer2::getNFKCCasefoldInstance(err);
         break;
      default:
         error("stri_unicode_normalization: incorrect Unicode normalization type");
   }
   if (U_FAILURE(err)) {
      error("ICU4R: could not get Normalizer2 instance");
   }
   
   R_len_t ns = LENGTH(s);
   SEXP e;   
   PROTECT(e = allocVector(STRSXP, ns));
   string y;
   UnicodeString out;
   
   for (int i=0; i<ns; ++i)
   {
      SEXP ss = STRING_ELT(s, i);
      if (ss == NA_STRING)
         SET_STRING_ELT(e, i, NA_STRING);
      else {
         UnicodeString xu = UnicodeString::fromUTF8(StringPiece(CHAR(ss)));
         err = U_ZERO_ERROR;
         nfc->normalize(xu, out, err);
         if (U_FAILURE(err)) {
            error("ICU4R: could not normalize a string with current Normalizer2 instance");
         }
         out.toUTF8String(y);
         SET_STRING_ELT(e, i, mkCharLen(y.c_str(), y.length()));
         if (i < ns-1) y.clear();
      }
   }
   UNPROTECT(1);
   return e;
}


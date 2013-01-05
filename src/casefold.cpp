/** This file is part of the 'stringi' library.
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




/* 
   vectorized over s
   if s is NA the result will be NA
   
   TO DO: WHAT IF s is not in UTF-8??? Encoding marking!
   TO DO: USE C API (no UnicodeString....)
*/
SEXP stri_casefold(SEXP s, SEXP type)
{
   int _type = INTEGER(type)[0];
   R_len_t ns = LENGTH(s);
   SEXP e;   
   PROTECT(e = allocVector(STRSXP, ns));
   string y;

   for (int i=0; i<ns; ++i)
   {
      SEXP ss = STRING_ELT(s, i);
      if (ss == NA_STRING)
         SET_STRING_ELT(e, i, NA_STRING);
      else {
         UnicodeString xu = UnicodeString::fromUTF8(StringPiece(CHAR(ss)));
         switch (_type) {
            case 1:
               xu.toLower();
               break;
            case 2:
               xu.toUpper();
               break;
            case 3:
               error("stri_case: toTitle() has not been implemented yet");
//               xu.toTitle();
               break;
            case 4:
               xu.foldCase(U_FOLD_CASE_DEFAULT);
               break;
            case 5:
               xu.foldCase(U_FOLD_CASE_EXCLUDE_SPECIAL_I);
               break;
            default:
               error("stri_case: incorrect case conversion type");
         }
         xu.toUTF8String(y);
         SET_STRING_ELT(e, i, mkCharLen(y.c_str(), y.length()));
         if (i < ns-1) y.clear();
      }
   }
   UNPROTECT(1);
   return e;
}

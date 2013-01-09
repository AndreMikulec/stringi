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

/** Get curent-default ICU locale information
 *  @return an R named list
*/
SEXP stri_getinfo()
{
   const R_len_t infosize = 6;
   SEXP vals;
   SEXP names;
   PROTECT(names = allocVector(STRSXP, infosize));
   SET_STRING_ELT(names, 0, mkChar("Locale.language"));
   SET_STRING_ELT(names, 1, mkChar("Locale.country"));  
   SET_STRING_ELT(names, 2, mkChar("Locale.variant"));   
   SET_STRING_ELT(names, 3, mkChar("Locale.name"));
   SET_STRING_ELT(names, 4, mkChar("Charset.internal"));
   SET_STRING_ELT(names, 5, mkChar("Charset.native"));
   
   PROTECT(vals = allocVector(VECSXP, infosize));
   for (int i=0; i<infosize; ++i) 
      SET_VECTOR_ELT(vals, i, ScalarString(NA_STRING));
      
   Locale loc = Locale::getDefault();
   SET_VECTOR_ELT(vals, 0, mkString(loc.getLanguage()));
   SET_VECTOR_ELT(vals, 1, mkString(loc.getCountry()));
   SET_VECTOR_ELT(vals, 2, mkString(loc.getVariant()));
   SET_VECTOR_ELT(vals, 3, mkString(loc.getName()));
   SET_VECTOR_ELT(vals, 4, mkString("UTF-8"));
   SET_VECTOR_ELT(vals, 5, stri_ucnv_encinfo(R_NilValue));

   setAttrib(vals, R_NamesSymbol, names);
   UNPROTECT(2);
   return vals;
}

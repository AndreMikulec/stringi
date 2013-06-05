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




///** 
// * Pad a string
// * 
// * vectorized over s, width and side
// *  if s is NA the result will be NA
// *
// * @param s
// * @param width
// * @param side
// * @param pad
// * @return character vector
// * 
// * @version 0.1 (Bartek Tartanus)
//*/
//SEXP stri_pad(SEXP s, SEXP width, SEXP side, SEXP pad)
//{
//   s     = stri_prepare_arg_string(s, "str"); // prepare string argument
//   width = stri_prepare_arg_integer(width, "width");
//   side  = stri_prepare_arg_integer(side, "side");
//   pad   = stri_prepare_arg_string(pad, "pad");
//   
//   R_len_t ns     = LENGTH(s);
//   R_len_t nside  = LENGTH(side);
//   R_len_t nwidth = LENGTH(width);
//   R_len_t np     = LENGTH(pad);
//   
//   //check if pad is single character
//   int* iplen = INTEGER(stri_length(pad));
//   for(R_len_t i=0; i<np; ++i){
//      if(iplen[i] != 1) 
//         error("pad must be single character");
//   }
//   
//   R_len_t nmax = stri__recycling_rule(true, 4, ns, nside, nwidth, np);
//   
//   int needed=0;
//   SEXP ret, curs;
//   PROTECT(ret = allocVector(STRSXP, nmax));
//   
//   int* iwidth = INTEGER(width);
//   int* iside  = INTEGER(side);
//   int* islen  = INTEGER(stri_length(s));
//   int* isnum  = INTEGER(stri_numbytes(s));
//   int* ipnum  = INTEGER(stri_numbytes(pad));
//   
//   for (R_len_t i=0; i<nmax; ++i){
//      curs = STRING_ELT(s, i % ns);
//      const char* p = CHAR(STRING_ELT(pad, i % np));
//      if(curs == NA_STRING || iwidth[i % nwidth] == NA_INTEGER){
//         SET_STRING_ELT(ret, i, NA_STRING);
//         continue;
//      }
//      //if current string is long enough - return with no change
//      needed = max(0, iwidth[i % nwidth] - islen[i % ns]);
//      if(needed == 0){
//         SET_STRING_ELT(ret, i, curs);
//         continue;
//      }
//      char* buf = R_alloc(isnum[i%ns]+ipnum[i%np]*needed, sizeof(char)); 
//      char* buf2 = buf;
//      switch(iside[i % nside]){
//         //pad from left
//         case 1:
//         for(int j=0; j<needed; ++j){
//            memcpy(buf2, p, ipnum[i%np]);
//            buf2 += ipnum[i%np];
//         }
//         memcpy(buf2, CHAR(curs), isnum[i % ns]);
//         break;
//         //right
//         case 2:
//         memcpy(buf2, CHAR(curs), isnum[i % ns]);
//         buf2 += isnum[i % ns];
//         for(int j=0; j<needed; ++j){
//            memcpy(buf2, p, ipnum[i%np]);
//            buf2 += ipnum[i%np];
//         }
//         break;
//         //both
//         case 3:
//         for(int j=0; j<floor(needed/2); ++j){
//            memcpy(buf2, p, ipnum[i%np]);
//            buf2 += ipnum[i%np];
//         }
//         memcpy(buf2, CHAR(curs), isnum[i % ns]);
//         buf2 += isnum[i % ns];
//         for(int j=0; j<ceil(double(needed)/2); ++j){
//            memcpy(buf2, p, ipnum[i%np]);
//            buf2 += ipnum[i%np];
//         }
//         break;
//      }
//      SET_STRING_ELT(ret, i, mkCharLen(buf, isnum[i%ns]+ipnum[i%np]*needed));
//   }
//   
//   UNPROTECT(1);
//   return ret;
//}

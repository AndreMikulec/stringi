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
 * Default constructor
 * 
 */
StriContainerUTF8::StriContainerUTF8()
   : StriContainerUTF_Base()
{
   this->str = NULL;
}


/**
 * Construct String Container from R character vector
 * @param rstr R character vector
 * @param nrecycle extend length [vectorization]
 * @param shallowrecycle will \code{this->str} be ever modified?
 */
StriContainerUTF8::StriContainerUTF8(SEXP rstr, R_len_t nrecycle, bool shallowrecycle)
   : StriContainerUTF_Base()
{
#ifndef NDEBUG 
   if (!isString(rstr))
      error("DEBUG: !isString in StriContainerUTF8::StriContainerUTF8(SEXP rstr)");
#endif
   this->n = LENGTH(rstr);
   this->isShallow = shallowrecycle;
   
   if (nrecycle == 0) this->n = 0;
#ifndef NDEBUG 
   if (this->n > nrecycle) error("DEBUG: n>nrecycle");
#endif
   this->nrecycle = nrecycle;
   
   if (this->n <= 0) {
      this->enc = NULL;
      this->str = NULL;
   }
   else {
      this->enc = new StriEnc[(this->isShallow)?this->n:this->nrecycle];
      this->str = new String8*[(this->isShallow)?this->n:this->nrecycle];
      for (R_len_t i=0; i<this->n; ++i)
         this->str[i] = NULL; // in case it fails during conversion
         
      UConverter* ucnvLatin1 = NULL;
      UConverter* ucnvNative = NULL;
      int bufsize = -1;
      char* buf = NULL;
         
      for (R_len_t i=0; i<this->n; ++i) {
         SEXP curs = STRING_ELT(rstr, i);
         if (curs == NA_STRING) {
            this->enc[i] = STRI_NA; 
            this->str[i] = NULL;
         }
         else {
            if (IS_ASCII(curs)) { // ASCII - ultra fast 
               this->str[i] = new String8(CHAR(curs), LENGTH(curs), !this->isShallow);
               this->enc[i] = STRI_ENC_ASCII; 
            }
            else if (IS_UTF8(curs)) { // UTF-8 - ultra fast 
               this->str[i] = new String8(CHAR(curs), LENGTH(curs), !this->isShallow);
               this->enc[i] = STRI_ENC_UTF8; 
            }
            else if (IS_BYTES(curs)) 
               error(MSG__BYTESENC);
            else {
//             LATIN1 ------- OR ------ Any encoding - detection needed  
//             Assume it's Native; this assumes the user working in an 8-bit environment
//             would convert strings to UTF-8 manually if needed - I think is's
//             a more reasonable approach (Native --> input via keyboard)


// version 0.1 - through UnicodeString & std::string
//               if (!ucnvNative) ucnvNative = stri__ucnv_open((char*)NULL);
//               UErrorCode status = U_ZERO_ERROR;
//               UnicodeString tmp(CHAR(curs), LENGTH(curs), ucnvNative, status);
//               if (U_FAILURE(status))
//                  error(MSG__ENC_ERROR_CONVERT); 
//               std::string tmp2;
//               tmp.toUTF8String(tmp2);
//               this->str[i] = new String8(tmp2.c_str(), tmp2.size(), true);
//               this->enc[i] = STRI_ENC_NATIVE; 

// version 0.2 - faster - through UnicodeString & u_strToUTF8
               UConverter* ucnvCurrent;
               if (IS_LATIN1(curs)) {
                  if (!ucnvLatin1) ucnvLatin1 = stri__ucnv_open("ISO-8859-1");
                  ucnvCurrent = ucnvLatin1;
               }
               else {
                  if (!ucnvNative) ucnvNative = stri__ucnv_open((char*)NULL);
                  ucnvCurrent = ucnvNative;
               }
               UErrorCode status = U_ZERO_ERROR;
               UnicodeString tmp(CHAR(curs), LENGTH(curs), ucnvCurrent, status);
               if (!U_SUCCESS(status))
                  error(MSG__ENC_ERROR_CONVERT);
               
               if (!buf) {
                  // calculate max string length
                  R_len_t maxlen = 0;
                  for (R_len_t z=i; z<this->n; ++z) { // start from current string (this wasn't needed before)
                     SEXP tmps = STRING_ELT(rstr, z);
                     if ((tmps != NA_STRING) && (maxlen < LENGTH(tmps))) 
                        maxlen = LENGTH(tmps);
                  }
                  bufsize = maxlen*3+1; // 1 UChar -> max 3 UTF8 bytes
                  buf = new char[bufsize];
               }
               int realsize = 0;
               
               u_strToUTF8(buf, bufsize, &realsize,
               		tmp.getBuffer(), tmp.length(), &status);
               if (!U_SUCCESS(status))
                  error(MSG__ENC_ERROR_CONVERT);
                  
               this->str[i] = new String8(buf, realsize, true);
               this->enc[i] = STRI_ENC_NATIVE; 
            }
         }
      }
      
      if (ucnvLatin1) ucnv_close(ucnvLatin1);
      if (ucnvNative) ucnv_close(ucnvNative);
      if (buf) delete [] buf;


      if (!this->isShallow) {
         for (R_len_t i=this->n; i<this->nrecycle; ++i) {
            this->enc[i] = this->enc[i%this->n];
            if (this->enc[i] == STRI_NA)
               this->str[i] = NULL;
            else
               this->str[i] = new String8(*this->str[i%this->n]);
         }
         this->n = this->nrecycle;
      }
   }
}


StriContainerUTF8::StriContainerUTF8(StriContainerUTF8& container)
   :    StriContainerUTF_Base((StriContainerUTF_Base&)container)
{
   if (this->n > 0) {
      this->str = new String8*[this->n];
      for (int i=0; i<this->n; ++i) {
         if (this->str[i])
            this->str[i] = new String8(*(container.str[i]));
         else
            this->str[i] = NULL;
      }
   }
   else {
      this->str = NULL;
   }
}




StriContainerUTF8& StriContainerUTF8::operator=(StriContainerUTF8& container)
{
   this->~StriContainerUTF8();
   (StriContainerUTF_Base&) (*this) = (StriContainerUTF_Base&)container;

   if (this->n > 0) {
      this->str = new String8*[this->n];
      for (int i=0; i<this->n; ++i) {
         if (this->str[i])
            this->str[i] = new String8(*(container.str[i]));
         else
            this->str[i] = NULL;
      }
   }
   else {
      this->str = NULL;
   }
   return *this;
}



StriContainerUTF8::~StriContainerUTF8()
{
   if (this->n > 0) {
      for (int i=0; i<this->n; ++i) {
         if (this->str[i])
            delete this->str[i];
      }
      delete [] this->str;
   }

   this->str = NULL;
}




/** Export character vector to R
 *  THE OUTPUT IS ALWAYS IN UTF-8
 *  Recycle rule is applied, so length == nrecycle
 */
SEXP StriContainerUTF8::toR() const
{
   SEXP ret;   
   PROTECT(ret = allocVector(STRSXP, this->nrecycle));
   
   for (R_len_t i=0; i<this->nrecycle; ++i) {
      if (!this->str[i%n])
         SET_STRING_ELT(ret, i, NA_STRING);
      else {
         SET_STRING_ELT(ret, i,
            mkCharLenCE(this->str[i%n]->c_str(), this->str[i%n]->length(), CE_UTF8));
      }
   }
   
   UNPROTECT(1);
   return ret;
}



/** Export string to R
 *  THE OUTPUT IS ALWAYS IN UTF-8
 *  @param i index [with recycle]
 */
SEXP StriContainerUTF8::toR(R_len_t i) const
{
#ifndef NDEBUG
   if (i < 0 || i >= nrecycle) error("toR: INDEX OUT OF BOUNDS");
#endif

   switch (this->enc[i%n]) {
      case STRI_NA:
         return NA_STRING;
         
      default:
      {
         // This is already in UTF-8
         return mkCharLenCE(this->str[i%n]->c_str(), this->str[i%n]->length(), CE_UTF8);
      }
   } 
}
 
 





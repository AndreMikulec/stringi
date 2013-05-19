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



/** Get Desired Normalizer2 instance
 * 
 * @param type R object, will be tested whether it's an integer vector of length 1
 * @return unmodifiable singleton instance. Do not delete it.
 * 
 * @version 0.1 (Marek Gagolewski)
 */
const Normalizer2* stri__normalizer_get(SEXP type)
{
   if (!isInteger(type) || LENGTH(type) != 1)
      error(MSG__INCORRECT_INTERNAL_ARG); // this is an internal arg, check manually   
   int _type = INTEGER(type)[0];
   
   UErrorCode status = U_ZERO_ERROR;
   const Normalizer2* normalizer = NULL;

   switch (_type) {
      case STRI_NFC:
         normalizer = Normalizer2::getNFCInstance(status);
         break;
         
      case STRI_NFD:
         normalizer = Normalizer2::getNFDInstance(status);
         break;
         
      case STRI_NFKC:
         normalizer = Normalizer2::getNFKCInstance(status);
         break;
         
      case STRI_NFKD:
         normalizer = Normalizer2::getNFKDInstance(status);
         break;
         
      case STRI_NFKC_CASEFOLD:
         normalizer = Normalizer2::getNFKCCasefoldInstance(status);
         break;
         
      default:
         error(MSG__INCORRECT_INTERNAL_ARG);
   }
   
   return normalizer;
}

   


/** 
 * Perform Unicode Normalization
 * 
 * @param str character vector
 * @param type normalization type [internal]
 * @return character vector
 * @version 0.1 (Marek Gagolewski)
 * @version 0.2 (Marek Gagolewski) - use StriContainerUTF16 & ICU facilities
 */
SEXP stri_enc_nf(SEXP str, SEXP type)
{
   str = stri_prepare_arg_string(str, "str");    // prepare string argument
   const Normalizer2* normalizer =
      stri__normalizer_get(type); // auto `type` check here

   
   StriContainerUTF16* ss = new StriContainerUTF16(str, LENGTH(str), false); // writable, no recycle

   for (R_len_t i = ss->vectorize_init();
         i != ss->vectorize_end();
         i = ss->vectorize_next(i))
   {
      if (!ss->isNA(i)) {
         UErrorCode status = U_ZERO_ERROR;
         ss->set(i, normalizer->normalize(ss->get(i), status));
         if (U_FAILURE(status))
            error(MSG__RESOURCE_ERROR_APPLY);
      }
   }
   
   SEXP ret;
   PROTECT(ret = ss->toR());
   delete ss;
   // normalizer shall not be deleted at all
   UNPROTECT(1);
   return ret;
}


/** 
 * Check if String is Normalized
 * 
 * @param str character vector
 * @param type normalization type [internal]
 * @return logical vector
 * @version 0.1 (Marek Gagolewski) - use StriContainerUTF16
 */
SEXP stri_enc_isnf(SEXP str, SEXP type)
{
   str = stri_prepare_arg_string(str, "str");    // prepare string argument
   const Normalizer2* normalizer =
      stri__normalizer_get(type); // auto `type` check here

   R_len_t nstr = LENGTH(str);
   
   SEXP ret;
   PROTECT(ret = allocVector(LGLSXP, nstr));
   int* retlog = LOGICAL(ret);
   
   StriContainerUTF16* ss = new StriContainerUTF16(str, nstr);

   for (R_len_t i = ss->vectorize_init();
         i != ss->vectorize_end();
         i = ss->vectorize_next(i))
   {
      if (!ss->isNA(i)) {
         UErrorCode status = U_ZERO_ERROR;
         retlog[i] = normalizer->isNormalized(ss->get(i), status);
         if (U_FAILURE(status))
            error(MSG__RESOURCE_ERROR_APPLY);
      }
      else
         retlog[i] = NA_LOGICAL;
   }
   

   delete ss;
   // normalizer shall not be deleted at all
   UNPROTECT(1);
   return ret;
}


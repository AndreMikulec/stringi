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


/** Check if a string may be valid 8-bit (including UTF-8) encoded
 *
 *  simple check whether all charcodes are nonzero
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Bartek Tartanus)
 * @version 0.2 (Marek Gagolewski, 2013-08-06) separate func
 * @version 0.3 (Marek Gagolewski, 2013-08-13) warnchars count added
 */
double stri__enc_check_8bit(const char* str_cur_s, R_len_t str_cur_n, bool get_confidence) {
   R_len_t warnchars = 0;
   for (R_len_t j=0; j < str_cur_n; ++j) {
      if (str_cur_s[j] == 0)
         return 0.0;
      if (get_confidence && (str_cur_s[j] <= 31 || str_cur_s[j] == 127)) {
         switch (str_cur_s[j]) {
            case 9:  // \t
            case 10: // \n
            case 13: // \r
            case 26: // ASCII SUBSTITUTE
               break; // ignore
            default:
               warnchars++;
         }
      }
   }
   return (get_confidence?(double)warnchars/double(str_cur_n):1.0);
}


/** Check if a string is valid ASCII
 *
 *  simple check whether charcodes are in [1..127]
 * by using U8_IS_SINGLE
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Bartek Tartanus)
 * @version 0.2 (Marek Gagolewski, 2013-08-06) separate func
 * @version 0.3 (Marek Gagolewski, 2013-08-13) warnchars count added
 */
double stri__enc_check_ascii(const char* str_cur_s, R_len_t str_cur_n, bool get_confidence) {
   R_len_t warnchars = 0;
   for (R_len_t j=0; j < str_cur_n; ++j) {
      if (!U8_IS_SINGLE(str_cur_s[j]) || str_cur_s[j] == 0) // i.e. 0 < c <= 127
         return 0.0;
      if (get_confidence && (str_cur_s[j] <= 31 || str_cur_s[j] == 127)) {
         switch (str_cur_s[j]) {
            case 9:  // \t
            case 10: // \n
            case 13: // \r
            case 26: // ASCII SUBSTITUTE
               break; // ignore
            default:
               warnchars++;
         }
      }
   }
   return (get_confidence?(double)(str_cur_n-warnchars)/double(str_cur_n):1.0);
}



/** Check if a string is valid UTF-8
 *
 * checks if a string is probably UTF-8-encoded;
 * simple check with U8_NEXT
 *
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Bartek Tartanus)
 * @version 0.2 (Marek Gagolewski, 2013-08-06) separate func
 * @version 0.3 (Marek Gagolewski, 2013-08-13) confidence calculation basing on ICU's i18n/csrutf8.cpp
 */
double stri__enc_check_utf8(const char* str_cur_s, R_len_t str_cur_n, bool get_confidence)
{
   if (!get_confidence) {
      UChar32 c;
      for (R_len_t j=0; j < str_cur_n; ) {
         if (str_cur_s[j] == 0)
            return 0.0; // definitely not valid UTF-8

         U8_NEXT(str_cur_s, j, str_cur_n, c);
         if (c < 0) // ICU utf8.h doc for U8_NEXT: c -> output UChar32 variable, set to <0 in case of an error
            return 0.0; // definitely not valid UTF-8
      }
      return 1.0;
   }
   else {
      // Based on ICU's i18n/csrutf8.cpp [with own mods]
      bool hasBOM = (str_cur_n >= 3 &&
            (uint8_t)(str_cur_s[0]) == (uint8_t)0xEF &&
            (uint8_t)(str_cur_s[1]) == (uint8_t)0xBB &&
            (uint8_t)(str_cur_s[2]) == (uint8_t)0xBF);
      R_len_t numValid = 0;   // counts only valid UTF-8 multibyte seqs
      R_len_t numInvalid = 0;

      // Scan for multi-byte sequences
      for (R_len_t i=0; i < str_cur_n; i += 1) {
         uint32_t b = str_cur_s[i];

         if ((b & 0x80) == 0) {
            continue;   // ASCII => OK
         }

         // Hi bit on char found.  Figure out how long the sequence should be
         R_len_t trailBytes = 0;
         if ((b & 0x0E0) == 0x0C0)
            trailBytes = 1;
         else if ((b & 0x0F0) == 0x0E0)
            trailBytes = 2;
         else if ((b & 0x0F8) == 0xF0)
            trailBytes = 3;
         else {
            numInvalid += 1;
            if (numInvalid > 5)
                break; // that's enough => not UTF-8
            continue;
         }

         // Verify that we've got the right number of trail bytes in the sequence
         while (true) {
            i += 1;

            if (i >= str_cur_n)
                break;

            b = str_cur_s[i];

            if ((b & 0xC0) != 0x080) {
                numInvalid += 1;
                break;
            }

            if (--trailBytes == 0) {
                numValid += 1;
                break;
            }
         }
      }

      // Cook up some sort of confidence score, based on presense of a BOM
      //    and the existence of valid and/or invalid multi-byte sequences.
      if (hasBOM && numInvalid == 0)
         return 1.0;
      else if (hasBOM && numValid > numInvalid*10)
         return 0.75;
      else if (numValid > 3 && numInvalid == 0)
         return 1.0;
      else if (numValid > 0 && numInvalid == 0)
         return 0.50; // too few multibyte UTF-8 seqs to be quite sure
      else if (numValid == 0 && numInvalid == 0)
         // Plain ASCII. => It's OK for UTF-8
         return 0.50;
      else if (numValid > numInvalid*10)
         // Probably corrupt utf-8 data.  Valid sequences aren't likely by chance.
         return 0.25;
      else
         return 0.0;
   }
}



/** Check if a string is valid UTF-16LE or UTF-16BE
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 * @param le check for UTF-16LE?
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Marek Gagolewski, 2013-08-09)
 * @version 0.2 (Marek Gagolewski, 2013-08-14) confidence calculation basing on ICU's i18n/csucode.cpp
 */
double stri__enc_check_utf16(const char* str_cur_s, R_len_t str_cur_n,
   bool get_confidence, bool le)
{
   if (str_cur_n % 2 != 0)
      return 0.0;

   bool hasLE_BOM = STRI__ENC_HAS_BOM_UTF16LE(str_cur_s, str_cur_n);
   bool hasBE_BOM = STRI__ENC_HAS_BOM_UTF16BE(str_cur_s, str_cur_n);

   if ((!le && hasLE_BOM) || (le && hasBE_BOM))
      return 0.0;

   R_len_t warnchars = 0;
   
   for (R_len_t i=0; i<str_cur_n; i += 2) {
      uint16_t c = (le)?
                  STRI__GET_INT16_LE(str_cur_s, i):
                  STRI__GET_INT16_BE(str_cur_s, i);
                  
      if (U16_IS_SINGLE(c)) {
         if (c == 0)
            return 0.0;
         else if (c >= 0x0530) // last cyrrilic supplement
            warnchars += 2;
         continue;
      }
         
      if (!U16_IS_SURROGATE_LEAD(c))
         return 0.0;

      i += 2;
      if (i >= str_cur_n)
         return 0.0;
      c = (le)?
          STRI__GET_INT16_LE(str_cur_s, i):
          STRI__GET_INT16_BE(str_cur_s, i);
      if (!U16_IS_SURROGATE_TRAIL(c))
         return 0.0;
   }

   return (get_confidence?(double)(str_cur_n-warnchars)/double(str_cur_n):1.0);
}


/** Check if a string is valid UTF-16BE
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or to exact check
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Marek Gagolewski, 2013-08-09)
 */
double stri__enc_check_utf16be(const char* str_cur_s, R_len_t str_cur_n, bool get_confidence)
{
   return stri__enc_check_utf16(str_cur_s, str_cur_n, get_confidence, false);
}


/** Check if a string is valid UTF-16LE
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Marek Gagolewski, 2013-08-09)
 */
double stri__enc_check_utf16le(const char* str_cur_s, R_len_t str_cur_n, bool get_confidence)
{
   return stri__enc_check_utf16(str_cur_s, str_cur_n, get_confidence, true);
}


/** Check if a string is valid UTF-32LE or UTF-32BE
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 * @param le check for UTF-32LE?
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Marek Gagolewski, 2013-08-09)
 * @version 0.2 (Marek Gagolewski, 2013-08-13) confidence calculation basing on ICU's i18n/csucode.cpp
 */
double stri__enc_check_utf32(const char* str_cur_s, R_len_t str_cur_n,
   bool get_confidence, bool le)
{
   if (str_cur_n % 4 != 0)
      return 0.0;

   bool hasLE_BOM = STRI__ENC_HAS_BOM_UTF32LE(str_cur_s, str_cur_n);
   bool hasBE_BOM = STRI__ENC_HAS_BOM_UTF32BE(str_cur_s, str_cur_n);

   if ((!le && hasLE_BOM) || (le && hasBE_BOM))
      return 0.0;

   R_len_t numValid = 0;
   R_len_t numInvalid = 0;

   for (R_len_t i=0; i<str_cur_n; i+=4) {
      int32_t ch = le?
         (int32_t)STRI__GET_INT32_LE(str_cur_s, i):
         (int32_t)STRI__GET_INT32_BE(str_cur_s, i);

      if (ch < 0 || ch >= 0x10FFFF || (ch >= 0xD800 && ch <= 0xDFFF)) {
         if (!get_confidence)
            return 0.0;
         else
            numInvalid++;
      }
      else
         numValid++;
   }

   if (!get_confidence)
      return 1.0;

   if ((hasLE_BOM || hasBE_BOM) && numInvalid==0)
      return 1.0;
   else if ((hasLE_BOM || hasBE_BOM) && numValid > numInvalid*10)
      return 0.80;
   else if (numValid > 3 && numInvalid == 0)
      return 1.0;
   else if (numValid > 0 && numInvalid == 0)
      return 0.80;
   else if (numValid > numInvalid*10)
      return 0.25; // Probably corruput UTF-32BE data. Valid sequences aren't likely by chance.
   else
      return 0.0;
}


/** Check if a string is valid UTF-32BE
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Marek Gagolewski, 2013-08-13)
 */
double stri__enc_check_utf32be(const char* str_cur_s, R_len_t str_cur_n, bool get_confidence)
{
   return stri__enc_check_utf32(str_cur_s, str_cur_n, get_confidence, false);
}



/** Check if a string is valid UTF-32LE
 *
 * @param str_cur_s character vector
 * @param str_cur_n number of bytes
 * @param get_confidence determine confidence value or do exact check
 *
 * @return confidence value in [0,1]
 *
 * @version 0.1 (Marek Gagolewski, 2013-08-13)
 */
double stri__enc_check_utf32le(const char* str_cur_s, R_len_t str_cur_n, bool get_confidence)
{
   return stri__enc_check_utf32(str_cur_s, str_cur_n, get_confidence, true);
}




/** Which string is in given encoding
 *
 *
 *  @param str character vector or raw vector or list of raw vectors
 *  @param type (single integer, internal)
 *  @return logical vector
 *
 * @version 0.1 (Bartek Tartanus)
 * @version 0.2 (Marek Gagolewski, 2013-08-08) use StriContainerListRaw
 * @version 0.3 (Marek Gagolewski, 2013-08-09) one function for is_*, do dispatch
 */
SEXP stri_enc_isenc(SEXP str, SEXP type)
{
   if (!Rf_isInteger(type) || LENGTH(type) != 1)
      Rf_error(MSG__INCORRECT_INTERNAL_ARG); // this is an internal arg, check manually, error() allowed here
   int _type = INTEGER(type)[0];
   double (*isenc)(const char*, R_len_t, bool) = NULL;
   switch (_type) {
      case 1:  isenc = stri__enc_check_ascii;   break;
      case 2:  isenc = stri__enc_check_utf8;    break;
      case 3:  isenc = stri__enc_check_utf16be; break;
      case 4:  isenc = stri__enc_check_utf16le; break;
      case 5:  isenc = stri__enc_check_utf32be; break;
      case 6:  isenc = stri__enc_check_utf32le; break;
      default: Rf_error(MSG__INCORRECT_INTERNAL_ARG); // error() call allowed here
   }


   str = stri_prepare_arg_list_raw(str, "str");

   STRI__ERROR_HANDLER_BEGIN
   StriContainerListRaw str_cont(str);
   R_len_t str_length = str_cont.get_n();

   SEXP ret;
   PROTECT(ret = Rf_allocVector(LGLSXP, str_length));
   int* ret_tab = LOGICAL(ret); // may be faster than LOGICAL(ret)[i] all the time

   for (R_len_t i=0; i < str_length; ++i) {
      if (str_cont.isNA(i)) {
         ret_tab[i] = NA_LOGICAL;
         continue;
      }

      bool get_confidence = false; // TO BE DONE
      ret_tab[i] = isenc(str_cont.get(i).c_str(), str_cont.get(i).length(), get_confidence) != 0.0;
   }

   UNPROTECT(1);
   return ret;

   STRI__ERROR_HANDLER_END({ /* no-op on error */ })
}


/** Detect encoding and language
 *
 * @param str character vector
 * @param filter_angle_brackets logical vector
 *
 * @return list
 *
 * @version 0.1 (Marek Gagolewski, 2013-08-03)
 * @version 0.2 (Marek Gagolewski, 2013-08-08) use StriContainerListRaw + BUGFIX
 */
SEXP stri_enc_detect(SEXP str, SEXP filter_angle_brackets)
{
   str = stri_prepare_arg_list_raw(str, "str");
   filter_angle_brackets = stri_prepare_arg_logical(filter_angle_brackets, "filter_angle_brackets");

   UCharsetDetector* ucsdet = NULL;


   STRI__ERROR_HANDLER_BEGIN

   UErrorCode status = U_ZERO_ERROR;
   ucsdet = ucsdet_open(&status);
   if (U_FAILURE(status)) throw StriException(status);

   StriContainerListRaw str_cont(str);
   R_len_t str_n = str_cont.get_n();

   R_len_t vectorize_length = stri__recycling_rule(true, 2, str_n, LENGTH(filter_angle_brackets));
   str_cont.set_nrecycle(vectorize_length); // must be set after container creation

   SEXP ret, names, wrong;
   PROTECT(ret = Rf_allocVector(VECSXP, vectorize_length));

   PROTECT(names = Rf_allocVector(STRSXP, 3));
   SET_STRING_ELT(names, 0, Rf_mkChar("Encoding"));
   SET_STRING_ELT(names, 1, Rf_mkChar("Language"));
   SET_STRING_ELT(names, 2, Rf_mkChar("Confidence"));

   PROTECT(wrong = Rf_allocVector(VECSXP, 3));
   SET_VECTOR_ELT(wrong, 0, stri__vector_NA_strings(1));
   SET_VECTOR_ELT(wrong, 1, stri__vector_NA_strings(1));
   SET_VECTOR_ELT(wrong, 2, stri__vector_NA_integers(1));
   Rf_setAttrib(wrong, R_NamesSymbol, names);

   StriContainerLogical filter(filter_angle_brackets, vectorize_length);
   for (R_len_t i=0; i<vectorize_length; ++i) {
      if (str_cont.isNA(i) || filter.isNA(i)) {
         SET_VECTOR_ELT(ret, i, wrong);
         continue;
      }

      const char* str_cur_s = str_cont.get(i).c_str();
      R_len_t str_cur_n     = str_cont.get(i).length();

      status = U_ZERO_ERROR;
      ucsdet_setText(ucsdet, str_cur_s, str_cur_n, &status);
   	if (U_FAILURE(status)) throw StriException(status);
      ucsdet_enableInputFilter(ucsdet, filter.get(i));

      status = U_ZERO_ERROR;
      int matchesFound;
      const UCharsetMatch** match = ucsdet_detectAll(ucsdet, &matchesFound, &status);
   	if (U_FAILURE(status) || !match || matchesFound <= 0) {
         SET_VECTOR_ELT(ret, i, wrong);
         continue;
   	}


      SEXP val_enc, val_lang, val_conf;
      PROTECT(val_enc  = Rf_allocVector(STRSXP, matchesFound));
      PROTECT(val_lang = Rf_allocVector(STRSXP, matchesFound));
      PROTECT(val_conf = Rf_allocVector(REALSXP, matchesFound));

      for (R_len_t j=0; j<matchesFound; ++j) {
         status = U_ZERO_ERROR;
         const char* name = ucsdet_getName(match[j], &status);
         if (U_FAILURE(status) || !name)
            SET_STRING_ELT(val_enc, j, NA_STRING);
         else
            SET_STRING_ELT(val_enc, j, Rf_mkChar(name));

         status = U_ZERO_ERROR;
    	   int32_t conf = ucsdet_getConfidence(match[j], &status);
         if (U_FAILURE(status))
            REAL(val_conf)[j] = NA_REAL;
         else
            REAL(val_conf)[j] = (double)(conf)/100.0;

         status = U_ZERO_ERROR;
    	   const char* lang = ucsdet_getLanguage(match[j], &status);
         if (U_FAILURE(status) || !lang)
            SET_STRING_ELT(val_lang, j, NA_STRING);
         else
            SET_STRING_ELT(val_lang, j, Rf_mkChar(lang));
      }

      SEXP val;
      PROTECT(val = Rf_allocVector(VECSXP, 3));
      SET_VECTOR_ELT(val, 0, val_enc);
      SET_VECTOR_ELT(val, 1, val_lang);
      SET_VECTOR_ELT(val, 2, val_conf);
      Rf_setAttrib(val, R_NamesSymbol, names);
      SET_VECTOR_ELT(ret, i, val);
      UNPROTECT(4);
   }

   if (ucsdet) {
      ucsdet_close(ucsdet);
      ucsdet = NULL;
   }
   UNPROTECT(3);
   return ret;

   STRI__ERROR_HANDLER_END(
      if (ucsdet) {
         ucsdet_close(ucsdet);
         ucsdet = NULL;
      })
}





/** help struct for stri_enc_detect2 */
struct Converter8bit {
   bool isNA;
   bool countChars[256-128];
   bool badChars[256-128];
   const char* name;
   UConverter* ucnv;
   
   Converter8bit(SEXP enc_cur, SEXP chr_cur) {    
      isNA = true;
      name = NULL;
      ucnv = NULL;
      
      if (enc_cur == NA_STRING) 
         return;

      UErrorCode err = U_ZERO_ERROR;
      ucnv = ucnv_open(CHAR(enc_cur), &err);
      if (U_FAILURE(err)) {
         throw StriException(MSG__ENC_INCORRECT_ID_WHAT, CHAR(enc_cur));
      }
      
      if (ucnv_getMaxCharSize(ucnv) != 1 || ucnv_getMinCharSize(ucnv) != 1) {
         ucnv_close(ucnv);
         throw StriException(MSG__ENC_NOT8BIT, CHAR(enc_cur));
      }
      
      isNA = false;
      name = CHAR(enc_cur);

      // Check which characters in given encoding
      // are not mapped to Unicode [badChars]
      char allChars[256-128+1]; // all bytes 128-256
      for (R_len_t i=128; i<256; ++i)
         allChars[i-128] = (char)i;
      allChars[256-128] = '\0';
      
      const char* text_start = allChars;
      const char* text_end   = allChars+(256-128);
      ucnv_reset(ucnv);
      for (R_len_t i=128; i<256; ++i) {
         UErrorCode err = U_ZERO_ERROR;
         UChar32 c = ucnv_getNextUChar(ucnv, &text_start, text_end, &err);
         if (U_FAILURE(err)) {
            ucnv_close(ucnv);
            throw StriException("Cannot convert character 0x%2x (encoding=%s)",
               (int)(unsigned char)*(text_start-1), CHAR(enc_cur));
         }
         else {
            if (c == UCHAR_REPLACEMENT || !u_isdefined(c) || u_isalpha(c))
               badChars[i-128] = true; // letters are bad if they are not counted
            else
               badChars[i-128] = false;
         }
      }
      
      for (R_len_t i=128; i<256; ++i)
            countChars[i-128] = false; // reset
            
      if (chr_cur != NA_STRING && LENGTH(chr_cur) > 0) {
         // determine which characters the user wants to count [countChars]
      
         UnicodeString encs = UnicodeString::fromUTF8(std::string(CHAR(chr_cur)));
         R_len_t encs_count = encs.countChar32();
         R_len_t bufneed = UCNV_GET_MAX_BYTES_FOR_STRING(encs.length(), 1);
         // "The calculated size is guaranteed to be sufficient for this conversion."
         String8 buf(bufneed);
         err = U_ZERO_ERROR;
         encs.extract(buf.data(), buf.size(), ucnv, err); // UTF-16 -> TO
         if (U_FAILURE(err)) throw StriException(err);
         R_len_t buf_count = 0;
         for (R_len_t i=0; i<buf.size() && buf.data()[i]; ++i) {
            if ((uint8_t)(buf.data()[i]) >= (uint8_t)128) {
               countChars[(uint8_t)(buf.data()[i])-(uint8_t)128] = true;
               buf_count++;
            }
         }
         if (buf_count != encs_count)
            throw StriException(MSG__INTERNAL_ERROR); // we use internal chr_cur, everything should be OK
      }
      
      ucnv_close(ucnv);
      ucnv = NULL;
   }
   
   ~Converter8bit() {
      if (ucnv) {
         ucnv_close(ucnv);
         ucnv = NULL;
      }
   }
};



/** help struct for stri_enc_detect2 */
struct EncGuess {
   const char* name;
   double confidence;

   EncGuess(const char* _name, double _confidence) {
      name = _name;
      confidence = _confidence;
   }

   bool operator<(const EncGuess& e2) const {
      return (this->confidence > e2.confidence); // decreasing sort
   }
   
   static void do_utf32(vector<EncGuess>& guesses, const char* str_cur_s, R_len_t str_cur_n)
   {
      /* check UTF-32LE, UTF-32BE or UTF-32+BOM */
      double isutf32le = stri__enc_check_utf32le(str_cur_s, str_cur_n, true);
      double isutf32be = stri__enc_check_utf32be(str_cur_s, str_cur_n, true);
      if (isutf32le >= 0.25 && isutf32be >= 0.25) {
         // no BOM, both valid
         // i think this will never happen
         guesses.push_back(EncGuess("UTF-32LE", isutf32le));
         guesses.push_back(EncGuess("UTF-32BE", isutf32be));
      }
      else if (isutf32le >= 0.25) {
         if (STRI__ENC_HAS_BOM_UTF32LE(str_cur_s, str_cur_n))
            guesses.push_back(EncGuess("UTF-32", isutf32le)); // with BOM
         else
            guesses.push_back(EncGuess("UTF-32LE", isutf32le));
      }
      else if (isutf32be >= 0.25) {
         if (STRI__ENC_HAS_BOM_UTF32BE(str_cur_s, str_cur_n))
            guesses.push_back(EncGuess("UTF-32", isutf32be)); // with BOM
         else
            guesses.push_back(EncGuess("UTF-32BE", isutf32be));
      }
   }
   
   static void do_utf16(vector<EncGuess>& guesses, const char* str_cur_s, R_len_t str_cur_n)
   {
      /* check UTF-16LE, UTF-16BE or UTF-16+BOM */
      double isutf16le = stri__enc_check_utf16le(str_cur_s, str_cur_n, true);
      double isutf16be = stri__enc_check_utf16be(str_cur_s, str_cur_n, true);
      if (isutf16le >= 0.25 && isutf16be >= 0.25) {
         // no BOM, both valid
         // this may sometimes happen
         guesses.push_back(EncGuess("UTF-16LE", isutf16le));
         guesses.push_back(EncGuess("UTF-16BE", isutf16be));
      }
      else if (isutf16le >= 0.25) {
         if (STRI__ENC_HAS_BOM_UTF16LE(str_cur_s, str_cur_n))
            guesses.push_back(EncGuess("UTF-16", isutf16le)); // with BOM
         else
            guesses.push_back(EncGuess("UTF-16LE", isutf16le));
      }
      else if (isutf16be >= 0.25) {
         if (STRI__ENC_HAS_BOM_UTF16BE(str_cur_s, str_cur_n))
            guesses.push_back(EncGuess("UTF-16", isutf16be)); // with BOM
         else
            guesses.push_back(EncGuess("UTF-16BE", isutf16be));
      }
   }
   
   static void do_8bit(vector<EncGuess>& guesses, const char* str_cur_s, R_len_t str_cur_n,
      vector<Converter8bit>& converters, R_len_t converters_num)
   {
      double is8bit = stri__enc_check_8bit(str_cur_s, str_cur_n, false);
      if (is8bit != 0.0) {
         // may be an 8-bit encoding
         double isascii = stri__enc_check_ascii(str_cur_s, str_cur_n, true);
         if (isascii >= 0.25) // i.e. equal to 1.0 => nothing more to check
            guesses.push_back(EncGuess("ASCII", isascii));
         else {
            // not ascii
            double isutf8 = stri__enc_check_utf8(str_cur_s, str_cur_n, true);
            if (isutf8 >= 0.25)
               guesses.push_back(EncGuess("UTF-8", isutf8));
            if (converters_num > 0 && isutf8 < 1.0) {
               do_8bit_locale(guesses, str_cur_s, str_cur_n, converters);
            }
         }
      }
   }
   
   static void do_8bit_locale(vector<EncGuess>& guesses, const char* str_cur_s, R_len_t str_cur_n,
      vector<Converter8bit>& converters)
   {
      // count all bytes with codes >= 128 in str_cur_s
      R_len_t counts[256-128];
      R_len_t countsge128 = 0; // total count
      for (R_len_t k=0; k<256-128; ++k)
         counts[k] = 0; // reset tab
      for (R_len_t j=0; j<str_cur_n; ++j) {
         if ((uint8_t)(str_cur_s[j]) >= (uint8_t)128) {
            counts[(uint8_t)(str_cur_s[j])-(uint8_t)(128)]++;
            countsge128++;
         }
      }
            
      std::vector<int> badCounts(converters.size(), 0); // filled with 0
      std::vector<int> desiredCounts(converters.size(),0);
      R_len_t maxDesiredCounts = 0;
      
      
      for (R_len_t j=0; j<(R_len_t)converters.size(); ++j) { // for each converter
         if (converters[j].isNA) continue;
         for (R_len_t k=0; k<256-128; ++k) { // for each character
            // 1. Count bytes that are BAD and NOT COUNTED in this encoding
            if (converters[j].badChars[k] && !converters[j].countChars[k]) {
               badCounts[j] += (int)counts[k];
            }
            // 2. Count indicated characters
            if (converters[j].countChars[k]) {
               desiredCounts[j] += (int)counts[k];
            }
         }
         if (desiredCounts[j] > maxDesiredCounts)
            maxDesiredCounts = desiredCounts[j];
      }
      
      // add guesses
      for (R_len_t j=0; j<(R_len_t)converters.size(); ++j) { // for each converter
         if (converters[j].isNA) continue;
         guesses.push_back(EncGuess(converters[j].name, 
            // some heuristic:
            min(1.0, max(0.0, 
               (double)(countsge128-0.5*badCounts[j]-maxDesiredCounts+desiredCounts[j])/
               (double)(countsge128)))
         ));
      }
   }
};




/** Detect encoding with initial guess
 *
 * @param str character vector
 * @param encodings character vector
 * @param characters character vector
 *
 * @return list
 *
 * @version 0.1 (2013-08-15, Marek Gagolewski)
 * @version 0.2 (2013-08-18, Marek Gagolewski) improved 8-bit confidence measurement, 
 * some code moved to structs
 */
SEXP stri_enc_detect2(SEXP str, SEXP encodings, SEXP characters)
{
   str = stri_prepare_arg_list_raw(str, "str");
   
   if (encodings != R_NilValue) {
      encodings = stri_enc_toutf8(
               stri_prepare_arg_string(encodings, "encodings"),
               Rf_ScalarLogical(FALSE)
            ); /* now definitely in UTF-8 */
   }
   
   if (characters != R_NilValue) {
      characters = stri_enc_toutf8(
            stri_flatten(
               stri_prepare_arg_string(characters, "characters"), Rf_mkString("")
            ),
            Rf_ScalarLogical(FALSE)
         ); /* now definitely in UTF-8 */
   }

   STRI__ERROR_HANDLER_BEGIN

   vector<Converter8bit> converters;
   R_len_t converters_num = 0;
   if (encodings != R_NilValue && LENGTH(encodings) > 0) {
      converters.reserve(LENGTH(encodings));
      for (R_len_t i=0; i<LENGTH(encodings); ++i) {
         SEXP enc_cur = STRING_ELT(encodings, i);
         SEXP chr_cur = STRING_ELT(characters, 0);
         converters.push_back(Converter8bit(enc_cur, chr_cur));
         if (!converters[i].isNA)
            ++converters_num;
      }
   }

   StriContainerListRaw str_cont(str);
   R_len_t str_n = str_cont.get_n();

   SEXP ret, names, wrong;
   PROTECT(ret = Rf_allocVector(VECSXP, str_n));

   PROTECT(names = Rf_allocVector(STRSXP, 3));
   SET_STRING_ELT(names, 0, Rf_mkChar("Encoding"));
   SET_STRING_ELT(names, 1, Rf_mkChar("Language"));
   SET_STRING_ELT(names, 2, Rf_mkChar("Confidence"));

   PROTECT(wrong = Rf_allocVector(VECSXP, 3));
   SET_VECTOR_ELT(wrong, 0, stri__vector_NA_strings(1));
   SET_VECTOR_ELT(wrong, 1, stri__vector_NA_strings(1));
   SET_VECTOR_ELT(wrong, 2, stri__vector_NA_integers(1));
   Rf_setAttrib(wrong, R_NamesSymbol, names);

   for (R_len_t i=0; i<str_n; ++i) {
      if (str_cont.isNA(i)) {
         SET_VECTOR_ELT(ret, i, wrong);
         continue;
      }

      const char* str_cur_s = str_cont.get(i).c_str();
      R_len_t str_cur_n     = str_cont.get(i).length();
      if (str_cur_n <= 0) {
         SET_VECTOR_ELT(ret, i, wrong);
         continue;
      }

      vector<EncGuess> guesses;
      guesses.reserve(6);

      EncGuess::do_utf32(guesses, str_cur_s, str_cur_n);
      EncGuess::do_utf16(guesses, str_cur_s, str_cur_n);
      EncGuess::do_8bit(guesses, str_cur_s, str_cur_n, converters, converters_num);  // includes UTF-8

      R_len_t matchesFound = guesses.size();
      if (matchesFound <= 0) {
         SET_VECTOR_ELT(ret, i, wrong);
         continue;
      }

      std::stable_sort(guesses.begin(), guesses.end());

      SEXP val_enc, val_lang, val_conf;
      PROTECT(val_enc  = Rf_allocVector(STRSXP, matchesFound));
      PROTECT(val_lang = Rf_allocVector(STRSXP, matchesFound));
      PROTECT(val_conf = Rf_allocVector(REALSXP, matchesFound));

      for (R_len_t j=0; j<matchesFound; ++j) {
         SET_STRING_ELT(val_enc, j, Rf_mkChar(guesses[j].name));
         REAL(val_conf)[j] = guesses[j].confidence;
    	   SET_STRING_ELT(val_lang, j, NA_STRING); // always no lang
      }

      SEXP val;
      PROTECT(val = Rf_allocVector(VECSXP, 3));
      SET_VECTOR_ELT(val, 0, val_enc);
      SET_VECTOR_ELT(val, 1, val_lang);
      SET_VECTOR_ELT(val, 2, val_conf);
      Rf_setAttrib(val, R_NamesSymbol, names);
      SET_VECTOR_ELT(ret, i, val);
      UNPROTECT(4);
   }

   UNPROTECT(3);
   return ret;

   STRI__ERROR_HANDLER_END({ /* no-op on error */ })
}

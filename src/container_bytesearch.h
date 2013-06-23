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
 
#ifndef __container_bytesearch_h
#define __container_bytesearch_h





/**
 * A class to handle StriByteSearch patterns
 * @version 0.1 (Marek Gagolewski, 2013-06-23)
 */
class StriContainerByteSearch : public StriContainerUTF8 {
   
   private:
      
      
      R_len_t patternLen; 
      const char* patternStr; 
      R_len_t searchPos; // -1 after reset, searchLen on no further matches
      const char* searchStr; // owned by caller
      R_len_t searchLen; // in bytes
      
#ifndef NDEBUG
      R_len_t debugMatcherIndex;  ///< used by vectorize_getMatcher (internally - check)
#endif


   public:
       
      StriContainerByteSearch();
      StriContainerByteSearch(SEXP rstr, R_len_t nrecycle);
      StriContainerByteSearch(StriContainerByteSearch& container);
      ~StriContainerByteSearch();
      StriContainerByteSearch& operator=(StriContainerByteSearch& container);      
      
      void setupMatcher(R_len_t i, const char* searchStr, R_len_t searchLen);
      void resetMatcher();
      R_len_t findFirst();
      R_len_t findNext();
      R_len_t findLast();
      R_len_t getMatchedStart();
      R_len_t getMatcherLength();
};

#endif

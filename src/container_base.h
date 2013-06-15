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
 
#ifndef __container_base_h
#define __container_base_h





/**
 * Base class for StriContainers
 * @version 0.1 (Marek Gagolewski)
 * @version 0.2 (Marek Gagolewski) - removed ucnvNative, ucnvLatin1 (not needed per-object)
 * @version 0.3 (Marek Gagolewski) - removed enc array
 */
class StriContainerUTF_Base {
   
   protected:
      
      R_len_t n;                 ///< number of strings (size of \code{str})
      R_len_t nrecycle;          ///< number of strings for the recycle rule (can be > \code{n})

#ifndef NDEBUG
      bool isShallow;            ///< have we made only shallow copy of the strings? (=> read only)
#endif

      StriContainerUTF_Base();
      StriContainerUTF_Base(StriContainerUTF_Base& container);
      ~StriContainerUTF_Base();

      void init_Base(R_len_t n, R_len_t nrecycle, bool shallowrecycle);


   public:
      StriContainerUTF_Base& operator=(StriContainerUTF_Base& container);

      

      
      
      /** Loop over vectorized container - init */
      inline R_len_t vectorize_init() const {
         if (n <= 0) return nrecycle;
         else return 0;
      }
      
      /** Loop over vectorized container - end iterator */
      inline R_len_t vectorize_end() const {
         return nrecycle;  
      }
      
      /** Loop over vectorized container - next iteration */
      inline R_len_t vectorize_next(R_len_t i) const {
         if (i == nrecycle - 1 - (nrecycle%n))
            return nrecycle; // this is the end
         i = i + n;
         if (i >= nrecycle)
            return (i % n) + 1;  
         else
            return i;
      }
};

#endif

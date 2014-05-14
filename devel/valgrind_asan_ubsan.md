```
R -d "valgrind --tool=memcheck --leak-check=full"
```


```
wget ftp://ftp.stat.math.ethz.ch/Software/R/R-devel.tar.bz2
tar -jxvf R-devel.tar.bz2
# ...
# -fsanitize=address
# MAIN_LDFLAGS="-fsanitize=address" \
CC="clang -std=gnu99" \  
CXX="clang++ -std=gnu++98" \
CFLAGS="-fno-omit-frame-pointer -O2 -Wall -pedantic -mtune=native" \
MAIN_CFLAGS="-fno-omit-frame-pointer -O2 -Wall -pedantic -mtune=native" \
./configure  --enable-memory-profiling \
--enable-R-profiling --prefix='/opt/R-devel-clang/' 
make
make install
# ...
/opt/R-devel-clang/bin/R CMD INSTALL  /home/gagolews/Projekty/stringi_0.2-3.tar.gz 

#/opt/R-devel-clang/bin/R CMD INSTALL --configure-args="--disable-pkg-config --with-extra-cppflags=\"-fsanitize=undefined-trap,unsigned-integer-overflow  -Wall -pedantic -fsanitize-undefined-trap-on-error\" --with-extra-ldflags=\"-fsanitize=undefined-trap\"" /home/gagolews/Projekty/stringi_0.2-3.tar.gz


# ...
R CMD INSTALL --configure-args="--with-extra-cppflags=\"-fsanitize=address  -Wall -pedantic -fno-omit-frame-pointer\" --with-extra-ldflags=\"-fsanitize=address -fno-omit-frame-pointer -lasan\"" /home/gagolews/Projekty/stringi_0.2-3.tar.gz


ASAN_OPTIONS="detect_leaks=1" R
setwd('/home/gagolews/Projekty/stringi')
source('devel/testthat/run_package_tests.R')
```

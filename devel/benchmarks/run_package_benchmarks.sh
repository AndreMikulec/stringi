#!/bin/bash

if test $# -eq 0
then
   RHOME=`R RHOME`
else
   RHOME="$1"
fi

$RHOME/bin/R CMD INSTALL --preclean --no-data --no-demo --no-help . > /dev/null \
&& \
LC_ALL="pl_PL.UTF-8"      $RHOME/bin/Rscript \
   --vanilla devel/testthat/run_package_tests.R \
&& \
LC_ALL="pl_PL.iso-8859-2"      $RHOME/bin/Rscript \
   --vanilla devel/testthat/run_package_tests.R \
&& \
LC_ALL="pl_PL.UTF-8"      $RHOME/bin/Rscript \
   --vanilla devel/benchmarks/run_package_benchmarks.R \
&& \
LC_ALL="pl_PL.iso-8859-2" $RHOME/bin/Rscript \
   --vanilla devel/benchmarks/run_package_benchmarks.R


# ... Rscript -e "knitr::knit2pdf('$1')" ....
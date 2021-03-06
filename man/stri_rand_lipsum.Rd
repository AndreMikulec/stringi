% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/random.R
\name{stri_rand_lipsum}
\alias{stri_rand_lipsum}
\title{A Lorem Ipsum Generator}
\usage{
stri_rand_lipsum(nparagraphs, start_lipsum = TRUE)
}
\arguments{
\item{nparagraphs}{single integer, number of paragraphs to generate}

\item{start_lipsum}{single logical value; should the resulting
text start with \emph{Lorem ipsum dolor sit amet}?}
}
\value{
Returns a character vector of length \code{nparagraphs}.
}
\description{
Generates (pseudo)random \emph{lorem ipsum} text consisting
of a given number of text paragraphs.
}
\details{
\emph{Lorem ipsum} is a dummy text often used as a source
of data for string processing and displaying/layouting exercises.

Current implementation is very simple:
words are selected randomly from a Zipf distribution
(we base on a set of ca. 190 predefined Latin words).
Number of words per sentence and sentences per paragraph
follows a discretized, truncated normal distribution.
No Markov chain modeling, just i.i.d. word selection.
}
\examples{
cat(sapply(
   stri_wrap(stri_rand_lipsum(10), 80, simplify=FALSE),
   stri_flatten, collapse="\\n"), sep="\\n\\n")
cat(stri_rand_lipsum(10), sep="\\n\\n")

}
\seealso{
Other random: \code{\link{stri_rand_shuffle}},
  \code{\link{stri_rand_strings}}
}

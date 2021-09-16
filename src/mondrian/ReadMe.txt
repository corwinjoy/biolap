========================================================================
      Mondrian Changes
========================================================================


As much as possible, we are using the standard mondrian distribution.
Unfortunately, the way that mondrian is written right now, you cannot 
add a new aggregation function without recompiling the project.
Fortunately, to add a new fuction you need only change two files:
Mondrian.xml and RolapAggregator.java.

These two files let mondrian know about the existence of a new user
defined aggregation function.

So, rather than reproduce the entire mondrian source tree we show only
these two files which are the only things we have changed from the
standard distribution.  To find our changes, look for the word 'seqsim'
to see our new aggregation function.

We have also included a binary, mondrian.jar, which contains mondrian
recompiled from perforce as of Oct-5-2005 with our changes.

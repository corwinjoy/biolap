========================================================================
       DYNAMIC LINK LIBRARY : ORA_SUFFIX
========================================================================


This library defines aggregation functions called seqsim and seqsimhash to 
compute sequence similarities. To set these functions up in Oracle: 

1. Copy the file Release/ORA_SUFFIX.dll to your Oracle server machine as 'C:\ORA_SUFFIX.dll'.

2. Follow the instructions in suffix_PL.sql to configure your listener.ora file
to allow external procedures.

3. Run the script suffix_PL.sql to define the new aggregation functions.

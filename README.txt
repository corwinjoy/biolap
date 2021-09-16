========================================================================
      Introduction
========================================================================

The idea behind this project is to extend high dimensional OLAP cubes
by adding biologically relevant aggregation functions. In the example
code we add an aggregation function that computes sequence similarity.
We are then able to browse large genomic data sets to find similarities
between key proteins and visually understand evolution paths.
Please see the whitepaper for details.

========================================================================
      Contents
========================================================================

Each subfolder contains a ReadMe.txt folder explaining the contents of that folder.
Here is an overview:

docs/ 
	Information about the project

database/ 
	Sample schemas and genomic data

demos/ 
	Two .war files which can be deployed to tomcat to demonstrate genomic OLAP cubes

src/mondrian 
	Changes I made to the mondrian.sourceforge.net project to add new functions.

src/seqsim 
	Source & binary code for a new similarity function to summarize protein sequences.



========================================================================
      System Requirements
========================================================================


1. Oracle version 9i or higher running on a windows machine 
(for now, until I generalize the interface for sequence similarities to other databases).
You can get a free copy of Oracle here: 
http://www.oracle.com/technology/software/products/database/oracle10g/index.html

For testing I just installed 10g an old 1GHZ machine with 260MB of memory which runs fine.


2. Apache Tomcat version 4.1 or higher: http://tomcat.apache.org/

3. JDK 1.4.2 or higher. http://java.sun.com/j2se/1.5.0/download.jsp


========================================================================
      Installation
========================================================================


1. Create the Oracle sample schema as per \database\seq database\ReadMe.txt

2. Copy the file demos\seq_sim.war to your tomcat\webapps folder.  
Start tomcat to automatically unzip the war file to the directory tomcat\webapps\seq_sim. 

3. Look in \tomcat\webapps\seq_sim\WEB-INF\queries for a file called seq_query.jsp to see the MDX query we are running.

   You will need to change the field in seq_query.jsp that says

   jdbcUrl="jdbc:oracle:thin:seq/seq@localhost:1521:orcl" 

   to point to the locataion of where you set up the seq database. The format is

   jdbcUrl="jdbc:oracle:thin:username/password@machine_name:port:sid" 


5. Look in \tomcat\webapps\seq_sim\WEB-INF\queries for a file called warehouse.xml to see the cube design we are using.

We have not yet defined the sequence similarity function.
So, change

<Measure name="SequenceSimilarity" column="SEQ" aggregator="seqsim" /> 

to

<Measure name="SequenceSimilarity" column="SEQ" aggregator="count" /> 


You can now run the demo cube by pointing a web browser to the demo running under tomcat at

http://localhost:8080/seq_sim

Select the JPivot link.


4. Create the real sequence similarity function, called seqsim, in Oracle. 
Follow the instructions in src\seqsim\ReadMe.txt.
Change warehouse.xml back to use seqsim as the aggregator.
Restart tomcat and refresh the demo webpage.







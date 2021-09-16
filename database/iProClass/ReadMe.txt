Here are instructions for setting up the iProClass database schema along with protein sequence data and GO classifications.

1. Get an Oracle dump of the iProClass database from ftp://ftp.pir.georgetown.edu/pir_databases/iproclass/dmpfiles/
The file you want is csqreport2_30.dmp.gz.  Import the dump to get the table CSQREPORT2_30.

2. Import the SwissProt / Uniprot protein sequence data.  This is fairly easy, see the instructions in swissprot.htm in this folder.

3. Import the GO sequence classification data.  All you need from the GO database are the "core tables".  
We have oracle table definitions for these tables in go_schema.sql. Importing the data from GO is a little bit tricker.
I simply grabbed a MySQL database dump from here: http://www.godatabase.org/dev/database/ , then I just copied the data
into Oracle tables using ODBC. If you look at the program import_go_data.cpp this is a small 50 line C++ program to bulk
copy data from MySQL to Oracle - I used this mainly for the graph_path table since it is a bit big at 1.3 million records.

4. Now that you have the tables from these three databases, it is time to join them together into a unified table for reporting.
Run the script called cube_tables.sql.  This joins the protein annotations to the protein sequence data and the go hierarchies.

That's it!  You are now ready to run cubes against this data.  To see how the cube uses this tables in cube_tables.sql
look at the file warehouse.xml.

To see the demo, deploy file demos/iProClass.war to tomcat by copying it to your tomcat\webapps directory. 
When tomcat deploys this file it will unzip it to a directory called iProclass.
 
Look in \tomcat\webapps\iProClass\WEB-INF\queries for a file called seq_query.jsp to see the MDX query we are running.
You will need to change the field that says

jdbcUrl="jdbc:oracle:thin:iproclass/iproclass@localhost:1521:orcl" 

to point to the locataion of where you set up the iproclass database. The format is
jdbcUrl="jdbc:oracle:thin:username/password@machine_name:port:sid" 
 
Look in \tomcat\webapps\iProClass\WEB-INF\queries for a file called warehouse.xml to see the cube design.




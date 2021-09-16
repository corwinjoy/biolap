// This is a simple program to copy a large GO table from a MySQL database to
// an ORACLE database.  It uses the Database Template Library which can be found at
// dtemplatelib.sourceforge.net

#include "dtl.h"
#include "table.h"
#include <iostream>

using namespace dtl;
using namespace std;

DTL_STRUCT4(graph_path,
            long int, id,
            long int, term1_id,
            long int, term2_id,
            long int, distance
            );

int main(void) {

    try {
    DBConnection oracle("uid=iproclass;pwd=iproclass;dsn=cjoy10g");
    DBConnection mysql("dsn=localMySQL");

    oracle.Connect();
    mysql.Connect();
    
    typedef DBView<graph_path> DBV;
    DBV src(DBV::Args().tables("graph_path").conn(mysql));
    DBV dest(DBV::Args().tables("graph_path").conn(oracle));

    DBV::insert_iterator ins_it(dest); 

    bulk_insert_helper(src.begin(), src.end(), 256, ins_it);

    oracle.CommitAll();

    }
    catch (std::exception &err) {
        cerr << err.what() << endl;
        return 1;
    }


    return 0;

}


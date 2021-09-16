#include "dtl.h"

using namespace dtl;

DTL_STRUCT4(graph_path,
            long, id,
            long, term1_id,
            long, term2_id,
            lond, distance
            );

int main(void) {

    try {
    DBConnection oracle("uid=iproclass;pwd=iproclass;dsn=cjoy10g");
    DBConnection mysql("dsn=localMySQL");
    
    typedef DBView<graph_path> DBV;
    DBV src(DBV::Args().tables("graph_path").conn(mysql));
    DBV dest(DBV::Args().tables("graph_path").conn(oracle));

    DBV::insert_iterator ins_it(dest); 

    bulk_insert_helper(src.begin(), src.end(), 256, ins_it);

    }
    catch (std::exception &err) {
        cerr << err << endl;
    }


}
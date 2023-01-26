#include <iostream>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorString.h"



int main(int argc, char **argv){

    for(auto i=0; i<100; ++i){
        FUNCINFO("Some random strings: '" << Generate_Random_String_of_Length(100) << "'");
    }

    //Test of Get_Parent_Directory().
    std::string in, need, got;

    in = "/some/file";   need = "/some/";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "/some/path/";   need = "/some/";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "some/path/";   need = "some/";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "some/file";   need = "some/";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "/some/";   need = "/";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "/";   need = "";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "file";   need = "";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "../file";   need = "../";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "./file";   need = "./";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "../../../";   need = "../../";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }

    in = "./.././";   need = "./../";
    got = Get_Parent_Directory(in);
    if(got != need){   FUNCERR("Got '" << got << "' and needed '" << need << "'");
    }else{             FUNCINFO("Success! '" << in << "' --> '" << got << "'"); }


    return 0;
}

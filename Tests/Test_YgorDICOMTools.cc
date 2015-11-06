#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>      //Needed for intmax_t

#include "YgorMisc.h"
#include "YgorFilesDirs.h"

#include "YgorDICOMTools.h"

int main(int argc, char **argv){

    //Choose a default filename.
    std::string filename_in = "./RS1.dcm";
    //filename_in = "OTHER_DATA/CT3.dcm";
    //filename_in = "RS1.dcm_repacked.dcm";
    //filename_in = "/home/hal/SGF/SGF2/RS.1.2.246.352.71.4.371336894.14697590.20080428144535.dcm";
    //filename_in = "/home/hal/SGF/SGF2/RS.1.2.246.352.71.4.371336894.14697590.20080428144535.dcm_repacked.dcm";

    //See if the user specifies another file.
    if(argc == 2) filename_in = argv[1];

    //These are files which are written to at intermediate steps, for human verification.
    std::string filename_parsed       = filename_in + "_parsed.txt";
    std::string filename_fully_parsed = filename_in + "_fully_parsed.txt";
    std::string filename_repacked     = filename_in + "_repacked.dcm";

    //... but we black-hole the output because it can be a lot for a test program.
    filename_parsed        = "/dev/null";
    filename_fully_parsed  = "/dev/null";
    filename_repacked      = "/dev/null";

    //Variables which are used later.
    intmax_t size = 0;

FUNCINFO("___________________________________________________________ Performing basic loading routines ____________________________________________________");

    FUNCINFO("About to load file " << filename_in );

    //Load the file.
    std::unique_ptr<unsigned char[]> file_in_memory = Load_Binary_File<unsigned char>(filename_in, &size);

    if(file_in_memory == nullptr) FUNCERR("Unable to load binary file");
    const unsigned char *begin = file_in_memory.get();
    const unsigned char *end = begin + size;

    FUNCINFO("The size of the file in flat memory is " << size );

    //Check the file for the existence of the 'DICM' signature. 
    const unsigned char *it = Validate_DICOM_Format(begin, end);
    if(it == nullptr) FUNCERR("Unexpected error - unable to validate file");
    if(it == end    ) FUNCERR("The file does not appear to be a valid DICOM file. Please double check it");

FUNCINFO("___________________________________________________________ Performing parsing routines __________________________________________________________");

    //De-lineate the data within the file. Note that the nodes output are NOT traversed and expanded.
    std::vector<piece> data = Parse_Binary_File(it, end);
    if(data.size() == 0) FUNCERR("No data was output: the file is either empty or there was an issue processing data");

    //Dump the non-fully-expanded data. Note that none of the children nodes have been expanded (if there are any..)
    std::ofstream out(filename_parsed.c_str(), std::ios::out );
    for(size_t i=0; i<data.size(); ++i){
        out << data[i] << std::endl;
    }
    out.close();
 
    //Expand all children nodes recursively.
    Delineate_Children(data);      //   <------ this call modifies the data vector (by appending new nodes.)

    //Now cycle through the data and print *only* data strings from those pieces which *do not* have a child.
    // All of this data will be (should be) fully delinearized.
    //Dump_Children(std::cout, data);
    std::ofstream fully_parsed(filename_fully_parsed.c_str(), std::ios::out );
    Dump_Children(fully_parsed, data);
    fully_parsed.close();

FUNCINFO("___________________________________________________________ Performing modification routines _____________________________________________________");

    //Select an element (or a nested element) to get the data from. Elements are returned as pointer to the in-place data. We can ask for references to however
    // high up the tree we wish.
    std::vector<piece *> selection;
//    Get_Elements(selection, data, {6291464});  //Should pull out the modality (RTSTRUCT, etc..)
    Get_Elements(selection, data, {2109446, 3758161918, 2502662});  //For RS files, should pick out the ROI contour names.
//    Get_Elements(selection, data, {2109446, 3758161918, 0});  //For RS files, will pick out the ROI contour names and all neighbouring at-depth data.
//    Get_Elements(selection, data, {3747846, 3758161918, 4206598, 3758161918, 1454086, 3758161918, 290783240 }); //For RS files, will pull out references to CT data near the contour data.


//THIS CODE WORKS FINE - WE JUST ENABLED "QUIET MODE"
/*
    //Print this data to screen.
    Dump_Children(std::cout, selection);


    //Modify this data in place. Note that we need not worry about the size of data - this is recomputed (recursively) as needed upon writing.
    for( auto i=selection.begin(); i!=selection.end(); ++i){
        if( (**i).data == (unsigned char*)("L PAROTID ") ){
            (**i).data = (unsigned char*)("VCC RULZ, VIC DRULZ");
            break;
        }
    }


    //Print out the data again to show the (possible) change which we have enacted.
    FUNCINFO("After attempting to modify the data, we now have..");
    Dump_Children(std::cout, selection);
*/
    //Do not use the vector filled by Get_Elements after this point - the data may get shuffled and reorganized!

FUNCINFO("___________________________________________________________ Performing writing routines __________________________________________________________");

    //Write the data to file. We make sure to re-compute the 'size' of objects, since any of them may have been altered in some way (or their children, or their
    // children's children, etc..)
    Prep_Children_For_Recompute_Children_Data_Size( data );
    Recompute_Children_Data_Size( data );

    std::basic_string<unsigned char> repacked;
    Repack_Nodes( data, repacked );

    //Append a (token) DICOM header so that the file will be recognized elsewhere.
    repacked = Simple_DICOM_Header() + repacked;
   
    FUNCINFO("The size of the repacked flat file is " << repacked.size());

    std::ofstream bin_out( filename_repacked.c_str(), std::ios::out | std::ios::binary );
    bin_out << repacked;
    bin_out.close();



    return 0;
}


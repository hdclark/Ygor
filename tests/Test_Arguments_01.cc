#include <iostream>
#include <string>
#include <functional>
#include <tuple>

#include "YgorMisc.h"
#include "YgorString.h"
#include "YgorArguments.h"

int main(int argc, char **argv){
    
    class ArgumentHandler arger;

    const std::string progname(argv[0]);
    arger.examples = { { "--help" , "Show the help screen and some info about the program." },
                       { "--test 'Something to Echo!'", "Echo the provided argument." }, 
                       { "--ahelp --another-option 'yes'", "Perform a bogus, ripoff help knockoff." } };
    arger.description = "A simple test program which highlights the use of the ArgumentHandler class.";

    //For unrecognized options. Optarg might kill this for us, though. Needs more testing!
    arger.default_callback    =  [](int opt, const std::string &optarg) -> void { FUNCINFO("Performing the DEFAULT directive: '" << opt << "' with arg '" << optarg << "'"); return; };

    //For passing in filenames in bulk, usually.
    arger.optionless_callback =  [](const std::string &optarg) -> void { FUNCINFO("Performing the OPTIONLESS directive!"); return; };

    //Uncomment this to override the --help argument. This is probably not a good idea, though, so it serves only to illustrate how it *might* be done.
    //arger.push_back( ygor_arg_handlr_t(123, 'h', "help", false, "The help directive should not really have a description!", [](const std::string &optarg) -> void { FUNCINFO("Performing the help directive!"); return; }) );
    arger.push_back( ygor_arg_handlr_t(0, 'a', "ahelp", false, "", "aThe help directive should not really have a description!", [](const std::string &optarg) -> void { FUNCINFO("Performing the ahelp directive!"); return; }) );
    arger.push_back( ygor_arg_handlr_t(0, 's', "shelp", false, "", "sThe help directive should not really have a description!", [](const std::string &optarg) -> void { FUNCINFO("Performing the shelp directive!"); return; }) );
    arger.push_back( ygor_arg_handlr_t(1, 'd', "dhelp", false, "", "dThe help directive should not really have a description!", [](const std::string &optarg) -> void { FUNCINFO("Performing the dhelp directive!"); return; }) );
    arger.push_back( ygor_arg_handlr_t(2, 'f', "fhelp", false, "", "fThe help directive should not really have a description!", [](const std::string &optarg) -> void { FUNCINFO("Performing the fhelp directive!"); return; }) );

    arger.push_back( ygor_arg_handlr_t(2, 't', "test", true, "'some text'", "An option which takes an argument and echoes it.", [](const std::string &optarg) -> void { FUNCINFO("Echoing argument: '" << optarg << "'"); return; }) );
 
    arger.Launch(argc, argv);


    return 0;
}

//YgorArguments.h - A wrapper around getopt (or similar) which uses a more-DRY-like method of handling main's arguments.
//                Common arguments, like '--help' are provided automatically.

#ifndef YGOR_ARGUMENTS_PROJECT_UTILITIES_HC_
#define YGOR_ARGUMENTS_PROJECT_UTILITIES_HC_

#include <iostream>
#include <iomanip>
#include <string>
#include <tuple>
#include <vector>
#include <list>
#include <utility>
#include <functional>
#include <set>

#include <unistd.h>            //Needed for getopt().
#ifdef __linux__ || __MINGW32__ || __MINGW64__
#include <getopt.h>            //Needed for getopt_long() which is a GNU extension.
#endif //__linux__ || __MINGW32__ || __MINGW64__

#include "YgorDefinitions.h"
#include "YgorMisc.h"            //Needed for function macros FUNCINFO, FUNCWARN, FUNCERR.
#include "YgorString.h"
#include "YgorEnvironment.h"     //Needed for Get_Terminal_Char_Dimensions();


// #########################################################################
// ##### NOTE:                                                       #######
// #####  This typedef has been made so that some compiler errors    #######
// #####  can be mitigated in gcc 4.8.1. If one uses std::make_tuple #######
// #####  with a 'bare' closure, then a long, nasty error is given   #######
// #####  about: "/usr/include/c++/4.8.1/functional: In substitution #######
// #####  of ‘template<class _Res, class ... _ArgType  ........"     #######
// #####  Using this typedef solves the problem. I'm not sure if it  #######
// #####  should stay or not, though. It sort of hides the fact that #######
// #####  it is a simple tuple.                                      #######
// #####                                                             #######
// #####  Remove this note and the typedef if it is desired, or      #######
// #####  just this note otherwise.                                  #######
// #########################################################################
typedef std::tuple<long int,char,std::string,bool,std::string,std::string,std::function<void(const std::string &optarg)>> ygor_arg_handlr_t;

class ArgumentHandler {
    private:
        std::string theprogsname; //Set when we launch the processing command.

    public:
        //This list holds all the directives or arguments (with their actions, description, etc..) we want to handle.
        //
        // NOTE: The 'family' number determines where to print the description when calling the --help argument. Families
        // which share an indentifier will be printed in the same section. The '--help' description itself is in the 0 family.
        // Families can be larger or smaller than 0, and will be printed in ascending order.
        //
        // NOTE: The 'sample argument' is used in the help menu, and should be a (string) of a sample argument (like a filename
        // or something similar.)
        //
        // Key:  family, short form, long form, takes argument?, sample argument, description, callback function.
        //         0         1           2             3               4               5               6
        std::list< std::tuple< long int, char, std::string, bool, std::string, std::string, std::function< void (const std::string &optarg) > > > directives;

        std::function< void (int, const std::string &optarg) > default_callback; //Gets called by default, when no others matches are found. 
        std::function< void (const std::string &optarg) > optionless_callback;   //Gets called for each optionless argument passed into main.

        std::string description;                                                 //A description of the program itself (for the help menu.)
        std::list<std::pair<std::string, std::string> > examples;                //Example invocations and a description (for the help menu.)

        //Constructor/Destructor.
        ArgumentHandler() {  
            //Register the default callbacks.
            this->default_callback = [](int opt, const std::string &optarg) -> void { 
                FUNCINFO("Received unrecognized option '" << opt << "' with argument '" << optarg << "'. Register callback to handle it if desired. Ignoring"); 
                return; 
            };

            this->optionless_callback = [](const std::string &optarg) -> void { 
                FUNCINFO("Received an optionless argument '" << optarg << "'. Register callback to handle it if desired. Ignoring");
                return; 
            };
 
            //Register the special --help directive.
            auto help_callback = [&](const std::string &optarg) -> void {
                //------------------------------------------------------ help output ---------------------------------------------------
                //Environmental information and common things.
#ifdef YGOR_USE_LINUX_SYS
                const auto terminal_dims = Get_Terminal_Char_Dimensions();
                const auto termWL = static_cast<long int>(terminal_dims.first);
                const long int termW = ((termWL < 80L) || (300L < termWL)) ? 120L : termWL;
#else
                const long int termW = 120L;
#endif // YGOR_USE_LINUX_SYS
                std::string DoubleLine, SingleLine(" ");
                for(long int i=0; i<termW; ++i) DoubleLine += '=';
                for(long int i=2; i<termW; ++i) SingleLine += '-';
                SingleLine += ' ';

                //Program's name.
                std::cout << DoubleLine << std::endl << Reflow_Line_Align_Center(this->theprogsname, termW) << std::endl;

                //We first output the program's description.
                std::cout << DoubleLine << std::endl << " Description." << std::endl << SingleLine << std::endl;
                const auto rf_des = Reflow_Text_to_Fit_Width_Left_Just(description, termW, 4);
                for(auto itt = rf_des.begin(); itt != rf_des.end(); ++itt) std::cout << *itt << std::endl;
                std::cout << std::endl;

                //Determine which families are out there, the maximum arg length, etc.. so we can group them together in the help usage.
                std::set<long int> uniq_families;
                size_t max_shrt(0), max_long(0), max_desc(0);
                for(auto d_it = directives.begin(); d_it != directives.end(); ++d_it){
                    uniq_families.insert( std::get<0>(*d_it) );

                    const size_t len_shrt = 1 + (std::get<4>(*d_it)).size() + 1;
                    const size_t len_long = (std::get<2>(*d_it)).size() + ("..."_s).size() + 1; // "..." instead of std::get<4>(*d_it).
                    const size_t len_desc = (std::get<5>(*d_it)).size();

                    if(max_shrt < len_shrt) max_shrt = len_shrt;
                    if(max_long < len_long) max_long = len_long;
                    if(max_desc < len_desc) max_desc = len_desc;
                }

                //The width of the text blocks for short options, long options, and descriptions. We will attempt to word-wrap the descriptions.
                const long int shrtW  = static_cast<long int>(max_shrt + 2); //And an extra space after.
                const long int longW  = static_cast<long int>(max_long + 2); //And an extra space after.
                const long int remain = termW - (longW + shrtW + 1);
                const long int descW  = (remain > 20) ? remain : 20; 

                //Now we cycle over the arguments, displaying them in a tabular fashion on the screen.
                std::cout << DoubleLine << std::endl << " Usage." << std::endl << SingleLine << std::endl;
                std::cout << " " << std::setiosflags(std::ios_base::left) << std::setfill(' ') << std::setw( shrtW + longW + 2) << "Option (short/long)";
                std::cout << " " << std::setiosflags(std::ios_base::left) << std::setfill(' ') << std::setw( descW - 3) << "Description";
                std::cout << std::endl;

                for(auto f_it = uniq_families.begin(); f_it != uniq_families.end(); ++f_it){
                    if(f_it != uniq_families.begin()) std::cout << std::endl;
                    std::cout << SingleLine << std::endl;

                    //bool firstnewlineskipped = false;
                    for(auto d_it = directives.begin(); d_it != directives.end(); ++d_it){
                        if( (*f_it) == std::get<0>(*d_it) ){
                           //if(!firstnewlineskipped){
                           //    firstnewlineskipped = true;
                           //}else{
                           //      std::cout << " " << std::endl;  //Put a space between descriptions. I prefer not to...
                           //}

                           //Short options.
                           std::string shrtandsmpl = std::get<1>(*d_it) + " "_s + std::get<4>(*d_it);
                           std::cout << " -" << std::setiosflags(std::ios_base::left) << std::setfill(' ') << std::setw( shrtW ) << shrtandsmpl;

                           //Long options.
                           std::string longandsmpl = std::get<2>(*d_it) + " "_s + (std::get<3>(*d_it) ? "..."_s : ""_s);
                           std::cout << "--" << std::setiosflags(std::ios_base::left) << std::setfill(' ') << std::setw( longW ) << longandsmpl;

                           //Descriptions.
                           const auto rf_des = Reflow_Text_to_Fit_Width_Left_Just(std::get<5>(*d_it), descW-4, -2);
                           for(auto dl_it = rf_des.begin(); dl_it != rf_des.end(); ++dl_it){
                               if(dl_it != rf_des.begin()) for(size_t i=0; i<(shrtW + longW + 2 + 1 + 1); ++i) std::cout << " ";
                               std::cout << *dl_it << std::endl;
                           }
                        }
                    }
                }

                //Now we check if any examples have been registered.
                std::cout << std::endl << DoubleLine << std::endl << " Examples." << std::endl;
                for(auto e_it = examples.begin(); e_it != examples.end(); ++e_it){
                    std::cout << SingleLine << std::endl;
                    std::cout << " " << this->theprogsname << " " << e_it->first << std::endl;

                    const auto rf_des = Reflow_Text_to_Fit_Width_Left_Just(e_it->second , termW-4, 2);
                    for(auto itt = rf_des.begin(); itt != rf_des.end(); ++itt) std::cout << "    "_s << *itt << std::endl;
                }
                std::cout << DoubleLine << std::endl;
                exit(0);
                //------------------------------------------------------ /help output ---------------------------------------------------
            };
            this->push_back( std::make_tuple( 0, 'h', "help", false, "", "Show (this) help information.", help_callback) );
        };


        //Member functions.
        void push_back( const std::tuple< long int, char, std::string, bool, std::string, std::string, std::function< void (const std::string &optarg) > > &directive){
            //Check for conflicting options??

            // ...
 
            directives.push_back( directive );


            return;
        };

        bool Launch(int local_argc, char **local_argv){  //This function performs the actual argument handling.
            this->theprogsname = ((local_argv != nullptr) && (local_argv[0] != nullptr)) ? local_argv[0] : "";

            //Check if there is anything to do.
            if(local_argc <= 1) return true;

            //First, we set up the getopt stuff.
            std::string short_opts;
            for(auto d_it = directives.begin(); d_it != directives.end(); ++d_it){
                short_opts += std::get<1>(*d_it);
                if( std::get<3>(*d_it) == true ) short_opts += ":";
            }

            std::vector<struct option> long_opts;
            for(auto d_it = directives.begin(); d_it != directives.end(); ++d_it){
                //long_opts.push_back( { description, (int)(takes arg?), nullptr, (char)(short form) } );
                const int takesarg = (std::get<3>(*d_it) == true) ? 1 : 0;
                const struct option shuttle = { (std::get<2>(*d_it)).c_str(), takesarg, nullptr, std::get<1>(*d_it) };
                long_opts.push_back( shuttle );
            }
            {
                const struct option shuttle = { NULL, 0, NULL, 0 };
                long_opts.push_back( shuttle ); //The manual says to terminate with all-zeros.
            }
            const std::vector<struct option> long_const_opts( long_opts );
            long_opts.clear(); //So we do not accidentally invalidate the pointer to the vector's data.

            //Now we loop over the arguments, calling the callbacks if necessary. We cycle through the directives 
            // backward so that more recently-pushed directives get priority over others.
            int next_options;
            do{
#ifdef __linux__ || __MINGW32__ || __MINGW64__
                next_options = getopt_long(local_argc, local_argv, short_opts.c_str(), &long_const_opts[0], nullptr);
#else
                next_options = getopt(local_argc, local_argv, short_opts.c_str());
#endif // __linux__ || __MINGW32__ || __MINGW64__
                bool matched = false;
                for(auto d_it = directives.rbegin(); d_it != directives.rend(); ++d_it){
                    if( next_options == std::get<1>(*d_it) ){
                        matched = true;
                        std::string theoptarg;
                        if(optarg != nullptr) theoptarg = optarg;
                        if( std::get<6>(*d_it) ){
                            (std::get<6>(*d_it))( theoptarg );
                        }else{
                            FUNCWARN("Encountered argument '" << next_options << "', but there is not a valid callback function to perform. Continuing");
                        }
                        break;
                    }
                }

                if((matched == false) && (next_options != -1)){
                    std::string theoptarg;
                    if(optarg != nullptr) theoptarg = optarg;
                    if(default_callback){
                        default_callback( next_options, theoptarg );
                    }else{
                        FUNCWARN("Unable to match argument '" << next_options << "', and unable to execute default callback. Continuing");
                    } 
                }

            }while(next_options != -1);
            //From the optind man page:
            //  "If the resulting value of optind is greater than argc, this indicates a missing option-argument,
            //         and getopt() shall return an error indication."
            for( ; optind < local_argc; ++optind){
                std::string theoptarg;
                if(local_argv[optind] != nullptr) theoptarg = local_argv[optind];
                if(optionless_callback){
                    optionless_callback( theoptarg );
                }else{
                    FUNCWARN("Unable to perform optionless argument processing because callback is not valid. Continuing");
                }
            }
            return true;
        };

};
#endif 

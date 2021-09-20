//YgorArguments.h - A portable getopt_long()-like parser that uses a more-DRY-like method of handling arguments.

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

#include "YgorDefinitions.h"
#include "YgorMisc.h"            //Needed for function macros FUNCINFO, FUNCWARN, FUNCERR.
#include "YgorString.h"
#include "YgorEnvironment.h"     //Needed for Get_Terminal_Char_Dimensions();


typedef std::function<void(const std::string &optarg)> ygor_arg_functor_t;
typedef std::tuple<long int,char,std::string,bool,std::string,std::string,ygor_arg_functor_t> ygor_arg_handlr_t;

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
        std::list< std::tuple< long int, char, std::string, bool, std::string, std::string, ygor_arg_functor_t > > directives;

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
            auto help_callback = [&](const std::string &) -> void {
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
        void push_back( const std::tuple< long int, char, std::string, bool, std::string, std::string, ygor_arg_functor_t > &directive){
            //Check for conflicting options??

            // ...
 
            directives.push_back( directive );
            return;
        };

        bool Launch(int local_argc, char **local_argv){  //This function performs the actual argument handling.
            this->theprogsname = ((local_argv != nullptr) && (local_argv[0] != nullptr)) ? local_argv[0] : "";

            //Check if there is anything to do.
            if(local_argc <= 1) return true;

            // Extract the tokens.
            std::list<std::string> tokens;
            for(int i = 1; i < local_argc; ++i){
                tokens.emplace_back( std::string(local_argv[i]) );
                tokens.back() = Canonicalize_String2( tokens.back(), CANONICALIZE::TRIM_ENDS );
            }

            // Combine tokens.
            const auto explode = [](const std::string &s){
                const auto N = s.size();
                const auto has_dash_1 = (0UL < N) && (s[0] == '-');
                const auto has_dash_2 = has_dash_1 && (1UL < N) && (s[1] == '-');

                const auto pos_key = s.find_first_not_of('-');

                // Only search for key-value structure if this is an option (i.e., preceeded by 1-2 dashes).
                const auto pos_equals = (has_dash_1 || has_dash_2) ? s.find('=') : std::string::npos;
                const auto has_equals = (pos_equals != std::string::npos);

                std::string key = (pos_key != std::string::npos) ? s.substr(pos_key) : s;
                std::string val;
                if( has_dash_1
                &&  (0UL < N)
                &&  (pos_key != std::string::npos) ){
                    key = s.substr(pos_key, pos_equals);
                    if(has_equals) val = s.substr(pos_equals);
                }
                return std::make_tuple(has_dash_1, has_dash_2, key, val);
            };

            long int processed_tokens = 0;
            const auto end = std::end(tokens);
            for(auto s_it = std::begin(tokens); s_it != end; ){
                // Valid cases to handle:
                //
                //   Short-form options:
                //    1. -a          (a does not accept/require an argument.)
                //    2. -a=abc      (a accepts/requires an argument.)
                //    3. -a abc      (a accepts/requires an argument.)
                //    4. -abc        (none of a, b, or c accept/require arguments.)
                //    5. -abc=123    (only c accepts/requires arguments.)
                //    6. -abc 123    (only c accepts/requires arguments.)
                //
                //   Long-form options:
                //    7. --a         (a is a long-form option.)
                //    8. --abc       (abc is a long-form option.)
                //    9. --abc=123
                //   10. --abc 123
                //
                //   Other options: (currently these are all treated as default options.)
                //   11. -
                //   12. --
                //   13. abc
                //   14. abc=123
                //
                auto [has_dash_1, has_dash_2, key, val] = explode(*s_it);

                // Capture the bound variable. Note that some (older) compilers disallowed this.
                // See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0588r1.html
                const auto read_ahead_for_argument = [&,key = key](){
                    ++s_it;
                    if(s_it == end){
                        throw std::invalid_argument("Option '"_s + key + "' requires an argument, but none were provided");
                    }
                    const auto [arg_has_dash_1, arg_has_dash_2, arg_key, arg_val] = explode(*s_it);
                    if( arg_key.empty()
                    ||  !arg_val.empty() ){
                        throw std::invalid_argument("Option '"_s + key + "' requires an argument, but '" + *s_it + "' was provided");
                    }
                    return arg_key;
                };

                const auto short_form = (key.size() == 1UL) && has_dash_1 && !has_dash_2;
                const auto long_form  = (1UL < key.size())  && has_dash_1 &&  has_dash_2;
                if( false ){
                }else if( short_form || long_form ){ // Cases 1, 2, 3,  7, 8, 9, and 10.
                    bool found = false;
                    // Note: cycle through directives backward so that more recently-pushed directives get priority over others.
                    for(auto d_it = directives.rbegin(); d_it != directives.rend(); ++d_it){
                        if(
                            (short_form && (std::get<char>(*d_it) == key.front())) // Cases 1, 2, and 3.
                        ||
                            (long_form && (std::get<2>(*d_it) == key)) // Cases 7, 8, 9, and 10.
                        ){
                            const auto wants_val = std::get<bool>(*d_it);
                            const auto has_val = !val.empty();
                            if(false){
                            }else if( !wants_val && !has_val ){
                                // Case 1. Do nothing.
                            }else if( wants_val && has_val ){
                                // Case 2. Do nothing.
                            }else if( wants_val && !has_val ){
                                // Case 3. Look to next token for val.
                                val = read_ahead_for_argument();
                            }else{ // !wants_val && has_val
                                throw std::invalid_argument("Option '-"_s + key + "' does not accept arguments");
                            }
                            // Invoke the user functor.
                            if( std::get<ygor_arg_functor_t>(*d_it) ){
                                (std::get<ygor_arg_functor_t>(*d_it))( val );
                            }else{
                                throw std::runtime_error("Option '-"_s + key + "' does not have a valid callback handler");
                            }
                            found = true;
                            break;
                        }
                    }
                    if(found){
                        ++s_it;
                        ++processed_tokens;
                        continue;
                    }
                    if(default_callback){
                        default_callback( processed_tokens, key );
                        ++s_it;
                        ++processed_tokens;
                        continue;
                    }
                    throw std::runtime_error("No valid callback available to handle unknown option '"_s + *s_it + "'");

                }else if( (1UL < key.size()) && has_dash_1 && !has_dash_2 ){ // Cases 4, 5, and 6.
                    // Convert cases 4, 5, and 6 to multiple case 1s and possibly a case 2 or 3 by splitting the tokens.
                    decltype(tokens) l_tokens;
                    for(const auto &c : key) l_tokens.emplace_back("-"_s + c);
                    tokens.splice(std::next(s_it), l_tokens);
                    ++s_it;
                    continue;

                }else if( (key.empty() && has_dash_1 && !has_dash_2) // Case 11.
                      ||  (key.empty() && has_dash_1 &&  has_dash_2) // Case 12.
                ){
                    if(optionless_callback){
                        optionless_callback( *s_it );
                        ++s_it;
                        ++processed_tokens;
                        continue;
                    }
                    throw std::runtime_error("No valid callback available for '"_s + *s_it + "'");

                }else if(!key.empty() && !has_dash_1){ // Cases 13 and 14.
                    if(optionless_callback){
                        optionless_callback( key );
                        ++s_it;
                        ++processed_tokens;
                        continue;
                    }
                    throw std::runtime_error("No valid callback available for '"_s + *s_it + "'");

                }else{
                    throw std::logic_error("Unexpected input: '"_s + *s_it + "'");
                }
            }
            return true;
        };
};

#endif 

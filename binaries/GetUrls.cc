//GetUrls.cc - Given some urls, get the html (or whatever), parse for all urls, and spit
// them out.
//
// It turns out that both are a PITA to do in C++. Why? Because parsing in C++ is not very
// fun and downloading is even less so. We thus outsource this to other programs.

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <utility>

#include "YgorMisc.h"
#include "YgorNetworking.h"
#include "YgorString.h"
#include "YgorFilesDirs.h"

#include "YgorURITools.h"
#include "YgorCONFIGTools.h"

//#include "htmlcxx/html/ParserDom.h"

int main(int argc, char **argv){
    if(argc < 2) FUNCERR("usage: " << argv[0] << " <urls/html filenames>");

    for(int i = 1; i < argc; ++i){
        const std::string url(argv[i]);
    
        bool IS_RSS = !GetFirstRegex(url, "rss$").empty();
        bool IS_FILE = Does_File_Exist_And_Can_Be_Read(url);
    
        //Load the html into a string.
        std::string page;
        if(IS_FILE){
            page = LoadFileToString(url);
    
        }else{
            page = Request_URL(url);
        
            //If the url is for an rss feed, we may have to decode the <,>,and &'s in the document
            // prior to parsing (else htmlcxx chokes!)
            if(IS_RSS){
                //For some reason, some rss is URL-encoded... (Vimeo!)
                page = Basic_Decode_URL( page ); 
            }
        }

        //Get all the links from the page, including the link text.
        std::list<std::pair<std::string,std::string>> thelinks = Get_Links(page);
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "src"));
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "SRC"));
    
        //Clean up (decode URL's, convert relative links to absolute.)
        std::list<std::pair<std::string, std::string> > fixedlinks;
        if(IS_FILE){
            fixedlinks = Clean_Links(thelinks, "");
        }else{
            fixedlinks = Clean_Links(thelinks, url);
        }
    
        //Dump just the links, one per line.
        if(true) for(auto it = fixedlinks.begin(); it != fixedlinks.end(); ++it){
            const std::string thelink(it->first);
            std::cout << thelink << std::endl;
        }
    
        //Dump the links and descriptions in a serialized format.
        if(false) for(auto it = fixedlinks.begin(); it != fixedlinks.end(); ++it){
            const std::string thelink(it->first);
            std::string thetext( PurgeCharsFromString(it->second, "'`\"") );
            Canonicalize_String(thetext, CANONICALIZE::TRIM);
    
            std::vector<std::vector<std::string>> configformat;
            configformat.push_back({ "LINK",        thelink });
            configformat.push_back({ "DESCRIPTION", thetext });
            std::cout << Double_Serialize_Simple_Config_File(configformat, "<---->", "<<---->>") << std::endl;        
        }
    }
    return 0;
}


//GetNutrtionfactsUrls.cc.

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

int main(int argc, char **argv){
    std::list<std::string> urls;
    urls.push_back("http://nutritionfacts.org/videos/");

    for(auto url_it = urls.begin(); url_it != urls.end(); ++url_it){
        std::string page = Request_URL(*url_it);
        bool IS_RSS = !GetFirstRegex(*url_it, "rss$").empty();

        //If the url is for an rss feed, we may have to decode the <,>,and &'s in the document
        // prior to parsing (else the parser chokes!)
        if(IS_RSS){
            page = Basic_Decode_URL( page );
        }
 
        //Get all the links from the page, including the link text.
        auto thelinks = Get_Links(page);
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "src"));    
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "SRC"));    

        //Clean up (decode URL's.)
        auto fixedlinks = Clean_Links(thelinks, "");
        decltype(fixedlinks) videolinks;
     
        videolinks = Filter_Whitelist_Links(fixedlinks, R"***([.]youtu[.]{0,1}be|vimeo)***", "");  
 
        //Dump the links and descriptions in a serialized format.
        for(auto it = videolinks.begin(); it != videolinks.end(); ++it){
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


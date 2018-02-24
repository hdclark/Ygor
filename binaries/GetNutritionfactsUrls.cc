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

    for(auto & url : urls){
        std::string page = Request_URL(url);
        bool IS_RSS = !GetFirstRegex(url, "rss$").empty();

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
        for(auto & videolink : videolinks){
            const std::string thelink(videolink.first);
            std::string thetext( PurgeCharsFromString(videolink.second, "'`\"") );
            Canonicalize_String(thetext, CANONICALIZE::TRIM);

            std::vector<std::vector<std::string>> configformat;
            configformat.push_back({ "LINK",        thelink });
            configformat.push_back({ "DESCRIPTION", thetext });
            std::cout << Double_Serialize_Simple_Config_File(configformat, "<---->", "<<---->>") << std::endl;        
        }
    }
    return 0;
}


//GetVimeoUrls.cc - A program which attempts to grab a few select video links from Vimeo.

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
    urls.push_back("http://vimeo.com/channels/staffpicks/videos");
    urls.push_back("http://vimeo.com/channels/staffpicks/videos/page:2/sort:preset");
//    urls.push_back("http://vimeo.com/channels/staffpicks/page:2");
//    urls.push_back("http://vimeo.com/channels/staffpicks/page:3");
//    urls.push_back("http://vimeo.com/categories/technology/programming");
//    urls.push_back("http://vimeo.com/categories/technology/chemistry");
//    urls.push_back("http://vimeo.com/categories/technology/physics");
//    urls.push_back("http://vimeo.com/categories/technology/software");

    for(auto url_it = urls.begin(); url_it != urls.end(); ++url_it){
        const std::string page = Request_URL(*url_it);
        
        //Get all the links from the page, including the link text.
        auto thelinks = Get_Links(page);
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "src"));
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "SRC"));
    
        //Clean up (decode URL's, convert relative links to absolute.)
        auto fixedlinks = Clean_Links(thelinks, "https://vimeo.com");
    
        decltype(fixedlinks) videolinks;
        videolinks = Filter_Whitelist_Links(fixedlinks, R"***([/][0-9]{7,})***", "");  

        videolinks = Filter_Blacklist_Links(videolinks, R"***(jpg$)***", "");     

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


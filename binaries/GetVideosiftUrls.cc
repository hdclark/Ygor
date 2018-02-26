//GetVideosiftUrls.cc - A program which attempts to grab all video links from Videosift.

#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "YgorCONFIGTools.h"
#include "YgorString.h"
#include "YgorURITools.h"

int main(int  /*argc*/, char ** /*argv*/){
    std::list<std::string> urls;
    urls.emplace_back("http://videosift.com");
    urls.emplace_back("http://videosift.com/top");

    for(auto & url : urls){
        const std::string page = Request_URL(url);
        
        //Get all the links from the page, including the link text.
        auto thelinks = Get_Links(page);
//        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "src"));
//        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "SRC"));
 
        //Clean up (decode URL's, convert relative links to absolute.)
        auto fixedlinks = Clean_Links(thelinks, url);
    
        decltype(fixedlinks) videolinks;
        videolinks = Filter_Whitelist_Links(fixedlinks, R"***([/]video[/])***", "");

        videolinks = Filter_Blacklist_Links(videolinks, R"***([#]comment)***", "");    
        videolinks = Filter_Blacklist_Links(videolinks, "", "more inside"); 
        videolinks = Filter_Blacklist_Links(videolinks, "", "here");

        //Remove all duplicate links, preferring those links which have a description.
        videolinks = Remove_Duplicate_Links(videolinks);

        //Now, for each of the links, we download THAT page and search for the actual video links.
        for(auto & videolink : videolinks){
            const std::string thispage = Request_URL(videolink.first);
            const std::string topleveldescription(videolink.second);

            auto actualvidlinks = Get_Tags_Attributes(thispage, "iframe", "src");
            actualvidlinks = Clean_Links(actualvidlinks, "");  //Decode any URL-encoding. These links should be absolute already.
    
            //actualvidlinks = Filter_Blacklist_Links(actualvidlinks, R"***([fF]acebook)***", "");
            //actualvidlinks = Filter_Blacklist_Links(actualvidlinks, R"***(amazon)***", "");
    
            actualvidlinks = Filter_Whitelist_Links(actualvidlinks, R"***(youtu[.]{0,1}be|vimeo)***", "");
   
            //Dump the links and descriptions in a serialized format.
            for(auto & actualvidlink : actualvidlinks){
                const std::string thelink(actualvidlink.first);
                std::string thetext( PurgeCharsFromString(topleveldescription, "'`\"") );
//                std::string thetext( PurgeCharsFromString(it->second, "'`\"") );
                Canonicalize_String(thetext, CANONICALIZE::TRIM);

                std::vector<std::vector<std::string>> configformat;
                configformat.push_back({ "LINK",        thelink });
                configformat.push_back({ "DESCRIPTION", thetext });
                std::cout << Double_Serialize_Simple_Config_File(configformat, "<---->", "<<---->>") << std::endl;        
            }
        }
    }

    return 0;
}


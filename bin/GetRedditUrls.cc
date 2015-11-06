//GetRedditUrls.cc - A program which attempts to grab a few generic video links from Reddit.
// We could get anything, but try to get mostly shorter videos of interest, leaving the
// longer things like movies, documentaries, and stand-up comedy for other programs.

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
    //If we can avoid it, do not use RSS pages. They do not have useful text in the link text.
    std::list<std::string> urls; 

    //Standard generic things - typically around 5 minutes long or so.
    urls.push_back("http://www.reddit.com/r/videos/top/?sort=top&t=week&limit=100");
    urls.push_back("http://www.reddit.com/r/youtube/top/?sort=top&t=week&limit=100");
    urls.push_back("http://www.reddit.com/r/youtubehaiku/top/?sort=top&t=week&limit=100");

    //Slightly longer things.
//    urls.push_back("http://www.reddit.com/r/trailers/top/?sort=top&t=week&limit=100");
//    urls.push_back("http://www.reddit.com/r/standupcomedy/top/?sort=top&t=week&limit=100");

    for(auto url_it = urls.begin(); url_it != urls.end(); ++url_it){
        std::string page = Request_URL(*url_it);

        //If the url is for an rss feed, we may have to decode the <,>,and &'s in the document
        // prior to parsing (else the parser chokes!)
        if(!GetFirstRegex(*url_it, "rss$").empty()){
            page = Basic_Decode_URL( page );
        }
 
        //Get all the links from the page, including the link text.
        auto thelinks = Get_Links(page);
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "src"));
        thelinks.splice(thelinks.begin(), Get_Tags_Attributes(page, "", "SRC"));
 
        //Clean up (decode URL's.)
        auto fixedlinks = Clean_Links(thelinks, "");
    
        //Filter the links to remove likely garbage.
        decltype(fixedlinks) videolinks;
   
        //Filter in special links. Whitelist from the source of all links. 
        videolinks.splice(videolinks.end(), Filter_Whitelist_Links(fixedlinks, R"***([.]youtu[.]{0,1}be)***", ""));  
        videolinks.splice(videolinks.end(), Filter_Whitelist_Links(fixedlinks, R"***(vimeo)***", ""));
        videolinks.splice(videolinks.end(), Filter_Whitelist_Links(fixedlinks, R"***(dailymotion)***", ""));                   

        //Filter out the crap. Blacklist only from the whitelisted links.
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([cC][aA][tT] )***");  //Trying to be careful not to remove things like 'category' too.
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([dD][oO][gG] )***"); 
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([cC][uU][tT][eE] )***");
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([bB][oO][rR][nN] )***");
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([kK][iI][tT][tT][eE][nN])***");
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([pP][uU][pP][pP])***");  //Puppy, puppies.
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([bB][aA][bB][yY])***");
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([tT][oO][dD][dD][lL][eE][rR])***");
        videolinks = Filter_Blacklist_Links(videolinks, "", R"***([aA][dD][oO][rR][aA][bB][lL][eE])***");

        //Search for duplicate links. Given a duplicate, we prefer the one with a description.
        videolinks = Remove_Duplicate_Links(videolinks);

        //Dump the links directly, one per line.
        for(auto it = videolinks.begin(); it != videolinks.end(); ++it){
            const std::string thelink(it->first);
            std::cout << thelink << std::endl;
        }
 

        //Dump the links and descriptions in a serialized format.
        /*
        for(auto it = videolinks.begin(); it != videolinks.end(); ++it){
            const std::string thelink(it->first);
            std::string thetext( PurgeCharsFromString(it->second, "'`\"") );
            Canonicalize_String(thetext, CANONICALIZE::TRIM);

            std::vector<std::vector<std::string>> configformat;
            configformat.push_back({ "LINK",        thelink });
            configformat.push_back({ "DESCRIPTION", thetext });
            std::cout << Double_Serialize_Simple_Config_File(configformat, "<---->", "<<---->>") << std::endl;        
        }
        */
    }
    return 0;
}


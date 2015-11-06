//YgorURITools.cc - A collection of "shaky-at-best" routines for doing common things with URI's/URL's.

#include <cstdio>
#include <string>
#include <vector>

#include "YgorMisc.h"
#include "YgorString.h"

#include "YgorURITools.h"

#include "htmlcxx/html/ParserDom.h"


std::string Request_URL(const std::string &url){
    //std::string cmd = "wget --quiet  --convert-links --level=1 --no-cache "_s;
    std::string cmd = "wget --quiet                  --level=1 --no-cache "_s;
    cmd += " --user-agent="_s;
    cmd += Quote_Static_for_Bash("Mozilla/5.0 (X11; Linux i686) AppleWebKit/536.11 (KHTML, like Gecko) Chrome/20.0.1132.57 Safari/536.11");
    cmd += " "_s + Quote_Static_for_Bash(url) + " -O - "_s;
    return Execute_Command_In_Pipe(cmd);
}



//This is a helper function for the htmlcxx library. Originally found at 
// http://stackoverflow.com/questions/5081181/htmlcxx-c-crawling-html?rq=1 on September 1st, 2012,
// this version is modified to recursively get the text from ALL child nodes.
static std::string get_child_content(tree<htmlcxx::HTML::Node> const &dom, tree<htmlcxx::HTML::Node>::iterator const &parent){
    std::string result;
    for(unsigned i=0; i<dom.number_of_children(parent); i++ ){
        auto it = dom.child(parent,i);
        if(!it->isTag() && !it->isComment()){
            result += it->text();
//-----------------------
//Modified part.
//        }
        }else{
            result += get_child_content(dom, it);
        }
//-----------------------

    }
    return result;
}


//Pull all links from an HTML string. Each link is accompanied with the raw text found between the tags.
//
//NOTE: This will NOT pull all URL's from HTML - only bonafide links (which are in valid <A HREF="...">
// tags.) For getting SRC="..." urls, use the Get_All_Tag_Attributes() below.
//
//NOTE: This is a specialization of the more general routine Get_Tags_Attributes(....).
std::list<std::pair<std::string, std::string> > Get_Links(const std::string &source){
    std::list<std::pair<std::string, std::string> > out;

    //Parse the page.
    htmlcxx::HTML::ParserDom parser;
    auto dom = parser.parseTree(source);

    //Cycle through the tree of nodes and dump all links.
    for(auto it = dom.begin(); it != dom.end(); ++it){

        //Check if the tag is an <a href="..."> -like tag.
        if((it->isTag()) && (strcasecmp(it->tagName().c_str(), "A") == 0)){
            it->parseAttributes();
            const std::string thelink = it->attribute("href").second;
            const std::string thetext = get_child_content(dom, it);
 
            if(!thelink.empty()){
                out.push_back( { thelink, thetext } );
            }
        }
    }

    return out;
}



//Pull all given tags' attributes from an HTML string. Each link is accompanied with the raw text found within the tag.
//
//NOTE: The terminology 'tag', 'attribute', and 'text' mean:
//              ... < TAG ... ATTR="..." ...>   TEXT  </TAG>
// for which we will return:        ^^^   and   ^^^^. 
//
//NOTE: Leave "tag" empty to match all tags.
std::list<std::pair<std::string, std::string> > Get_Tags_Attributes(const std::string &source, const std::string &tag, const std::string &attr){
    std::list<std::pair<std::string, std::string> > out;

    //Parse the page.
    htmlcxx::HTML::ParserDom parser;
    auto dom = parser.parseTree(source);

    //Cycle through the tree of nodes and dump all links.
    for(auto it = dom.begin(); it != dom.end(); ++it){

        //Check if the tag matches.
        if((it->isTag()) &&  (tag.empty() || (strcasecmp(it->tagName().c_str(), tag.c_str()) == 0)) ){
            it->parseAttributes();
            const std::string theattr = it->attribute(attr.c_str() ).second;
            const std::string thetext = get_child_content(dom, it);

            //NOTE: Some tags may not contain any text. Do not filter on absence of text!
            if(!theattr.empty()){
                out.push_back( { theattr, thetext } );
            }
        }
    }

    return out;
}


//Clean up links by attempting to decode URL-encodings and inserting a base name for relative links.
std::list<std::pair<std::string, std::string> > Clean_Links(const std::list<std::pair<std::string, std::string> > &thelinks, const std::string &base){
    std::list<std::pair<std::string, std::string> > fixedlinks;

    for(auto up_it = thelinks.begin(); up_it != thelinks.end(); ++up_it){

        //The link is the first element, the text is the second.
        std::string thelink = up_it->first;
        std::string thetext = up_it->second;

        //Attempt to convert URL-encoded links to their original.
        thelink = Basic_Decode_URL(thelink);
        if(thelink.empty()) continue;
        thetext = Basic_Decode_URL(thetext);

        //Attempt to convert links to absolute form.
        if((thelink[0] == '/') || (thelink[0] == '#') || (thelink[0] == '?')){
            thelink = base + thelink;

        //Otherwise it is probably an absolute link.
        }else{
            //Do nothing?
        }

        //Push back the link.
        fixedlinks.push_back( { thelink, thetext } );
    }

    return fixedlinks;
}

//Remove duplicate links. Prefer those links with non-empty descriptions, but otherwise remove the first.
std::list<std::pair<std::string, std::string>> Remove_Duplicate_Links(const std::list<std::pair<std::string, std::string>> &thelinks){
    std::list<std::pair<std::string,std::string>> out(thelinks);
    for(auto it = ++(out.begin()); it != out.end();   ){
        for(auto it2 = out.begin(); it2 != it;   ){
            if(it->first == it2->first){
                if(!(it->second.empty())){
                    it2 = out.erase(it2);
                }else{
                    it = out.erase(it);
                    --it; //Go back so we can continue checking this duplicate.
                }
            }else{
                ++it2;
            }
        }
        ++it;
    }
    return out;
}


//Filter links based on two regex criteria (one for the links, one for the text.)
//
//To omit a criteria, simply send an empty string.
std::list<std::pair<std::string, std::string> > Filter_Whitelist_Links(const std::list<std::pair<std::string, std::string> > &thelinks, const std::string &link_regex, const std::string &text_regex){
    std::list<std::pair<std::string, std::string> > out;
    for(auto up_it = thelinks.begin(); up_it != thelinks.end(); ++up_it){
        //The link is the first element, the text is the second.
        const std::string thelink = up_it->first;
        const std::string thetext = up_it->second;
        if(!link_regex.empty() && (GetFirstRegex(thelink, link_regex)).empty()) continue;
        if(!text_regex.empty() && (GetFirstRegex(thetext, text_regex)).empty()) continue;

        //Push back the link.
        out.push_back( { thelink, thetext } );
    }
    return out;
}

std::list<std::pair<std::string, std::string> > Filter_Blacklist_Links(const std::list<std::pair<std::string, std::string> > &thelinks, const std::string &link_regex, const std::string &text_regex){
    std::list<std::pair<std::string, std::string> > out;
    for(auto up_it = thelinks.begin(); up_it != thelinks.end(); ++up_it){
        //The link is the first element, the text is the second.
        const std::string thelink = up_it->first;
        const std::string thetext = up_it->second;
        if(!link_regex.empty() && !(GetFirstRegex(thelink, link_regex)).empty()) continue;
        if(!text_regex.empty() && !(GetFirstRegex(thetext, text_regex)).empty()) continue;

        //Push back the link.
        out.push_back( { thelink, thetext } );
    }
    return out;
}


//Attempt to guess a (reasonable) mime type. The goal is to only implement things I need!
std::string Guess_Mime_Type(const std::string &filename){
    //Some simple ones. These *could* be removed if desired, but are probably beneficial to have around.
    if(!GetFirstRegex(filename, R"***([.][cC][sS][sS]$)***").empty()) return "text/css";    
    if(!GetFirstRegex(filename, R"***([.][cC][sS][vV]$)***").empty()) return "text/csv";
    if(!GetFirstRegex(filename, R"***([.][hH][tT][mM][lL]$)***").empty()) return "text/html";

    if(!GetFirstRegex(filename, R"***([.][jJ][sS]$)***").empty()) return "application/javascript";
    if(!GetFirstRegex(filename, R"***([.][gG][zZ]$)***").empty()) return "application/gzip";

    if(!GetFirstRegex(filename, R"***([.][mM][pP]4$)***").empty()) return "application/mp4";


    // ...

    //Appeal to a higher authority.
    const std::string out = Execute_Command_In_Pipe(" file -b --mime-type -- "_s + Quote_Static_for_Bash(filename) + " | tr -d '\\\n' "_s);

    //Overrides.

    return out;
}

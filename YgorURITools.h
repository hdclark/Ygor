//YgorURITools.h.

#ifndef YGOR_UTILITIES_URITOOLS_H_
#define YGOR_UTILITIES_URITOOLS_H_
#include <utility>
#include <list>
#include <string>
#include <vector>

//############################################################################
//NOTE: See the source for more detailed information regarding each function.
//############################################################################

//Send a GET request to a server to receive the item with the given url. 
//
// NOTE: Not guaranteed to be native code: it will probably invoke a shell 
// program (wget, curl, ...).
std::string Request_URL(const std::string &url);

//Pull all links from a file. Each link is accompanied with the raw text found between the tags.
std::list<std::pair<std::string, std::string>> Get_Links(const std::string &source);

//Pull all given tags' attributes from an HTML string. Each link is accompanied with the raw text found within the tag.
std::list<std::pair<std::string, std::string>> Get_Tags_Attributes(const std::string &source, const std::string &tag, const std::string &attr);

//Clean links (decode encoded-URL's, convert relative links to absolute, etc..) which have
// been spit out by Get_Links (or similar, simple link scraping routine.)
std::list<std::pair<std::string, std::string>> Clean_Links(const std::list<std::pair<std::string, std::string> > &thelinks, const std::string &base);

//Remove duplicate links. Prefer those links with non-empty descriptions, but otherwise remove the first.
std::list<std::pair<std::string, std::string>> Remove_Duplicate_Links(const std::list<std::pair<std::string, std::string>> &thelinks);

//Filter links based on two regex criteria (one for the links, one for the text.)
//
//NOTE: To ignore one of the criteria, provide an empty string.
std::list<std::pair<std::string, std::string>> Filter_Whitelist_Links(const std::list<std::pair<std::string, std::string>> &thelinks, const std::string &link_regex, const std::string &text_regex);
std::list<std::pair<std::string, std::string>> Filter_Blacklist_Links(const std::list<std::pair<std::string, std::string>> &thelinks, const std::string &link_regex, const std::string &text_regex);

//Attempt to guess a (reasonable) mime type. 
std::string Guess_Mime_Type(const std::string &filename);


#endif

//YgorVIDEOTools.cc - A collection of routines for dealing with video and video files.

#include <utility>
#include <string>

#include "YgorMisc.h"
#include "YgorString.h"
#include "YgorFilesDirs.h"
#include "YgorVIDEOTools.h"

//Returns the W and H (in pixels) of a video file. If an error is encountered, we return two -1's.
std::pair<long int, long int> YgorVIDEOTools_Get_Video_Dimensions(const std::string &filename){
    auto out = std::pair<long int, long int>(-1,-1);
//    if(Does_File_Exist_And_Can_Be_Read(filename)){ //Let's handle urls too!
        std::string command;
//FUNCWARN("Need to verify this timeout code is working OK");
//        command += "timeout --kill-after=10s --preserve-status 60s -- ";
        command += "mplayer -identify -frames 0 -vc null -vo null -ao null "; //Sometimes it locks up and uses 100% cpu until you kill it...

        command += Quote_Static_for_Bash(filename);
        command += " 2>/dev/null";

        const std::string info = Execute_Command_In_Pipe(command);

        const std::string W = GetFirstRegex(info, "ID_VIDEO_WIDTH=([0-9]{2,})");
        const std::string H = GetFirstRegex(info, "ID_VIDEO_HEIGHT=([0-9]{2,})");

        if(Is_String_An_X<long int>(W) && Is_String_An_X<long int>(H)){
            out = std::pair<long int, long int>(stringtoX<long int>(W), stringtoX<long int>(H));
        }
//    }        
    return out;
}


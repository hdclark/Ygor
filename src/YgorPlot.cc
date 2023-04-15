//YgorPlot.cc - Simple plotting routines which wrap around Gnuplot, (GNU)plotutils, a postscript plotting library, or good-ol'-fashioned 
// roll-your-own plotting routines.
//
//The idea behind this file is to easily have some plotting facilities within a program without having to deal directly with pipes and
// plotting formats in situ. 

#include "YgorMath.h"
#include <array>
#include <cstdio>    //Used for popen, FILE class.
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMisc.h"    //Used for the macros FUNCINFO, FUNCWARN, FUNCERR.
#include "YgorLog.h"
#include "YgorPlot.h"
#include "YgorString.h"


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------- Plotter - A Plotutils-based class ---------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Plotter::Plot(void){
    //First, check if there is any data in the stream and/or in the string.
    std::string from_the_stream = ss.str();
    bool stream_empty = from_the_stream.empty();
    bool string_empty = plotting_data.empty();

    if( stream_empty && string_empty ){
        //Then we do nothing. There is no data to send off.
        return false;    
    }

    if( (!stream_empty && string_empty) || ( !stream_empty && !string_empty ) ){
        //Then we have to place the stream contents into the string. We make sure to append the data.
        plotting_data += " " + from_the_stream;
        ss.str(""); // (empty the stringstream)

    }else if( stream_empty && !string_empty ){
        //Then the data is already in the string. No need to adjust anything.

    }

    if( fp != nullptr ){
        //Now we plot the string.
        fprintf(fp, "%s", plotting_data.c_str());
        plotting_data.clear();
        return true;
    }

    //If we could not plot, we simply hold onto the data (in the string) so that we might be able to plot it later after resolving the pipe issue.
    return false;
}

void Plotter::Iterate_Linestyle(void){
    //  From the plotutils documentation:
    // ..."For colored datasets, the line can be drawn in one of 25 distinct styles. Linemodes #1 through #5 signify red, green, blue, magenta, 
    //     and cyan; all are solid. Linemodes #6 through #10 signify the same five colors, but dotted rather than solid. Linemodes #11 through 
    //     #16 signify the same five colors, but dotdashed, and so forth. After linemode #25, the sequence repeats. Linemode #0, irrespective 
    //     of whether the rendering is in monochrome or color, means that the line is not drawn." ...
    ++linestyle_m;  //The line mode (solid, dashed, etc..) 
    ++linestyle_s;  //The line (point) style (dot, star, triangle, etc..) There are 31 distinct, native styles.

    if(linestyle_m == 0) linestyle_m = 1;
    if(linestyle_s > 31) linestyle_s = 1;

    //Although this looks like a comment, we can direct plotutils with specially-formated comments like this.
    ss << "#m=" << linestyle_m << ",S=" << linestyle_s << std::endl;
    return;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------- Plotter2 - A GNUplot-based class ----------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Constructor/Destructor.
Plotter2::Plotter2(){
    this->Apply_Basic_Header_Customizations();
    this->Apply_Basic_Footer_Customizations();
}
Plotter2::~Plotter2(){}

//Internal methods. Do not use unless you know what you are doing.
void Plotter2::Apply_Basic_Header_Customizations(void){
    this->header.emplace_back("# set term pdfcairo enhanced color solid font 'cmr10,12' size 7.5in,5.5in"); //Helpful comment.
    this->header.emplace_back("# set output ''"); //Helpful comment.

    //this->header.push_back("set term x11"); //I like this better than the default. To go with default comment this out.

    const std::string linewidth("1.1"), pointsize("0.4");
    const std::string pt_circ_solid("5"), pt_box_solid("9"), pt_tri_solid("13"), pt_dimnd_solid("11"), pt_invtri_solid("7");
    int64_t N(0);
    //These are tweakable. Try to favour dark colours (why in the fuck does GNUplot use light teal and light yellow?!)
    this->header.push_back("set pointsize "_s + pointsize);
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#000000' lw "_s + linewidth + " pt "_s + pt_circ_solid  );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#1D4599' lw "_s + linewidth + " pt "_s + pt_circ_solid  );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#11AD34' lw "_s + linewidth + " pt "_s + pt_circ_solid  );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#E62B17' lw "_s + linewidth + " pt "_s + pt_circ_solid  );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#E69F17' lw "_s + linewidth + " pt "_s + pt_box_solid   );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#2F3F60' lw "_s + linewidth + " pt "_s + pt_box_solid   );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#2F6C3D' lw "_s + linewidth + " pt "_s + pt_box_solid   );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#8F463F' lw "_s + linewidth + " pt "_s + pt_box_solid   );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#8F743F' lw "_s + linewidth + " pt "_s + pt_dimnd_solid );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#031A49' lw "_s + linewidth + " pt "_s + pt_dimnd_solid );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#025214' lw "_s + linewidth + " pt "_s + pt_dimnd_solid );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#6D0D03' lw "_s + linewidth + " pt "_s + pt_dimnd_solid );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#6D4903' lw "_s + linewidth + " pt "_s + pt_tri_solid   );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#224499' lw "_s + linewidth + " pt "_s + pt_tri_solid   );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#00FF00' lw "_s + linewidth + " pt "_s + pt_tri_solid   );
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#00BBBB' lw "_s + linewidth + " pt "_s + pt_invtri_solid);
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#000000' lw "_s + linewidth + " pt "_s + pt_invtri_solid);
    this->header.push_back("set style line "_s + Xtostring<int64_t>(++N) + " lc rgb '#000000' lw "_s + linewidth + " pt "_s + pt_invtri_solid);
    this->header.push_back("set style increment user"_s);
    this->header.push_back("set grid lc rgb '#000000'"_s);

    //These colours were tweaked by hand. They are 6 orthogonal colours which remain orthogonal when flattened to greyscale.
    // They are good for histograms or filled curves.
    //black
    //#126B12 - darkened forest-green
    //#FFE730 - light yellow (not suitable for thin lines)
    //#382D5B - dark violet
    //#008B8B - turquise
    //#FFB540 - peachy

    this->header.emplace_back("set zero 1e-50");
    return;
}
void Plotter2::Apply_Basic_Footer_Customizations(void){
    this->footer.emplace_back("# set output"); //Helpful comment.
    this->footer.emplace_back("# set term pop"); //Helpful comment.
    return;
}
void Plotter2::Append_Header(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");
    for(const auto & s_it : this->header) *ss << s_it << std::endl;
    return;
}
void Plotter2::Append_StrDat(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");

    //To avoid writing everything twice, we copy everything at runtime. Ouch! This is the cost of 
    // a minor decrease in maintenance burden...
    std::list<Plotter2_Helper> combined(this->data);
    if(!(this->working.data.samples.empty())) combined.push_back(this->working);

    //All the data we have cached.
    *ss << "plot 1/0 title '' ";  //Undefined first plot is ignored and most convenient. 
    for(auto & p2h_it : combined){
        *ss << R"***( , \)***" << std::endl << R"***(    "< echo -e ')***";     //"//Incorrect parsing of raw literals :/.
        for(auto v2_it = p2h_it.data.samples.begin(); v2_it != p2h_it.data.samples.end(); ++v2_it){
            //Gnuplot assumes the form of:
            //     (x, y, ydelta),
            //     (x, y, ylow, yhigh),
            //     (x, y, xdelta),
            //     (x, y, xlow, xhigh),
            //     (x, y, xdelta, ydelta), or    // <----- This is what we're using.
            //     (x, y, xlow, xhigh, ylow, yhigh).
            *ss << (*v2_it)[0] << " " << (*v2_it)[2] << " " << (*v2_it)[1] << " " << (*v2_it)[3] << R"***(\n)***";
        }
        *ss << R"***('" )***";                //"//Incorrect parsing of raw literals :/.
        *ss << "with  " << (p2h_it.type.empty() ? "xyerrorbars" : p2h_it.type) << " ";
        /*if(!p2h_it->title.empty())*/ *ss << "title \"" << p2h_it.title << "\" ";
    }
    *ss << std::endl;
    return;
}
void Plotter2::Append_Footer(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");
    for(const auto & s_it : this->footer) *ss << s_it << std::endl;

    //Also dump stringified samples_1D data so we can more easily read/manipulate the data, if needed.
    *ss << "# Stringified samples_1D raw data (in corresponding order as plotted)" << std::endl;
    for(const auto & p2h_it : this->data){
         *ss << "# " << p2h_it.data << std::endl;
    }
    if(!(this->working.data.samples.empty())){
         *ss << "# " << this->working.data << std::endl;
    }

    return;
}

//Methods.
void Plotter2::Set_Global_Title(const std::string &in){
    //this->header.push_back("set title '"_s + in + "'"_s);  //Using '' instead of "" will annoyingly forgo expansion of \n.
    this->header.push_back("set title \""_s + in + "\""_s); 
    return;
}

//Routines which use the working buffer and have internal state.
void Plotter2::Set_Current_Line_Title(const std::string &in){
    this->working.title = in;
    return;
}
void Plotter2::Set_Current_Line_Type(const std::string &in){  //'lines', 'points', or 'linespoints'
    this->working.type = in;
    return;
}
void Plotter2::Insert(double x, double y){
    this->working.data.push_back(vec2<double>(x,y));
    return;
}
void Plotter2::Next_Line(void){
    this->data.push_back(this->working);
    this->working = Plotter2_Helper();
    return;
}

//Routines which have minimal internal state.
void Plotter2::Insert_samples_1D(const samples_1D<double> &in, const std::string &title/*=""*/, const std::string &linetype/*=""*/){
    Plotter2_Helper shtl;
    shtl.data = in;
    shtl.title = title;
    shtl.type = linetype;
    this->data.push_back(shtl);
    return;
}
void Plotter2::Insert_map_of_string_and_samples_1D(const std::map<std::string, samples_1D<double>> &in, const std::string &linetype/*=""*/){
    for(const auto & m_it : in){
        this->Insert_samples_1D(m_it.second, m_it.first, linetype);
    }
    return;
}

//Various plotting options.
bool Plotter2::Plot(void) const {
    //We now gather all the pieces to produce a single string which is piped to GNUplot.
    std::stringstream ss;
    this->Append_Header(&ss);
    ss << "plot sin(x)" << std::endl;
    ss << "plot cos(x)" << std::endl;
    this->Append_StrDat(&ss);
    this->Append_Footer(&ss);

    //Pipe it.
    FILE *fp = nullptr;
    if(!(fp = popen("gnuplot -persist &>/dev/null", "w"))) return false;
    if(!ss.good()) return false;
    fprintf(fp, "%s", ss.str().c_str());
    if(pclose(fp) == -1) return false; //YLOGERR("Unable to close pipe. Is the process still active?");
    return true;
}
bool Plotter2::Plot_as_PDF(const std::string &Filename) const { //May overwrite existing files!
    //We now gather all the pieces to produce a single string which is piped to GNUplot.
    std::stringstream ss;
    this->Append_Header(&ss);
    ss << "set term pdfcairo enhanced color solid font 'cmr10,12' size 7.5in,5.5in" << std::endl; //Hopefully font 'cmr10' is around
    ss << "set output '" << Filename << "'" << std::endl;
    this->Append_StrDat(&ss);
    this->Append_Footer(&ss);
    ss << "set output" << std::endl;
    ss << "set term pop" << std::endl;

    //Pipe it.
    FILE *fp = nullptr;
    if(!(fp = popen("gnuplot -persist &>/dev/null", "w"))) return false;
    if(!ss.good()) return false;
    fprintf(fp, "%s", ss.str().c_str());
    if(pclose(fp) == -1) return false; //YLOGERR("Unable to close pipe. Is the process still active?");
    return true;
}
std::string Plotter2::Dump_as_String(void) const {  //aka 'plot to string'
    //We now gather all the pieces to produce a single string which is piped to GNUplot.
    std::stringstream ss;
    this->Append_Header(&ss);
    this->Append_StrDat(&ss);
    this->Append_Footer(&ss);
    return ss.str();
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//This is a sort of 'tiling plotter'. It wraps individual (unconnected) plots in the most basic way.
// This class does not (currently) provide a working space like Plotter2 does. Add it if needed.

//Constructor/Destructor.
MultiPlotter2::MultiPlotter2(){
    this->Apply_Basic_Header_Customizations();
    this->Apply_Basic_Footer_Customizations();
}
MultiPlotter2::~MultiPlotter2(){}

//Internal methods. Do not use unless you know what you are doing.
void MultiPlotter2::Apply_Basic_Header_Customizations(void){
    //NOTE: We will set multiplot at plot time.
    //this->header.push_back(R"***(set multiplot layout 4,1 title 'Auto-layout of stacked plots\n')***");
//    this->header.push_back("set tmargin 2");
    this->header.emplace_back("# set term pdfcairo enhanced color solid font 'cmr10,12' size 7.5in,5.5in"); //Helpful comment.
    this->header.emplace_back("# set output ''"); //Helpful comment.
    return;
}
void MultiPlotter2::Apply_Basic_Footer_Customizations(void){
    //NOTE: We will unset multiplot at plot time.
    this->footer.emplace_back("# set output"); //Helpful comment.
    this->footer.emplace_back("# set term pop"); //Helpful comment.
    return;
}
void MultiPlotter2::Append_Header(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");
    for(const auto & s_it : this->header) *ss << s_it << std::endl;
    return;
}
void MultiPlotter2::Append_SubPlots(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");
    for(const auto & subplot : this->subplots){
        *ss << subplot.Dump_as_String() << std::endl;
        *ss << "#" << std::endl; //Easier for poor human to read...
    }
    return;
}
void MultiPlotter2::Append_Footer(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");
    for(const auto & s_it : this->footer) *ss << s_it << std::endl;
    return;
}
void MultiPlotter2::Open_Multiplot_Environ(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");
    //Do to some little quirks in GNUplot, we have to choose both the layout
    // and commit the global title when we open up the multiplot environment.
    //
    //Therefore, this routine should apply some 'smart' heuristics to layout
    // plots in a sane way. Feel free to add/tweak as needed.
    const auto N = static_cast<int64_t>(this->subplots.size());

    //One-row layout.
    //*ss << "set multiplot layout 1, " << N << " title \"" << this->global_title << "\"" << std::endl;

    //One-col layout.
    //*ss << "set multiplot layout " << N << ", 1 title \"" << this->global_title << "\"" << std::endl;

    // ...

    if(N <= 2){
        *ss << "set multiplot layout 1, " << N << " title \"" << this->global_title << "\"" << std::endl;
    }else if(N <= 4){
        *ss << "set multiplot layout 2, " << 2 << " title \"" << this->global_title << "\"" << std::endl;
    }else{
        int64_t M = 0;
        while(2*M < N) ++M; 
        *ss << "set multiplot layout 2, " << M << " title \"" << this->global_title << "\"" << std::endl;
    }

    return;
}
void MultiPlotter2::Close_Multiplot_Environ(std::stringstream *ss) const {
    if(ss == nullptr) YLOGERR("Passed an invalid reference");
    *ss << "unset multiplot" << std::endl;
    return;
}

//Methods.
void MultiPlotter2::Set_Global_Title(const std::string &in){
    this->global_title = in;
    return;
}

//Routines which have minimal internal state.
void MultiPlotter2::Insert_Plotter2(const Plotter2 &in){
    this->subplots.push_back(in);
    return;
}

//Various plotting options.
bool MultiPlotter2::Plot(void) const {
    //We now gather all the pieces to produce a single string which is piped to GNUplot.
    std::stringstream ss;

    //ss << "set term x11" << std::endl;  //This is done to handle multiplot refreshing (wxt cannot do properly).

    this->Open_Multiplot_Environ(&ss);
    this->Append_Header(&ss);
    this->Append_SubPlots(&ss);
    this->Append_Footer(&ss);
    this->Close_Multiplot_Environ(&ss);

    //Pipe it.
    FILE *fp = nullptr;
    if(!(fp = popen("gnuplot -persist &>/dev/null", "w"))) return false;
    if(!ss.good()) return false;
    fprintf(fp, "%s", ss.str().c_str());
    if(pclose(fp) == -1) return false; //YLOGERR("Unable to close pipe. Is the process still active?");
    return true;
}
bool MultiPlotter2::Plot_as_PDF(const std::string &Filename) const { //May overwrite existing files!
    //We now gather all the pieces to produce a single string which is piped to GNUplot.
    std::stringstream ss;

    //NOTE: Here we can use multiplot (probably best for publications and what-not) or
    // we can simply issue multiple plot commands and get several-page pdfs. See
    // http://stackoverflow.com/questions/4334301/how-to-create-a-multi-page-pdf-file-with-gnuplot

    ss << "set term pdfcairo enhanced color solid font 'cmr10,12' size 7.5in,5.5in" << std::endl; //Hopefully font 'cmr10' is around
    ss << "set output '" << Filename << "'" << std::endl;

    this->Open_Multiplot_Environ(&ss);
    this->Append_Header(&ss);
    this->Append_SubPlots(&ss);
    this->Append_Footer(&ss);
    this->Close_Multiplot_Environ(&ss);

    ss << "set output" << std::endl;
    ss << "set term pop" << std::endl;

    //Pipe it.
    FILE *fp = nullptr;
    if(!(fp = popen("gnuplot -persist &>/dev/null", "w"))) return false;
    if(!ss.good()) return false;
    fprintf(fp, "%s", ss.str().c_str());
    if(pclose(fp) == -1) return false; //YLOGERR("Unable to close pipe. Is the process still active?");
    return true;
}
std::string MultiPlotter2::Dump_as_String(void) const {  //aka 'plot to string'
    std::stringstream ss;
    std::string out;

    this->Open_Multiplot_Environ(&ss);
    this->Append_Header(&ss);
    this->Append_SubPlots(&ss);
    this->Append_Footer(&ss);
    this->Close_Multiplot_Environ(&ss);

    if(!ss.good()) return out;
    return ss.str();
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------- Plotter3 - A GNUplot-based class ----------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Plotter3::Set_Current_Line_Title(const std::string &in){
    current_line_title = in;
    return;
}
void Plotter3::Set_Global_Title(const std::string &in){
    global_title = in;
    return;
}
void Plotter3::Set_Current_Line_Use(const std::string &in){   //'lines', 'points', or 'linespoints'
    current_line_use = in;
    return;
}
void Plotter3::Insert(const double &x, const double &y, const double &z){
    ss << x << " " << y << " " << z << R"***(\n)***";
    return;
}
void Plotter3::Next_Line(void){
    ss << YGORPLOT3_H_PIPE_FINISH_SENDING_DATA;
    ss << " w " << current_line_use << " ";
    ss << " lt " << current_line_style << " ";
    ss << " title '" << current_line_title << "' ";

    ss << YGORPLOT3_H_PIPE_NEXT_SERIES;
    ss << YGORPLOT3_H_PIPE_BEGIN_SENDING_DATA;

    //--------------- Iterate current settings for new line -------------------
    ++current_line_style;       //Update line type for next line.
    current_line_title.clear(); //Clear the title.

    return;
}

void Plotter3::Next_Line_Same_Style(void){
    ss << YGORPLOT3_H_PIPE_FINISH_SENDING_DATA;
    ss << " w "  << current_line_use   << " ";
    ss << " lt " << current_line_style << " ";
    ss << " title '" << current_line_title << "' ";

    ss << YGORPLOT3_H_PIPE_NEXT_SERIES;
    ss << YGORPLOT3_H_PIPE_BEGIN_SENDING_DATA;

    //Do not iterate current settings for next line.
    return;
}

bool Plotter3::Plot(void){
    ss << YGORPLOT3_H_PIPE_FINISH_SENDING_DATA;
    ss << " w "  << current_line_use   << " ";
    ss << " lt " << current_line_style << " ";
    ss << " title '" << current_line_title << "' ";

    //----------------------------------
    //Ensure the global settings are applied.
    if(! global_title.empty()){
        const std::string old(ss.str());
        std::string pre(" set title '");  pre += global_title + "' ; ";
        const std::streamsize pos = ss.tellp();
        ss.str(pre + ss.str());
        ss.seekp(pos + pre.length());
        global_title.clear();
    }

    ss << std::endl;
    if((fp != nullptr) && (ss.good())){
        fprintf(fp, "%s", ss.str().c_str());
        //Prepare it again.
        ss.str(""); //.clear();
        ss << YGORPLOT3_H_PIPE_PREP;
        ss << YGORPLOT3_H_PIPE_BEGIN_SENDING_DATA;
        return true;
    }
    //If we could not plot, we simply hold onto the data so that we might be able to plot it later after resolving the pipe issue.
    return false;
}




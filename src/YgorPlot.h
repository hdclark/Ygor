//YgorPlot.h - Simple plotting routines which wrap around Gnuplot, (GNU)plotutils, a postscript plotting library, or good-ol'-fashioned 
// roll-your-own plotting routines.
//
//The idea behind this file is to easily have some plotting facilities within a program without having to deal directly with pipes and
// plotting formats in situ. 


#pragma once
#ifndef YGORPLOT_H_HDR_GUARD
#define YGORPLOT_H_HDR_GUARD

#include <cstdio>  //Used for popen, FILE class.
#include <list>
#include <map>     //Required for plotting maps of samples_1D's.
#include <sstream>
#include <string>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorFilesDirs.h" //Needed for filename checking.
#include "YgorMath.h"  //Needed for helper plotting routines.
#include "YgorMisc.h"  //Used for the macros FUNCINFO, FUNCWARN, FUNCERR.


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------- Plotter - A Plotutils-based class ---------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//This class is a minimal class for plotting. To inject data, you input it directly into a stream.
// Plotutils is nice - in theory, but the support for many things is poor. 
//
//This class should be used for plotting 2D contours on a 2D plane. It works very well for this.
//
//NOTES:
//  FILE *graph_pipe = popen("~/plotutils-2.4.1/graph/graph -a -C -W .004 -g 3 -L Title -X Abscissa -Y Ordinate -T X -", "w");
//  const std::string PLOTTING_INITIATOR = "graph -T X -C -m 1 -q 0.3 ";
//  const std::string PLOTTING_INITIATOR = "graph -T X --bitmap-size \"1000x1000\" -h 0.8 -w 0.8 -l 0.1 -r 0.1 -C -q 0.3  2>/dev/null ";
const std::string PLOTTING_INITIATOR_SCREEN = "graph -T X  --bitmap-size \"1000x1000\" -C ";
const std::string PLOTTING_INITIATOR_FILE   = "graph -T ps --bitmap-size \"1000x1000\" -C ";
const std::string PLOTTING_INITIATOR_FILL   = " -q 0.3 ";     //This option will fill the area under curves. It is ON by default.

const std::string PLOTTING_INITIATOR_BLACK_HOLE = " 2>/dev/null ";

class Plotter {
    private:
        //These are plotutils-specific counters used by this class for various reasons.
        int linestyle_m; //Handles the linestyles so that they can be iterated over by calling a member function.
        int linestyle_s;

    public:
        FILE *fp;
        int status;
        std::stringstream ss;       //Provides a stream-like method of plotting. Passes data onto plotting_data string prior to plotting.
        std::string plotting_data; 

        //Constructor, Destructor.
        //Add more constructors as needed. Do not break this simple plotutils-based constructor, though.
        Plotter() : ss(std::stringstream::in | std::stringstream::out ) {  
            fp = popen( (PLOTTING_INITIATOR_SCREEN + PLOTTING_INITIATOR_FILL + PLOTTING_INITIATOR_BLACK_HOLE).c_str(), "w" );
            linestyle_m = 1;
            linestyle_s = 1;

            if(fp == nullptr) FUNCERR("Unable to open a pipe!");
        }

        Plotter(const std::string &filename) : ss(std::stringstream::in | std::stringstream::out ) {
            if( Does_File_Exist_And_Can_Be_Read(filename) ){
                const std::string safer = Get_Unique_Sequential_Filename( filename );
                FUNCWARN("Attempting to overwrite an existing plot \"" << filename << "\". Proceeding with output filename \"" << safer << "\"");
                fp = popen( (PLOTTING_INITIATOR_FILE + PLOTTING_INITIATOR_FILL + " > " + safer    + PLOTTING_INITIATOR_BLACK_HOLE).c_str(), "w" );
            }else{
                fp = popen( (PLOTTING_INITIATOR_FILE + PLOTTING_INITIATOR_FILL + " > " + filename + PLOTTING_INITIATOR_BLACK_HOLE).c_str(), "w" );
            }
            linestyle_m = 1;
            linestyle_s = 1;

            if(fp == nullptr) FUNCERR("Unable to open a pipe!");
        }

        ~Plotter() {
            if(fp == nullptr){
                status = pclose(fp);
                if(status == -1){
                    FUNCERR("Unable to close pipe. Is the process still active? \"pclose\" reports error " << status);
                }
                //else{
                //    //Use macros described under wait() to inspect `status' in order
                //    // to determine success/failure of command executed by popen()
                // }
            }
        };

        //Methods.
        void Iterate_Linestyle(void);
        //void Title_Current_Line(const std::string &in);   //plotutils, currently, cannot even create a legend - it is on the 'low priority' TO-DO list <eyeroll>...
        bool Plot(void);

        //bool Clear(void);

        // Getters, Setter, etc.. ?

};

//------------------------------------------------------------ Plotter-helpers -----------------------------------------------------------------------------------------

//Containers of Contours of points.
template <class T> void Plot_Container_of_Contour_of_Points( T begin, T end){
    Plotter a_plot;
    a_plot.ss << "# Default, simple plot for container of contours of vec3's: " << std::endl;
    for(auto it = begin; it != end; ++it){
        for(auto iter = it->points.begin(); iter != it->points.end(); iter++){
            a_plot.ss << iter->x << " ";
            a_plot.ss << iter->y << " ";
            //a_plot.ss << iter->z << " ";
            a_plot.ss << std::endl;
        }
        a_plot.Iterate_Linestyle();
    }
    a_plot.Plot();
    return;
}
template void Plot_Container_of_Contour_of_Points( std::list<contour_of_points<double>>::const_iterator begin, std::list<contour_of_points<double>>::const_iterator end );

template <class T> void Plot_Container_of_Contour_of_Points_to_File( T begin, T end, const std::string &in ){
    Plotter a_plot(in);
    a_plot.ss << "# Default, simple plot for container of vec3's: " << std::endl;
    for(auto it = begin; it != end; ++it){
        for(auto iter = it->points.begin(); iter != it->points.end(); iter++){
            a_plot.ss << iter->x << " ";
            a_plot.ss << iter->y << " ";
            //a_plot.ss << iter->z << " ";
            a_plot.ss << std::endl;
        }
        a_plot.Iterate_Linestyle();
    }
    a_plot.Plot();
    return;
}
template void Plot_Container_of_Contour_of_Points_to_File( std::list<contour_of_points<double>>::const_iterator begin, std::list<contour_of_points<double>>::const_iterator end, const std::string &in );


/*
//Containers of Containers of points.                                                                     (Where is this used? Why don't I have a template specialization?)
template <class T> void Plot_Container_of_Container_of_Points( T begin, T end){
    Plotter a_plot;
    a_plot.ss << "# Default, simple plot for container of containers of vec3's: " << std::endl;
    for(auto c_it = begin; c_it != end; ++c_it){
        for(auto it = c_it->begin(); it != c_it->end(); ++it){
            a_plot.ss << it->x << " ";
            a_plot.ss << it->y << " ";
            //a_plot.ss << it->z << " ";
            a_plot.ss << std::endl;
        }
        a_plot.Iterate_Linestyle();
    }
    a_plot.Plot();
    return;
}
*/

//Containers of samples_1D's of points.
template <class T> void Plot_Container_of_Samples_1D( T begin, T end){
    Plotter a_plot;
    a_plot.ss << "# Default, simple plot for container of 1D sequential samples: " << std::endl;
    for(auto it = begin; it != end; ++it){
        for(auto s_it = it->samples.begin(); s_it != it->samples.end(); ++s_it){
            a_plot.ss << (*s_it)[0] << " ";
            a_plot.ss << (*s_it)[2] << " ";
            a_plot.ss << std::endl;
        }
        a_plot.Iterate_Linestyle();
    }
    a_plot.Plot();
    return;
}
template void Plot_Container_of_Samples_1D(std::list<samples_1D<float>>::const_iterator begin, std::list<samples_1D<float>>::const_iterator end);
template void Plot_Container_of_Samples_1D(std::list<samples_1D<double>>::const_iterator begin, std::list<samples_1D<double>>::const_iterator end);

//Map: std::map<K,samples_1D<T>>
template <class T> void Plot_Map_of_Samples_1D( T begin, T end){
    Plotter a_plot;
    a_plot.ss << "# Default, simple plot for container of 1D sequential samples: " << std::endl;
    for(auto it = begin; it != end; ++it){
        //Name is (std::string) it->first.
        for(auto s_it = it->second.samples.begin(); s_it != it->second.samples.end(); ++s_it){
            a_plot.ss << (*s_it)[0] << " ";
            a_plot.ss << (*s_it)[2] << " ";
            a_plot.ss << std::endl;
        }
        a_plot.Iterate_Linestyle();
    }
    a_plot.Plot();
    return;
}
template void Plot_Map_of_Samples_1D(std::map<std::string,samples_1D<float>>::const_iterator begin, std::map<std::string,samples_1D<float>>::const_iterator end);
template void Plot_Map_of_Samples_1D(std::map<std::string,samples_1D<double>>::const_iterator begin, std::map<std::string,samples_1D<double>>::const_iterator end);



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------- Plotter2 - A GNUplot-based class ----------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//This class is a minimal class for interface for GNUplot. It tries to augment the Plotter (Plotutils) class where possible.
// This class is for 2D data - anything which could fit in a samples_1D. Data is not sorted, so it *can* draw contours, but
// they won't be filled. Titles are OK.
//
//This class is a 'data sponge'. It is not written to handle huge amounts of data. It is written to be easy to use. If you
// need to handle lots of data, write it to file first!
//
//Internally, data is stored in samples_1D. When needed (i.e. in Plot()) the data is assembled into GNUplot plotting 
// commands, a pipe is opened, and the script is piped. The data is embedded in the script. The script can be saved and
// called manually with a simple "gnuplot> load '/.../.../savedfile'". This seems like a good way to produce decent 
// later-editable graphs which cannot get separated from their data.
//
//NOTE: [ echo "plot \"< echo -e '4 5\n 3 2'\" w lp pt 2" | gnuplot ] == [ gnuplot> plot "< echo -e '4 5\n 3 2'" w lp pt 2 ].
//
struct Plotter2_Helper {
    samples_1D<double> data; //The 2D data we will be plotting.
    std::string title;       //A title identifying this particular line/data.
    std::string type;        //Point/line type: 'lines', 'points', or 'linespoints'
};
class Plotter2 {
    public:
        std::list<std::string> header, footer;
        std::list<Plotter2_Helper> data;
        Plotter2_Helper working;               //Where we 'cache' data.

        //Constructor, Destructor.
        Plotter2();
        ~Plotter2();

        //Internal methods. Do not use unless you know what you are doing.
        void Apply_Basic_Header_Customizations(void);
        void Apply_Basic_Footer_Customizations(void);
        void Append_Header(std::stringstream *ss) const;
        void Append_StrDat(std::stringstream *ss) const;
        void Append_Footer(std::stringstream *ss) const;

        //Methods.
        void Set_Global_Title(const std::string &in);

        //Routines which use the working buffer and have internal state.
        void Set_Current_Line_Title(const std::string &in);
        void Set_Current_Line_Type(const std::string &in); //lines/points/linespoints/xerrorbars/yerrorbars/xyerrorbars (<--default)
        void Insert(double x, double y);
        void Next_Line(void); 
    
        //Routines which have minimal internal state.
        void Insert_samples_1D(const samples_1D<double> &data, const std::string &title = "", const std::string &linetype = "");
        void Insert_map_of_string_and_samples_1D(const std::map<std::string, samples_1D<double>> &in, const std::string &linetype = "");
        // ... implement more YgorMath class routines here ...

        //Various plotting options.
        bool Plot(void) const;
        bool Plot_as_PDF(const std::string &Filename) const; //May overwrite existing files!
        std::string Dump_as_String(void) const;  //aka 'plot to string'
};

//This is a sort of 'tiling plotter'. It wraps individual (unconnected) plots in the most basic way.
// This class does not (currently) provide a working space like Plotter2 does. Add it if needed.
//
//NOTE: Due to awkward multiplot handling in GNUplot (e.g not refreshable in wxt) it is best to 
// plot to file and open a pdf renderer to view. File overwriting/appending is handled entirely
// by GNUplot, so it may be possible to choose a simple name like "/tmp/Plot.pdf" and simply
// overwrite it repeatedly..
class MultiPlotter2 {
    public:
        std::list<Plotter2> subplots;
        std::list<std::string> header, footer;
        std::string global_title;

        //Constructor, Destructor.
        MultiPlotter2();
        ~MultiPlotter2();

        //Internal methods. Do not use unless you know what you are doing.
        void Apply_Basic_Header_Customizations(void);
        void Apply_Basic_Footer_Customizations(void);
        void Append_Header(std::stringstream *ss) const;
        void Append_SubPlots(std::stringstream *ss) const;
        void Append_Footer(std::stringstream *ss) const;
        void Open_Multiplot_Environ(std::stringstream *ss) const;
        void Close_Multiplot_Environ(std::stringstream *ss) const;


        //Methods.
        void Set_Global_Title(const std::string &in);

        //Routines which have minimal internal state.
        void Insert_Plotter2(const Plotter2 &in);

        //Various plotting options.
        bool Plot(void) const;
        bool Plot_as_PDF(const std::string &Filename) const; //May overwrite existing files!
        std::string Dump_as_String(void) const;  //aka 'plot to string'
};

//------------------------------------------------------------- Plotter2-helpers -----------------------------------------------------------------------------------------

//Map: std::map<K,samples_1D<T>>
template <class T> void Plot2_Map_of_Samples_1D( T begin, T end, const std::string &title){
    Plotter2 a_plot;
    a_plot.Set_Global_Title(title);    

    for(auto it = begin; it != end; ++it){
        a_plot.Set_Current_Line_Title(it->first);

        for(auto s_it = it->second.samples.begin(); s_it != it->second.samples.end(); ++s_it){
            a_plot.Insert((*s_it)[0], (*s_it)[2]);
        }
        a_plot.Next_Line();
    }
    a_plot.Plot();
    return;
}
template void Plot2_Map_of_Samples_1D(std::map<std::string,samples_1D<float>>::const_iterator begin, std::map<std::string,samples_1D<float>>::const_iterator end, const std::string &title);
template void Plot2_Map_of_Samples_1D(std::map<std::string,samples_1D<double>>::const_iterator begin, std::map<std::string,samples_1D<double>>::const_iterator end, const std::string &title);


//Map: std::map<samples_1D<T>,K>
// or  std::vector<std::pair<samples_1D<T>,K>>
template <class T> void Plot3_Map_of_Samples_1D( T begin, T end, const std::string &title){
    Plotter2 a_plot;
    a_plot.Set_Global_Title(title);

    for(auto it = begin; it != end; ++it){
        a_plot.Set_Current_Line_Title(it->second);

        for(auto s_it = it->first.samples.begin(); s_it != it->first.samples.end(); ++s_it){
            a_plot.Insert((*s_it)[0], (*s_it)[2]);
        }
        a_plot.Next_Line();
    }
    a_plot.Plot();
    return;
}
template void Plot3_Map_of_Samples_1D(std::map<samples_1D<float >,std::string>::const_iterator begin, std::map<samples_1D<float >,std::string>::const_iterator end, const std::string &title);
template void Plot3_Map_of_Samples_1D(std::map<samples_1D<double>,std::string>::const_iterator begin, std::map<samples_1D<double>,std::string>::const_iterator end, const std::string &title);

template void Plot3_Map_of_Samples_1D(std::vector<std::pair<samples_1D<float >,std::string>>::const_iterator begin, std::vector<std::pair<samples_1D<float >,std::string>>::const_iterator end, const std::string &title);
template void Plot3_Map_of_Samples_1D(std::vector<std::pair<samples_1D<double>,std::string>>::const_iterator begin, std::vector<std::pair<samples_1D<double>,std::string>>::const_iterator end, const std::string &title);




//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------- Plotter3 - A GNUplot-based class ----------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//This class is a WIP, minimal class interface for GNUplot. It tries to augment the Plotter (Plotutils) class where possible.
// Namely, contours are NOT filled by default, titles/legends are allowed, and a more rigid input scheme is devised to simplify development.
//
//NOTE: This is strictly a R^3 plotter. For R^2 plotting, see Plotter2.
//NOTE: This is setup currently to plot contours floating in space. There are no surfaces (or if they are, they are not 'volume' surfaces!)
//
//NOTES:
//  echo "plot \"< echo -e '4 5\n 3 2'\" w lp pt 2" | gnuplot
//  ==
//  gnuplot> plot "< echo -e '4 5\n 3 2'" w lp pt 2

const std::string YGORPLOT3_H_PLOT_COMMAND("gnuplot -persist "); //&>/dev/null");
const std::string YGORPLOT3_H_PIPE_PREP(R"***(  set xlabel 'x' ; set ylabel 'y' ; set zlabel 'z' ; splot )***" ); // set view equal xyz ; splot )***" );
const std::string YGORPLOT3_H_PIPE_BEGIN_SENDING_DATA(R"***("< echo -e ')***" );                  //"//
const std::string YGORPLOT3_H_PIPE_FINISH_SENDING_DATA(R"***('")***" );          //"//
const std::string YGORPLOT3_H_PIPE_NEXT_SERIES(" , ");

class Plotter3 {
    private:
        FILE *fp;
        std::stringstream ss;

        //Global settings. These do not get flushed.
        std::string global_title;

        //Per-series (i.e. per-line) settings. These get flushed after a call to Next_Line. Call them prior to inserting data.
        std::string current_line_title;  // 'Ygor Data - #6'
        std::string current_line_use;    // 'lines', 'points', or 'linespoints'.
        long int current_line_style;     // Used with Gnuplot's "lt" specifier.
 
    public:
        //Constructor, Destructor.
        //Add more constructors as needed. Do not break this simple plotutils-based constructor, though.
        Plotter3() : ss(std::stringstream::in | std::stringstream::out){
            if(!(fp = popen(YGORPLOT3_H_PLOT_COMMAND.c_str(), "w"))) FUNCERR("Unable to open a pipe!");
            current_line_use   = "lp";
            current_line_style = 1;
            ss << YGORPLOT3_H_PIPE_PREP;
            ss << YGORPLOT3_H_PIPE_BEGIN_SENDING_DATA;
        }
        ~Plotter3(){
            if(fp != nullptr){
                if(pclose(fp) == -1) FUNCERR("Unable to close pipe. Is the process still active?");
            }
        };

        //Methods.
        void Set_Global_Title(const std::string &in);
        void Set_Current_Line_Title(const std::string &in);
        void Set_Current_Line_Use(const std::string &in);
        
        void Insert(const double &x, const double &y, const double &z);
        void Next_Line(void); 
        void Next_Line_Same_Style(void);
        
        bool Plot(void);
};

//------------------------------------------------------------- Plotter3-helpers -----------------------------------------------------------------------------------------

//List: std::list<contour_of_points<T>>
template <class T> void Plot3_List_of_contour_of_points(T begin, T end, const std::string &title){
    Plotter3 a_plot;
    a_plot.Set_Global_Title(title);

    for(auto c_it = begin; c_it != end; ++c_it){
        for(auto p_it = c_it->points.begin(); p_it != c_it->points.end(); ++p_it){
            a_plot.Insert(p_it->x, p_it->y, p_it->z);
        }
        if(c_it->closed){
            auto p_it = c_it->points.begin();
            a_plot.Insert(p_it->x, p_it->y, p_it->z);
        }
        a_plot.Next_Line_Same_Style();
    }
    a_plot.Plot();
    return;
}
template void Plot3_List_of_contour_of_points(std::list<contour_of_points<float >>::const_iterator begin, std::list<contour_of_points<float >>::const_iterator end, const std::string &title);
template void Plot3_List_of_contour_of_points(std::list<contour_of_points<double>>::const_iterator begin, std::list<contour_of_points<double>>::const_iterator end, const std::string &title);


//Each contour_collection represents a 3D structure. We plot all 3D structures in the list. 
//
//List: std:list<contour_collection<T>>
template <class T> void Plot3_List_of_contour_collections(T begin, T end, const std::string &title){
    Plotter3 a_plot;
    a_plot.Set_Global_Title(title);

    for(auto cc_it = begin; cc_it != end; ++cc_it){
        a_plot.Set_Current_Line_Use("lines"); //Avoid showing points because they quickly obscure the lines.
        for(auto c_it = cc_it->contours.begin(); c_it != cc_it->contours.end(); ++c_it){
            for(auto p_it = c_it->points.begin(); p_it != c_it->points.end(); ++p_it){
                a_plot.Insert(p_it->x, p_it->y, p_it->z);
            }
            if(c_it->closed){
                auto p_it = c_it->points.begin();
                a_plot.Insert(p_it->x, p_it->y, p_it->z);
            }
            if(c_it != --(cc_it->contours.end())){
                a_plot.Next_Line_Same_Style(); //For a given 3D structure, plot all contours with same linetype.
            }
        }
        a_plot.Next_Line();
    }
    a_plot.Plot();
    return;
}
template void Plot3_List_of_contour_collections(std::list<contour_collection<float >>::const_iterator begin, std::list<contour_collection<float >>::const_iterator end, const std::string &title);
template void Plot3_List_of_contour_collections(std::list<contour_collection<double>>::const_iterator begin, std::list<contour_collection<double>>::const_iterator end, const std::string &title);






#endif

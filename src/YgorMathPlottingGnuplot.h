//YgorMathPlottingGnuplot.h
#pragma once
#ifndef YGOR_MATH_PLOTTING_GNUPLOT_HDR_GRD_H
#define YGOR_MATH_PLOTTING_GNUPLOT_HDR_GRD_H

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <utility>
#include <sstream>
#include <thread>
#include <chrono>

// No longer needed; went with std::thread instead.
//#include <cstdio>  //For popen, class FILE.
//#include <unistd.h> //For fork.
#include <csignal>

#include "YgorDefinitions.h"
#include "YgorMath.h"

namespace YgorMathPlottingGnuplot {

//This simple class provides a way to block on a long-running thread that holds a pipe.
// Using this, the main process will block (in the destructor) waiting to join threads.
struct Waiter {
    std::thread t;

    Waiter(std::thread in) : t(std::move(in)) {};
    Waiter(Waiter &&) = default;
    ~Waiter(){
#if !defined(_WIN32) && !defined(_WIN64)
        //At this point we want probably want SIGPIPEs to cause termination.
        // Otherwise we may wait around forever for nothing.
        signal(SIGPIPE, SIG_DFL);
#endif

        //YLOGINFO("Called ~Waiter(). Waiting briefly before attempting to join");
        //std::this_thread::sleep_for(std::chrono::seconds(1));

        //Do NOT attempt to do anything here except wait on threads. It is possible that
        // everything else will already be decontructed and will result in failures!
        if(t.joinable()) t.join();
    };
};

//This is the actual object which will store the Waiter class instances. It only needs to
// (1) be thread safe to read and write -- could be atomic, wrapped with mutex, etc.., 
// (2) have it's destructor called whenever the main process exits, and
// (3) delegate to the Waiter class deconstructors.
//
// TODO: Use something atomic here instead of thread_local. Using thread_local accomplishes
//       goals, but is heavyweight if there are lots of threads around. We can't tell how
//       many are around (this is supposed to be library code), so it is quite wasteful.
static thread_local std::list<Waiter> ThreadWaiter;



//This is a simple shuttle class which can be used for a variety of data types. For example, 
// the template can be a samples_1D<T>, a contour_with_points<R>, or some STL container.
template <class C>
struct Shuttle {
    //C const &Data;
    C Data;
    std::string Title;
    std::vector<std::pair<std::string,std::string>> UsingWith;

    //Constructor with some reasonable defaults.
    Shuttle(const C &data, 
            const std::string &title = "", 
            const std::vector<std::pair<std::string,std::string>> &usingwith = 
                 { { "1:2",     "lp"          },
                   { "1:2:3:4", "xyerrorbars" } }
           ) : Data(data), Title(title), UsingWith(usingwith) { };
};



//This function plots data in a samples_1D using Gnuplot. All data are casted to doubles en route.
// Many details can be controlled by the caller. BE AWARE this routine does not sanitize input, so
// it is unsafe to pass user data directly to this routine.
//
//
// NOTE: While the layout of samples_1D is [x,dx,y,dy], we plot in a style Gnuplot wants and is 
//       more convenient in this context: [x,y,dx,dy]. This format makes it easier to ignore
//       uncertainties.
//
// NOTE: Remember, the Gnuplot 'using' string is formatted like "1:2:3:4" and can be modified
//       arbitrarily like "($1):($2*1.0):($3/1.0):($1+$2+$3+$4)".
//
// NOTE: Since Gnuplot is not very flexible accepting piped binary data, the exact way in which the
//       data is passed to Gnuplot is not specified. It might use a temporary file. It might send
//       data through the pipe several times. Whatever the case, keep in mind that several copies
//       may be necessary, which could be costly in terms of memory.
//
//       When supplying several using-with statements for a single set of data, only the first 
//       gets a title. So, if you want to be able to completely hide the data in the interactive
//       plot (by clicking the title), you'll need to send each plotting style as a completely
//       separate data set.
//
// NOTE: User-supplied options are supplied to Gnuplot immediately before plotting, after all
//       other options have been sent. They do NOT need to end with a newline -- one is provided.
//
template <typename T> 
inline
void Plot(const std::vector<Shuttle<samples_1D<T>>> &Shuttles,
          const std::string &GlobalTitle = "",
          const std::string &XLabel = "",      // AKA the "abscissa."
          const std::string &YLabel = "",      // AKA the "ordinate."
          const std::vector<std::string> &UserOpts = { } ){ // Generic user options.

    const bool BlockOnPlottingWindow = false;
 
    //Gnuplot is very picky about empty data sets and plotting nothing. A window might not be created
    // if such things are passed to Gnuplot. Try to filter them out.
    auto contains_plottable_data = [](const Shuttle<samples_1D<T>> &s) -> bool {
                                      //If there is no data at all.
                                      if(s.Data.size() == 0) return false;

                                      //If there are no finite (plottable) data.
                                      for(const auto &d : s.Data.samples) if(std::isfinite(d[0]) && std::isfinite(d[2])) return true;
                                      return false;
                                  };
    {
        bool PlottableDataPresent = false;
        for(const auto &shtl : Shuttles){
            if(contains_plottable_data(shtl)){
                PlottableDataPresent = true;
                break;
            }
        }
        if(!PlottableDataPresent){
            throw std::invalid_argument("No plottable data. Cannot plot!");
        }
    }
#if !defined(_WIN32) && !defined(_WIN64)
    signal(SIGPIPE, SIG_IGN);
#endif

    //FILE *fp = popen("gnuplot --persist ", "w");
    FILE *fp = popen("gnuplot", "w");
    if(fp == nullptr) throw std::runtime_error("Unable to open a pipe to gnuplot");

    fprintf(fp, "reset\n");
    fflush(fp);

    //fprintf(fp, "set size 1.0,1.0\n"); //Width, Height, clamped to [0,1].
    //fprintf(fp, "set autoscale fix\n");
    //fprintf(fp, "set palette defined (0 'black', 1 'white')\n");
    //fprintf(fp, "set palette grey\n");
    //fprintf(fp, "set tics scale 0\n");
    //fprintf(fp, "unset cbtics\n");
    //fprintf(fp, "unset key\n");

    fprintf(fp, "plot sin(x) \n"); // Try to get a window open ASAP.
    fflush(fp);
    std::this_thread::sleep_for(std::chrono::seconds(1));
 
    { 
      const std::string linewidth("1.1"), pointsize("0.4");
      const std::string pt_circ_solid("5"), pt_box_solid("9"), pt_tri_solid("13"), pt_dimnd_solid("11"), pt_invtri_solid("7");
      int N(0);

      fprintf(fp, "set pointsize %s \n", pointsize.c_str());
      fprintf(fp, "set linetype %d lc rgb '#000000' lw %s pt %s \n", ++N, linewidth.c_str(), pt_circ_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#1D4599' lw %s pt %s \n", ++N, linewidth.c_str(), pt_circ_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#11AD34' lw %s pt %s \n", ++N, linewidth.c_str(), pt_circ_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#E62B17' lw %s pt %s \n", ++N, linewidth.c_str(), pt_circ_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#E69F17' lw %s pt %s \n", ++N, linewidth.c_str(), pt_box_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#2F3F60' lw %s pt %s \n", ++N, linewidth.c_str(), pt_box_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#2F6C3D' lw %s pt %s \n", ++N, linewidth.c_str(), pt_box_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#8F463F' lw %s pt %s \n", ++N, linewidth.c_str(), pt_box_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#8F743F' lw %s pt %s \n", ++N, linewidth.c_str(), pt_dimnd_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#031A49' lw %s pt %s \n", ++N, linewidth.c_str(), pt_dimnd_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#025214' lw %s pt %s \n", ++N, linewidth.c_str(), pt_dimnd_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#6D0D03' lw %s pt %s \n", ++N, linewidth.c_str(), pt_dimnd_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#6D4903' lw %s pt %s \n", ++N, linewidth.c_str(), pt_tri_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#224499' lw %s pt %s \n", ++N, linewidth.c_str(), pt_tri_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#00FF00' lw %s pt %s \n", ++N, linewidth.c_str(), pt_tri_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#00BBBB' lw %s pt %s \n", ++N, linewidth.c_str(), pt_invtri_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#000000' lw %s pt %s \n", ++N, linewidth.c_str(), pt_invtri_solid.c_str());
      fprintf(fp, "set linetype %d lc rgb '#000000' lw %s pt %s \n", ++N, linewidth.c_str(), pt_invtri_solid.c_str());
      fprintf(fp, "set linetype cycle %d \n", N); // Now all higher lt := (lt % N).
      fflush(fp);

      //fprintf(fp, "set style increment user \n");  //<-- deprecated!
    }

    fprintf(fp, "set grid lc rgb '#000000' \n");
    fprintf(fp, "set xlabel '%s' \n", XLabel.c_str());
    fprintf(fp, "set ylabel '%s' \n", YLabel.c_str());
    fprintf(fp, "set zero %f \n", std::numeric_limits<float>::denorm_min() );
    //fprintf(fp, "set  \n");
    fflush(fp);

    fprintf(fp, "set title '%s' \n", GlobalTitle.c_str());
    //Could specify the location of the samples_1D we are plotting if wanted:
    // Object with address %s\n', std::tostring(static_cast<intmax_t>((size_t)(std::addressof(*this)))).c_str());
    fflush(fp);

    for(const auto &UserOpt : UserOpts){
        fprintf(fp, "%s \n", UserOpt.c_str());
    }
    fflush(fp);

    //Tell Gnuplot how much to expect and what to do with the coming binary data.
    std::stringstream ss;
    ss << " plot 1/0 title '' ";

    {
        int lt = 0;
        for(const auto &shtl : Shuttles){
            if(!contains_plottable_data(shtl)) continue;

            bool FirstUsingWith = true;
            ++lt;
            const std::string Title = shtl.Title;
            for(const auto &uw : shtl.UsingWith){
                const std::string Using = uw.first;
                const std::string With  = uw.second;
                ss << " , '-' binary record=" << shtl.Data.size() 
                   << R"***( format='%4double' )***"
                   << " using " << Using 
                   << " with " << With
                   << " lt " << lt
                   << " title '" << (FirstUsingWith ? Title : "") << "' ";
                FirstUsingWith = false;
            }
        }
    }
    fprintf(fp, "%s\n", ss.str().c_str()); 
    fflush(fp);


    //Pass Gnuplot the binary data.
    //
    // NOTE: Gnuplot unfortunately requires users to pass binary data across the pipe for each
    //       plot. Sadly, one cannot ask for, say, xyerrorbars and linespoints in the same plot,
    //       so we have to send a fresh copy of the data (in binary) for each specified using-
    //       with pair.
    //
    //       Hopefully someday Gnuplot will honor the [plot '-' ..., '' ...] construct (i.e., 
    //       the "use the same data as the most recently previously-specified source" for the
    //       '-' special file. Currently we are forced to send it again.
    //
    //       Alternatively, hopefully Gnuplot will eventually permit (large, binary) "named 
    //       data" aka "datablocks". Then we could simply do something like:
    //           > $DATA001 binary record=(X,Y) << 
    //           > ... (send the data) ...
    //           > 
    //           > plot $DATA001 using ... with ... , 
    //           >      $DATA001 using ... with ... ,
    //           >      $DATA002 using ... with ... ,
    //           >      ''       using ... with ... 
    //           >
    //           > replot $DATA003 using ... with ... 
    //
    //       which is currently not possible.
    //
    // NOTE: Gnuplot assumes the input data has one of the following layouts:
    //         (x, y, ydelta),
    //         (x, y, ylow, yhigh),
    //         (x, y, xdelta),
    //         (x, y, xlow, xhigh),
    //         (x, y, xdelta, ydelta), or    // <----- This is what we're using.
    //         (x, y, xlow, xhigh, ylow, yhigh).
    //
    //       But samples_1D are laid out like
    //         (x, xdelta, y, ydelta).
    // 
    //       So we need to 'visit' columns in order to emulate the Gnuplot layout.
    //       
    const std::vector<size_t> Element_Order = {0, 2, 1, 3};
    for(const auto &shtl : Shuttles){
        if(!contains_plottable_data(shtl)) continue;

        for(const auto &uw : shtl.UsingWith){
            for(const auto & data_arr : shtl.Data.samples){
                for(const auto & element : Element_Order){
                    auto val = static_cast<double>(data_arr[element]);
                    const size_t res = fwrite(reinterpret_cast<void *>(&val), sizeof(double), 1, fp);
                    if(res != 1) throw std::runtime_error("Unable to write to pipe");
                }
                fflush(fp);
            }
        }
    }
    fflush(fp);

    if(!BlockOnPlottingWindow){
        std::this_thread::sleep_for(std::chrono::seconds(1));        
        //fprintf(fp, " pause 1 \n");
        fflush(fp);
        fprintf(fp, " refresh \n");
        fflush(fp);
        fprintf(fp, " pause mouse close \n"); //Wait for the user to close the window.
        fflush(fp);
        fprintf(fp, " exit gnuplot \n");
        fflush(fp);
    }

    //Using fork() to wait on the Gnuplot pipe. This requires signal handling, child reaping, 
    // and (more) platform-specific code. Not ideal. On the plus side, it works well and 
    // could be a good fallback.
    //
    //signal(SIGCHLD, SIG_IGN); //Let init reap our children.
    //
    //auto thepid = fork();
    //if((thepid == 0) || (thepid == -1)){
    //    //Close stdin and stdout.
    //    ... iff needed ...
    //    while(true){
    //        std::this_thread::yield();
    //        std::this_thread::sleep_for(std::chrono::seconds(1));        
    //        if(0 > fprintf(fp, "\n")) break; // Keep trying to write until it fails.
    //        fflush(fp);
    //    }
    //    pclose(fp);
    //    exit(0);
    //}


    //Now, in order to keep the Gnuplot window active (responsive), the Gnuplot main process must 
    // continue as long as the user is interested. Instead of blocking our main process, we can 
    // fork and let the child take ownership, or use threads. Going with the latter! 
    //
    // NOTE: Do NOT attempt to access anything other than what you have value captured or thread-
    //       local items. This 'child' thread may run beyond the main thread, so it is possible 
    //       that everything else will be deconstructed while this runs!
    //
    // NOTE: I can't remember right now how to force parallel thread execution. Maybe std::async?
    //       If it is an issue, fix it. It shouldn't really be an issue because the thread just
    //       waits around to close the pipe, but Gnuplot will close the other end at some point.
    //
    // NOTE: I've found that if there are several items in the ThreadWaiter, the main process will
    //       terminate after any one of the threads have joined (actually I think all threads too,
    //       maybe with something like a quick_exit), but the gnuplot process will get reparented
    //       to init. This behaviour is a bit strange, because one would expect all destructors
    //       to be waited on. Well, it seems reasonable and maybe even useful -- if init kills all
    //       children ASAP then closing one window will close all others after the main process
    //       has finished. This would actually be ideal, I think. Still, be careful!
    // 
    auto Wait_and_Close_Pipe = [=](void) -> void {
        while(true){
            std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::seconds(1));        
            if(0 > fprintf(fp, "\n")) break; // Keep trying to write until it fails.
            fflush(fp);
        }
        pclose(fp);
        return;
    };

    if(BlockOnPlottingWindow){
        Wait_and_Close_Pipe();
    }else{
        ThreadWaiter.emplace_back(std::move(std::thread(Wait_and_Close_Pipe)));
    }
    return;
}

} // namespace YgorMathPlottingGnuplot.

#endif

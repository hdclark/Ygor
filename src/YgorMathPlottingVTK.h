//YgorMathPlottingVTK.h
//
// This file contains header-only plotting routines using VTK. You will need to link VTK
// libraries if you include this file. Here is a minimal CMakeLists.txt file:
//
//    |  cmake_minimum_required(VERSION 2.8)
//    |  PROJECT(YourProject)
//    |  # ...
//    |  find_package(VTK REQUIRED)
//    |  include(${VTK_USE_FILE})
//    |  # ...
//    |  add_executable(yourexecutable MACOSX_BUNDLE TheExecutableFilename.cc)
//    |  if(VTK_LIBRARIES)
//    |    target_link_libraries(yourexecutable ${VTK_LIBRARIES})
//    |  else()
//    |    target_link_libraries(yourexecutable vtkHybrid vtkWidgets)
//    |  endif()
//    |  # ...
//

#pragma once
#ifndef YGOR_MATH_PLOTTING_VTK_HDR_GRD_H
#define YGOR_MATH_PLOTTING_VTK_HDR_GRD_H

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <utility>
#include <sstream>
#include <thread>
#include <chrono>

#include <vtkVersion.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkChartXY.h>
#include <vtkTable.h>
#include <vtkPlot.h>
#include <vtkPlotPoints.h>
//#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkPen.h>
#include <vtkAxis.h>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorString.h"

namespace YgorMathPlottingVTK {

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

        //FUNCINFO("Called ~Waiter(). Waiting briefly before attempting to join");
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
thread_local std::list<Waiter> ThreadWaiter;

enum Method {
     // Line types.
     SOLID_LINE,
     DASH_LINE,
     DOT_LINE,
     DASH_DOT_LINE,
     DASH_DOT_DOT_LINE,

     //Point types.
     CIRCLE,
     SQUARE,
     DIAMOND,
     PLUS,
     CROSS
};

//This is a simple shuttle class which can be used for a variety of data types. For example, 
// the template can be a samples_1D<T>, a contour_with_points<R>, or some STL container.
template <class C>
struct Shuttle {

    C const &Data;
    //C Data;
    std::string Title;
    std::vector<Method> As;

    //Constructor with some reasonable defaults.
    Shuttle(const C &data, 
            const std::string &title = "", 
            const std::vector<Method> &as = { SOLID_LINE, CIRCLE }
           ) : Data(data), Title(title), As(as) { };
};



//This function plots data in a samples_1D using VTK. All data are casted to doubles en route.
// Many details can be controlled by the caller. BE AWARE this routine does not sanitize input, so
// it is unsafe to pass user data directly to this routine.
//
//
template <typename T> 
inline
void Plot(const std::vector<Shuttle<samples_1D<T>>> &Shuttles,
          const std::string &GlobalTitle = "",
          const std::string &XLabel = "",      // AKA the "abscissa."
          const std::string &YLabel = ""){     // AKA the "ordinate."

    const bool BlockOnPlottingWindow = false;

    //Ensure there is something plottable. This check may be insufficient if some exotic plotting 
    // format was requested.
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

    vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
    view->GetRenderer()->SetBackground(1.0,1.0,1.0);

    vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
    view->GetScene()->AddItem(chart);

    //This table holds point data.
    std::vector<vtkSmartPointer<vtkTable>> tables;

    const auto shtl_count = Shuttles.size();
    auto shtl_iter = 0;
 
    for(const auto &shtl : Shuttles){
        if(!contains_plottable_data(shtl)) continue;
        const auto datum_count = shtl.Data.size();

        tables.push_back(vtkSmartPointer<vtkTable>::New());

        //Create two columns for each shuttle passed in (x and y). Do not yet fill with data.
        vtkSmartPointer<vtkDoubleArray> arr1 = vtkSmartPointer<vtkDoubleArray>::New();
        arr1->SetName(XLabel.c_str());
        //arr1->SetName(Generate_Random_String_of_Length(10).c_str());
        tables.back()->AddColumn(arr1);

        vtkSmartPointer<vtkDoubleArray> arr2 = vtkSmartPointer<vtkDoubleArray>::New();
        arr2->SetName(shtl.Title.c_str());
        //arr2->SetName(Generate_Random_String_of_Length(10).c_str());
        tables.back()->AddColumn(arr2);

        //Fill in the data.
        tables.back()->SetNumberOfRows(datum_count);
        int i = 0;
        for(const auto &data_arr : shtl.Data.samples){
            tables.back()->SetValue(i, 0, static_cast<double>(data_arr[0]));
            tables.back()->SetValue(i, 1, static_cast<double>(data_arr[2]));
            ++i;
        }

        //Create the requested plot types (e.g., lines; points; both; etc.).
        for(const auto &as : shtl.As){

            vtkPlot *theplot;
            if( (as == Method::SOLID_LINE)
            ||  (as == Method::DASH_LINE)
            ||  (as == Method::DOT_LINE)
            ||  (as == Method::DASH_DOT_LINE)
            ||  (as == Method::DASH_DOT_DOT_LINE) ){
                theplot = chart->AddPlot(vtkChart::LINE);
            }else if( (as == Method::CIRCLE)
                  ||  (as == Method::SQUARE)
                  ||  (as == Method::DIAMOND)
                  ||  (as == Method::PLUS)
                  ||  (as == Method::CROSS) ){
                theplot = chart->AddPlot(vtkChart::POINTS);
            }else{
                throw std::invalid_argument("Plotting style (line, points, etc.) not recognized.");
            }
            theplot->SetInputData(tables.back(), 0, 1);

            const auto clamped_r = std::sqrt(static_cast<double>(shtl_iter)/static_cast<double>(shtl_count));
            const auto clamped_g = 0.5*std::pow(static_cast<double>(shtl_iter)/static_cast<double>(shtl_count),3.0);
            const auto clamped_b = 0.0;
            theplot->SetColor(clamped_r,clamped_g,clamped_b);
            theplot->SetWidth(2.0);
   
   
            if(as == Method::SOLID_LINE){
                theplot->GetPen()->SetLineType(vtkPen::SOLID_LINE);
            }else if(as == Method::DASH_LINE){
                theplot->GetPen()->SetLineType(vtkPen::DASH_LINE);
            }else if(as == Method::DOT_LINE){
                theplot->GetPen()->SetLineType(vtkPen::DOT_LINE);
            }else if(as == Method::DASH_DOT_LINE){
                theplot->GetPen()->SetLineType(vtkPen::DASH_DOT_LINE);
            }else if(as == Method::DASH_DOT_DOT_LINE){
                theplot->GetPen()->SetLineType(vtkPen::DASH_DOT_DOT_LINE);
   
            }else if(as == Method::CIRCLE){
                vtkPlotPoints::SafeDownCast(theplot)->SetMarkerStyle(vtkPlotPoints::CIRCLE);
            }else if(as == Method::SQUARE){
                vtkPlotPoints::SafeDownCast(theplot)->SetMarkerStyle(vtkPlotPoints::SQUARE);
            }else if(as == Method::DIAMOND){
                vtkPlotPoints::SafeDownCast(theplot)->SetMarkerStyle(vtkPlotPoints::DIAMOND);
            }else if(as == Method::PLUS){
                vtkPlotPoints::SafeDownCast(theplot)->SetMarkerStyle(vtkPlotPoints::PLUS);
            }else if(as == Method::CROSS){
                vtkPlotPoints::SafeDownCast(theplot)->SetMarkerStyle(vtkPlotPoints::CROSS);
            }


            //chart->RecalculateBounds();
        }
        ++shtl_iter;
    }

    //Final tweaks.
    //view->GetRenderWindow()->SetMultiSamples(0);
    chart->SetShowLegend(true);
    chart->SetTitle(GlobalTitle.c_str());
    chart->GetAxis(1)->SetTitle(XLabel.c_str());
    chart->GetAxis(0)->SetTitle(YLabel.c_str());

 
    //Use fork() to wait on the blocked viewer. This requires signal handling, child reaping, 
    // and (more) platform-specific code. Not ideal. On the plus side, it works well and 
    // could be a good fallback.
    //
    //signal(SIGCHLD, SIG_IGN); //Let init reap our children.
    //
    //auto thepid = fork();
    //if((thepid == 0) || (thepid == -1)){
    //    view->GetInteractor()->Initialize();
    //    view->GetInteractor()->Start();
    //    exit(EXIT_SUCCESS);
    //}

    auto Wait_On_Viewer = [=](void) -> void {
        //Launch the viewer.
        view->GetInteractor()->Initialize();
        view->GetInteractor()->Start(); //Blocks.
        return;
    };
    if(BlockOnPlottingWindow){
        Wait_On_Viewer(); //Blocks.
    }else{
        ThreadWaiter.emplace_back(std::move(std::thread(Wait_On_Viewer)));
    }
    return;
}

} // namespace YgorMathPlottingVTK.

#endif

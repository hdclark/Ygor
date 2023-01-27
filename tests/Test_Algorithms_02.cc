//Non-parametric regression techniques (ie. smoothing a 1D function to some optimally-defined smoothness).

#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMath_Samples.h"
#include "YgorString.h"
#include "YgorAlgorithms.h"
#include "YgorPlot.h"



int main(int argc, char **argv){
//    const auto data = samples_1D_sample_CMB_first_half();
    const auto data = samples_1D_sample_CMB();
//    const auto data = samples_1D_sample_LIDAR();
//    const auto data = samples_1D_sample_Calories();

    bool OK;

    //Comparison of smoothed functions compared with original data.
    if(true){
        std::map<std::string, samples_1D<double>> plot_shtl;
        plot_shtl["Original"] = data;

        for(auto h = 1.0; h < 10.0; h += 0.5){
            const auto smoothed = NPRLL::Get_Smoothed_Evenly_Spaced(h, 1.0, data, &OK); 
            if(OK) plot_shtl["h = "_s + Xtostring(h)] = smoothed;
        }

        Plot2_Map_of_Samples_1D(plot_shtl.begin(), plot_shtl.end(), "Different smoothing parameters");
    }

    //Plot of specific smoothed function compared with original data.
    if(true){
        std::list<double> hs({1.0, 10.0, 25.0, 37.0, 50.0, 100.0, 126.9, 150.0, 200.0, });
        std::map<std::string, samples_1D<double>> plot_shtl;
        plot_shtl["Original"] = data;

        for(auto hs_it = hs.begin(); hs_it != hs.end(); ++hs_it){
            const auto smoothed = NPRLL::Get_Smoothed_Evenly_Spaced(*hs_it, 2.0, data, &OK); 
            if(OK) plot_shtl["h = "_s + Xtostring(*hs_it)] = smoothed;
        }

        Plot2_Map_of_Samples_1D(plot_shtl.begin(), plot_shtl.end(), "Different smoothing parameters");
    }


    //Examine the cross-validation score over a range of h's.
    if(true){
        std::map<std::string, samples_1D<double>> plot_shtl;
        {
          samples_1D<double> cvs;
          for(double h = 5.0; h < 500.0; h += 2.5){
              const double cv = NPRLL::Get_Cross_Validation_Leave_One_Out(h,data);
              if(cv > 1E50){ cvs.push_back(vec2<double>(h,-50.0));
              }else{         cvs.push_back(vec2<double>(h,cv));
              }
          }
          plot_shtl["Leave-one-out CV"] = cvs;
        }
        {
          samples_1D<double> cvs;
          for(double h = 5.0; h < 500.0; h += 2.5){
              const double cv = NPRLL::Get_Cross_Validation_Generalized_CV(h,data);
              if(cv > 1E50){ cvs.push_back(vec2<double>(h,-50.0));
              }else{         cvs.push_back(vec2<double>(h,cv));
              }
          }
          plot_shtl["Generalized CV"] = cvs;
        } 
        Plot2_Map_of_Samples_1D(plot_shtl.begin(), plot_shtl.end(), "Cross-validation vs h");
    }


    //Attempt to determine an optimal smoothing parameter.
    if(true){
        const double h_0 = 100.0;
        const double h_scale = 5.0;
        const double h_optimal = NPRLL::Find_Optimal_H(h_0, h_scale, 1E-7, 5000, data, true, &OK);
        if(OK == false){  YLOGWARN("Unable to determine an optimal h. Value returned was " << h_optimal);
        }else{            YLOGINFO("Found optimal h = " << h_optimal);
        }
    }

    //Compute the 'c' value for the given h.
    if(true){
        const double c = NPRLL::Find_C(37.5, 95.0, data, &OK);
        if(OK == false){  YLOGWARN("Unable to determine the c value");
        }else{            YLOGINFO("Found c = " << c);
        }
    }


    //Perform a full-featured analysis. Starting with raw data, perform non-parametric regression, compute some
    // 95% confidence intervals, and plot: the data, the smoothed data, and the confidence bands.
    if(true){
        std::map<std::string, samples_1D<double>> plot_shtl;
        const auto xmin = data.samples.front()[0];
        const auto xmax = data.samples.back()[0];
        const auto dx   = 2.0;
        //---------------------------------------
        //Original data.
        plot_shtl["Original"] = data;

        //---------------------------------------
        //Smoothed function approximation (quantized on each x_i).
        const double h_0       = 100.0;
        const double h_scale   = 5.0;
        const double h_optimal = NPRLL::Find_Optimal_H(h_0, h_scale, 1E-7, 5000, data, true, &OK);
        if(OK == false) YLOGERR("Encountered error computing smoothed function");

        {
          samples_1D<double> plot;
          for(auto x = xmin; x < xmax; x += dx){
              const double fval = NPRLL::Get_Smoothed_at_X(h_optimal, x, data, &OK);
              if(OK == false) continue;

              plot.push_back(vec2<double>(x,fval));
          } 
          plot_shtl["Smoothed"] = plot;
        }

        //--------------------------------------
        //Compute necessary info to evaluate the confidence bands (not quite of the smooth function approximation, but close!).
        const auto log_mse_est = NPRLL::Log_of_MSE(h_optimal, data, &OK);
        if(OK == false) YLOGERR("Encountered error computing log of MSE estimate data");

        const double conf_h_0       = 100.0;
        const double conf_h_scale   = 5.0;
        const double conf_h_optimal = NPRLL::Find_Optimal_H(conf_h_0, conf_h_scale, 1E-7, 5000, log_mse_est, true, &OK);
        if(OK == false) YLOGERR("Encountered error computing smoothed function");

        const double c = NPRLL::Find_C(h_optimal, 95.0, data, &OK);
        {
          samples_1D<double> plot_l, plot_u;
          for(auto x = xmin; x < xmax; x += dx){
              const double fval = NPRLL::Get_Smoothed_at_X(h_optimal, x, data, &OK);
              if(OK == false) continue;
              const double band = NPRLL::Get_Conf_at_X(h_optimal, conf_h_optimal, c, x, log_mse_est, data, &OK);
              if(OK == false) continue;

              plot_l.push_back(vec2<double>(x,fval + band));
              plot_u.push_back(vec2<double>(x,fval - band));
          }
          plot_shtl["Lower band - 95%"] = plot_l;
          plot_shtl["Upper band - 95%"] = plot_u;
        }

        Plot2_Map_of_Samples_1D(plot_shtl.begin(), plot_shtl.end(), "Optimally-smoothed data with confidence bands");
    }

    return 0;
}

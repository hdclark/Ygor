//YgorAlgorithms.cc.
#include <cmath>
#include <iostream>
#include <vector>
#include <functional>
#include <list>
#include <limits>
#include <numeric>   //For std::accumulate(...).
#include <map>
#include <string>
#include <tuple>
#include <random>
#include <iomanip> //For the MD5 conversion from uint128_t to std::string.

#include "External/SpookyHash/SpookyV2.h"
//#include "External/SpookyHash/SpookyV2.cpp"
#include "External/MD5/md5.h"

#include "YgorAlgorithms.h"
#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorStats.h"
#include "YgorString.h"
#include "YgorPlot.h"
#include "YgorTime.h"


//----------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------- NMSimplex ----------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
// Nothing here right now - header-only.

//----------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------- For_Each_In_Parallel -----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
// Nothing here right now - header-only.

//----------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------- Nonparametric Regression ----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------


namespace NPRLL { //NPRLL - Non-Parametric Regression: Local Linear Smoothing.

    static inline double l_vector_kernel(double x){
        //This can be chosen by the user. See http://en.wikipedia.org/wiki/Kernel_(statistics) for info.
    
        //Tricube:
        const double xabs(YGORABS(x));
        if(xabs >= 1.0) return 0.0;
        return (70.0/81.0)*std::pow(1.0 - xabs*xabs*xabs, 3.0);
    
        //Gaussian:
        return std::exp(-0.5*x*x)/std::sqrt(2.0*M_PI);
    }
    
    static inline double l_vector_S(const double h, const samples_1D<double> &data, double x, double j_power){
        double out(0.0);
        for(const auto & sample : data.samples){
            const double dx((sample[0] - x));
            out += l_vector_kernel(dx/h)*std::pow(dx, j_power);
        }
        return out;
    }
    
    static inline std::vector<double> l_vector_b(const double h, const samples_1D<double> &data, double x){
        std::vector<double> out(data.samples.size());
        const double S1(l_vector_S(h,data,x,1.0)), S2(l_vector_S(h,data,x,2.0));
        size_t cnt(0);
        for(auto p_it = data.samples.begin(); p_it != data.samples.end(); ++p_it, ++cnt){
            const double dx(((*p_it)[0] - x));
            const double bi(l_vector_kernel(dx/h)*(S2 - dx*S1));
            out[cnt] = bi;
        }
        return std::move(out);
    }
    
    static std::vector<double> l_vector(const double h, const samples_1D<double> &data , double x, bool *OK){
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        if(!std::isnormal(h) || (h <= 0.0)){ *OK = false; return std::vector<double>(); }
        std::vector<double> bs = l_vector_b(h,data,x);
    
        //Divide each b_i by the sum of all b_i's.
        const double b_sum(std::accumulate(bs.begin(), bs.end(), 0.0));
        if(!std::isnormal(b_sum) || !std::isnormal(1.0/b_sum)){ *OK = false; return std::move(bs); }
        for(double & b : bs) b /= b_sum;
    
        *OK = true; return std::move(bs);
    }
    
    double Get_Smoothed_at_X(double h, double x, const samples_1D<double> &in, bool *OK){
        //Returns the estimated (smoothed) function value at the given x and smoothing parameter h.
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        //Generate the linear combination of 'l's to combine the Y values.
        const std::vector<double> ls(l_vector(h,in,x,OK));
        if(*OK == false){ return -1.0;
            //This is typically due to choice of h being too small, but may be due to many other things.
            // The easiest solution is to try avoid this h. Even better, use cross validation to get the
            // best value of h. This routine will handle these sorts of errors and should return a 'nice'
            // choice of h.
        }
    
        double fval(0.0);
        size_t cnt(0);
        for(auto p_it = in.samples.begin(); p_it != in.samples.end(); ++p_it, ++cnt)  fval += (*p_it)[2] * ls[cnt];
    
        *OK = true; return fval;
    }
    
    //Given an h, a position x, and the log of MSE data, evaluate an estimator of the variance (\hat{\sigma}^{2}(x)).
    //
    //This is almost exactly the same as the generic function evaluation routine (called herein), but also performs an
    // inverse transformation on the data, bringing it back from log space.
    static double Get_MSE_at_X(double h_conf, double x, const samples_1D<double> &log_mse_data_in, bool *OK){
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        const double log_of_mse = Get_Smoothed_at_X(h_conf,x,log_mse_data_in,OK);
        if(*OK == false) return -1.0;
    
        *OK = true; return std::exp(log_of_mse);
    }
    
    double Get_Conf_at_X(double h, double conf_h, double c, double x, const samples_1D<double> &log_mse_data_in, const samples_1D<double> &data, bool *OK){
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        //Get the l-vector at this x.
        const std::vector<double> ls(l_vector(h,data,x,OK));
        if(*OK == false) return -1.0;
    
        //Get the value of the mean-squared-error at this x.
        const double mse = Get_MSE_at_X(conf_h,x,log_mse_data_in,OK);
        if(*OK == false) return -1.0;
        if(mse < 0.0){ *OK = false; return -1.0; }
        const double me = std::sqrt(mse); //Convert \sigma^2 (ie. the mean-squared-error or variance estimator) into \sigma.
    
        //Compute ||l(x)||.
        double l_norm(0.0);
        for(double l : ls) l_norm += l*l;
        l_norm = std::sqrt(l_norm);
    
        *OK = true; return c*me*l_norm; //Now +- this with the smoothed function to get confidence bands!
    }
    
    double Get_Cross_Validation_Leave_One_Out(double h, const samples_1D<double> &in){
        //This function should gracefully handle infinities, computational errors, and any other sort of shit
        // which would derail a lesser program. The user expects to be able to 'just use it' to find an optimal
        // h parameter, and the algorithm searching for the best h parameter should be hand-held through the
        // process in case bad results show up.
        //
        //Therefore, at the slightest sign of computational shenanigans, we return the largest (positive)
        // non-inf value we can. The user can easily catch it, and the algorithm will simply consider it a
        // (highly) non-optimal h.
        bool OK;
        const double N(static_cast<double>(in.samples.size()));
        double out(0.0);
    
        size_t cnt(0);
        for(auto p_it = in.samples.begin(); p_it != in.samples.end(); ++p_it, ++cnt){
            const double x((*p_it)[0]);
            const std::vector<double> ls(l_vector(h,in,x,&OK));
            if(OK == false) return std::numeric_limits<double>::max();
    
            const double Lii(ls[cnt]);
    
            //----------------------
            //Compute estimate function value ($\hat{r}$).
            double rhat(0.0);
            size_t ccnt(0);
            for(auto pp_it = in.samples.begin(); pp_it != in.samples.end(); ++pp_it, ++ccnt){
                rhat += (*pp_it)[2] * ls[ccnt];
            }
            //-----------------------    
            
            const double numer(((*p_it)[2] - rhat)*((*p_it)[2] - rhat));
            const double denom((1.0 - Lii)*(1.0 - Lii));
            
            if(numer == 0.0) continue;
            if((denom == 0.0) || !std::isnormal(numer/denom)){
                return std::numeric_limits<double>::max();
            }
            out += numer/denom;
        }
    
        return out/N;
    }
    
    double Get_Cross_Validation_Generalized_CV(double h, const samples_1D<double> &in){
        //This function should gracefully handle infinities, computational errors, and any other sort of shit
        // which would derail a lesser program. The user expects to be able to 'just use it' to find an optimal
        // h parameter, and the algorithm searching for the best h parameter should be hand-held through the
        // process in case bad results show up.
        //
        //Therefore, at the slightest sign of computational shenanigans, we return the largest (positive)
        // non-inf value we can. The user can easily catch it, and the algorithm will simply consider it a
        // (highly) non-optimal h.
        bool OK;
        const double N(static_cast<double>(in.samples.size()));
        double out(0.0);
    
        size_t cnt(0);
        double TrL(0.0); //Trace of L (sum of all Lii).
        for(auto p_it = in.samples.begin(); p_it != in.samples.end(); ++p_it, ++cnt){
            const double xi((*p_it)[0]);
            const std::vector<double> ls(l_vector(h,in,xi,&OK));
            if(OK == false) return std::numeric_limits<double>::max();
    
            const double Lii(ls[cnt]);
            TrL += Lii;
    
            //----------------------
            //Compute estimate function value ($\hat{r}$).
            double rhat(0.0);
            size_t ccnt(0);
            for(auto pp_it = in.samples.begin(); pp_it != in.samples.end(); ++pp_it, ++ccnt){
                rhat += (*pp_it)[2] * ls[ccnt];
            }
            //-----------------------    
            
            const double numer = ((*p_it)[2] - rhat)*((*p_it)[2] - rhat);
            out += numer;
        }
    
        const double denom = (1.0 - TrL/N)*(1.0 - TrL/N);
    
        if(!std::isnormal(denom) || !std::isnormal(out/denom)){
            return std::numeric_limits<double>::max();
        }
        out /= denom;
        return out/N;
    }
    
    double Find_Optimal_H(double h_0, double h_scale, double cv_tol, long int N_iters, const samples_1D<double> &in, bool show_info, bool *OK){
        //Given a starting point (h_0) and a suspected 'scale' of the problem (ie. how much we need to move h to see a difference)
        // we try a minimization scheme to search for an optimal smoothing parameter h.
        // 
        //This routine is not impervious to local minima or poorly chosen scale. 
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        //This is the function we will minimize. We can use the generalized cross validation score, or leave-one-out, or whatever.
        auto func_to_min = [&in](double p[]) -> double {
            const double l_h = p[0];
            //const double cv = Get_Cross_Validation_Generalized_CV(l_h, in);
            const double cv = Get_Cross_Validation_Leave_One_Out(l_h, in);
            return cv;
        };
    
        //These are the parameters which will be passed in to the function.
        const int dimen = 1;
        double params[dimen];
        double optimal_h, lowest_func_val;
    
        //Fill the parameters with some best-guesses.
        params[0] = h_0;
    
        //NMSimplex<double> minimizer(dimension, characteristic length, max number of iters, ftol);
        NMSimplex<double> minimizer(dimen, h_scale, N_iters, cv_tol);
        minimizer.init(params, func_to_min);
    
        //Iterate through the procedure, breaking on errors or df < ftol.
        while(minimizer.iter() == 0){
            if(show_info){
                minimizer.get_params(params);
                FUNCINFO("Smallest func val is: " << minimizer.func_vals[minimizer.curr_min] << " at h = " << params[0] << " on iteration #" << minimizer.iteration);
//                std::cout << "Smallest func val is: " << minimizer.func_vals[minimizer.curr_min];
//                std::cout << " at h = " << params[0];
//                std::cout << " on iteration #" << minimizer.iteration << std::endl; 
            }
        }
    
        //Error of some kind.
        if(minimizer.iter() == -1){ *OK = false; return -1.0; }
    
        minimizer.get_all(func_to_min,params,lowest_func_val);
        optimal_h = params[0];
    
        //Max number of iterations exceeded. This may or may not be an error. We might still decide to just
        // 'roll with it' at this point. The best thing to do is plot cv versus h and visually inspect!
        if(minimizer.iter() ==  1){ *OK = false; return optimal_h; }
    
        //If we get here, df < ftol, so it is a success.
        *OK = true; return optimal_h;
    }
    
    //This function computes an estimate of 'c' - the 'confidence' scale which is used in computing the confidence bands.
    // It varies with the input data and the choice of confidence percentage (ie. 95% confidence bands). This function 
    // also determines an approximate estimate of k_0 - the leading order 'geometrical manifold volume' which is used to
    // compute 'c'. It is not a terribly precide computation of k_o, and may be altered in the future.
    double Find_C(double h, double confidence, const samples_1D<double> &in, bool *OK){
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        //Note:  95% = 100%*(1-\alpha). Therefore,  alpha = 0.05 -> 95% confidence, similarly
        // alpha = 0.1 -> 90% confidence and alpha = 0.5 -> 50% confidence, etc.
        const double alpha = 1.0 - (confidence/100.0);
        if(!isininc(0.001, alpha, 0.999)){ *OK = false; return -1.0; }
    
        //We step over each point and perform numerical integration. This is not very precise. A better approach would 
        // involve adaptive integration. In nearly-constant variance data, I expect this to be sufficiently accurate 
        // for the purposes of generating confidence intervals.
        auto p1_it = in.samples.begin();
        auto p2_it = ++(in.samples.begin());
        double k0(0.0);
        for(  ; p2_it != in.samples.end(); ++p1_it, ++p2_it){
            //const double x(0.5(p2_it->x + p1_it->x));
            //const double dx(p2_it->x - p1_it->x);
    
            //At each point, compute and normalize the l-vectors.
            std::vector<double> ls_1(l_vector(h,in,(*p1_it)[0],OK));
            if(*OK == false) return std::numeric_limits<double>::max();
            std::vector<double> ls_2(l_vector(h,in,(*p2_it)[0],OK));
            if(*OK == false) return std::numeric_limits<double>::max();
    
            {
              double ls_1_norm(0.0);
              for(double & it : ls_1) ls_1_norm += it*it;
              ls_1_norm = std::sqrt(ls_1_norm);
              if(!std::isnormal(1.0/ls_1_norm)){ *OK = false; return -1.0; }
              for(double & it : ls_1) it /= ls_1_norm;
            }
    
            {
              double ls_2_norm(0.0);
              for(double & it : ls_2) ls_2_norm += it*it;
              ls_2_norm = std::sqrt(ls_2_norm);
              if(!std::isnormal(1.0/ls_2_norm)){ *OK = false; return -1.0; }
              for(double & it : ls_2) it /= ls_2_norm;
            }
    
            //Compute ||T'(x)||*dx at this x. Because we use the centre-point approximation, we can say that
            // at this point we have ||T'(x)||*dx = sqrt( Sum_j of (T(x_{j+1}) - T(x_{j}))^2 ).
            double shtl(0.0);
            {
                auto t1_it = ls_1.begin();
                auto t2_it = ls_2.begin();
                for(  ; (t2_it != ls_2.end()) && (t1_it != ls_1.end()); ++t1_it, ++t2_it){
                    shtl += ((*t2_it) - (*t1_it))*((*t2_it) - (*t1_it));
                }
                shtl = std::sqrt(shtl);
                //shtl /= dx;
            }
            k0 += shtl; // *dx;
        }
    
        //Now, using our rough k0, compute the corresponding c.
        double c;
        {
          const double numer = (1.0/std::sqrt(2.0*M_PI)) + (k0/M_PI);
          const double denom = 0.05;
          const double theln = std::log(numer/denom);
          if(theln < 0.0){ *OK = false; return -1.0; }
          c = std::sqrt(2.0*theln);
        }
    
        *OK = true; return c;
    }
    
    //This function takes the data's optimal h and returns log(MSE) data which is itself suitable for regression.
    samples_1D<double> Log_of_MSE(double h, const samples_1D<double> &in, bool *OK){
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        //Get a (smoothed) estimate of the input data on the input data x_i's.
        samples_1D<double> s_f(Get_Smoothed_at_Xi(h,in,OK));
        if(*OK == false) return std::move(s_f);
    
        //Now, transform the data using F[smoothed_f_i] = ln( (Y_i - smoothed_f_i)**2 ) where Y_i is the input Y at the point X_i
        // where smoothed_f_i was calculated.
        auto p_it = in.samples.begin();
        auto s_it = s_f.samples.begin();
        for( ; (p_it != in.samples.end()) && (s_it != s_f.samples.end()); ++p_it, ++s_it){
            (*s_it)[2] = std::log( ((*s_it)[2] - (*p_it)[2])*((*s_it)[2] - (*p_it)[2]) );
        }
    
        *OK = true; return std::move(s_f);
    }
   
    //This is strictly a convenience function.
    //Breaks up the input data into (xmax-xmin)/dx equal-length parts. Samples the smoothed function at each. 
    samples_1D<double> Get_Smoothed_Evenly_Spaced(double h, double dx, const samples_1D<double> &in, bool *OK){
        //Return constant-separation (dx) smoothed function data.
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        samples_1D<double> out;
        const double xmin(in.samples.front()[0]);
        const double xmax(in.samples.back()[0]);
        for(auto x = xmin; x < xmax; x += dx){
            const double fval(Get_Smoothed_at_X(h,x,in,OK));
            if(*OK == false) return std::move(out);
            out.push_back(vec2<double>(x, fval));
        }
    
        *OK = true; return std::move(out);
    }
    
    //For the given input (x_i,y_i), generate smoothed output (x_i,s_y_i) at the (exact) same x_i. This is used internally
    // and might be of interest to the user as a convenience function.
    samples_1D<double> Get_Smoothed_at_Xi(double h, const samples_1D<double> &in, bool *OK){
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;
    
        //Generate a (smoothed) estimate of the input data on the input data x_i's.
        samples_1D<double> s_f(in); //"smoothed function"
        for(auto p_it = s_f.samples.begin(); p_it != s_f.samples.end(); ++p_it){
            const auto fval = Get_Smoothed_at_X(h,(*p_it)[0],in,OK);
            if(*OK == false) return std::move(s_f);
            (*p_it)[2] = fval;
        }
        *OK = true; return std::move(s_f);
    }


    //This function should work for most cases. Given some data, it attempts to smooth it. It then computes smoothed data
    // using Get_Smoothed_at_Xi(...). No info about h or c is given back - just the data.
    //
    //Only use this if you are satisfied with the output and/or have previously investigated the effect of h on the data.
    samples_1D<double> Attempt_Auto_Smooth(const samples_1D<double> &in, bool *OK){
        if(OK == nullptr) FUNCERR("Provided a nullptr when it was not expected");
        *OK = false;

        const double xmin      = in.samples.front()[0];
        const double xmax      = in.samples.back()[0];
        const double N         = static_cast<double>(in.size());
        const double avg_dx    = (xmax - xmin)/N;

        const double h_0       = (xmax - xmin)/5.0;
        const double h_scale   = 5.0 * avg_dx;

        const double h_optimal = Find_Optimal_H(h_0, h_scale, 1E-7, 5000, in, false, OK);
        if(*OK == false) return samples_1D<double>();

        *OK = true; return Get_Smoothed_at_Xi(h_optimal, in, OK);
    }

    //This routine will perform a battery of tests against some data. It should be suitable for initial exploration of some
    // data. 
    // NOTE: -To kill the process, send a nullptr for OK. Otherwise, errors will be signaled through it.
    void Attempt_Auto_Analysis(const samples_1D<double> &data, const std::string &Title /*=""*/, bool only_show_final /*=true*/, Plotter2 *plotter /*=nullptr*/, bool *OK /*=nullptr*/){
        if(OK != nullptr) *OK = false;
        bool l_OK(false);  //Local OK signalling.

        const double xmin       = data.samples.front()[0];
        const double xmax       = data.samples.back()[0];
        const double N          = static_cast<double>(data.samples.size());
        const double avg_dx     = (xmax - xmin)/N;

        const double h_0        = (xmax - xmin)/5.0;
        const double h_scale    = 5.0 * avg_dx;
        const std::string title = (Title.empty()) ? "" : Title + " - ";


        //Plot of specific smoothed function compared with original data.
        {
            std::list<double> hs({0.001, 0.01, 0.1, 1.0, 10.0, 25.0, 37.0, 50.0, 100.0, 150.0, 200.0, 300.0 });
            std::map<std::string, samples_1D<double>> plot_shtl;
            plot_shtl["Original"] = data;
            for(double & h : hs){
                const auto smoothed = NPRLL::Get_Smoothed_Evenly_Spaced(h, 2.0, data, &l_OK); 
                if(l_OK) plot_shtl["h = "_s + Xtostring(h)] = smoothed;
            }
            if(!only_show_final) Plot2_Map_of_Samples_1D(plot_shtl.begin(), plot_shtl.end(), title + "Different smoothing parameters");
        }
    
        //Examine the cross-validation score over a range of h's.
        {
            std::map<std::string, samples_1D<double>> plot_shtl;
            {
              samples_1D<double> cvs_loo, cvs_gcv;
              for(double h = 0.5; h < 500.0; h += 2.0){
                  const double cv_loo = NPRLL::Get_Cross_Validation_Leave_One_Out(h,data);
                  if(cv_loo > 1E50){ /* cvs_loo.samples.push_back(vec2<double>(h,-50.0)); */
                  }else{             cvs_loo.push_back(vec2<double>(h,cv_loo));
                  }
                  const double cv_gcv = NPRLL::Get_Cross_Validation_Generalized_CV(h,data);
                  if(cv_gcv > 1E50){ /* cvs_gcv.samples.push_back(vec2<double>(h,-50.0)); */
                  }else{             cvs_gcv.push_back(vec2<double>(h,cv_gcv));
                  }
              }
              plot_shtl["Leave-one-out CV"] = cvs_loo;
              plot_shtl["Generalized CV"]   = cvs_gcv;
            }
            if(!only_show_final) Plot2_Map_of_Samples_1D(plot_shtl.begin(), plot_shtl.end(), title + "Cross-validation vs h");
        }
    
        //Perform a full-featured analysis. Starting with raw data, perform non-parametric regression, compute some
        // 95% confidence intervals, and plot: the data, the smoothed data, and the confidence bands.
        {
            std::map<std::string, samples_1D<double>> plot_shtl;
            plot_shtl["Original Data"] = data;
    
            //Smoothed function approximation (quantized on each x_i).
            const bool verbose_optimal_h = false;
            const double h_optimal = NPRLL::Find_Optimal_H(h_0, h_scale, 1E-7, 5000, data, verbose_optimal_h, &l_OK);
            if(l_OK == false){  
                if(OK == nullptr) FUNCERR("Unable to determine an optimal h. Value returned was " << h_optimal);
                FUNCWARN("Unable to determine an optimal h. Value returned was " << h_optimal);
                return;
            }else{
                FUNCINFO("Found optimal h = " << h_optimal);
            }
    
            {
              const auto smoothed = Get_Smoothed_Evenly_Spaced(h_optimal, avg_dx, data, &l_OK);
              if(l_OK == true) plot_shtl["Smoothed Optimally"] = smoothed;  //Should be '~Optimally' but makes it annoying to plot.
            }
    
            //--------------------------------------
            //Compute necessary info to evaluate the confidence bands (not quite of the smooth function approximation, but close!).
            const auto log_mse_est = NPRLL::Log_of_MSE(h_optimal, data, &l_OK);
            if(l_OK == false){
                if(OK == nullptr) FUNCERR("Encountered error computing log of MSE estimate data");
                FUNCWARN("Unable to compute log of MSE estimate data");
                return;
            }    
            const double conf_h_0       = 100.0;
            const double conf_h_scale   = 5.0;
            const double conf_h_optimal = NPRLL::Find_Optimal_H(conf_h_0, conf_h_scale, 1E-7, 5000, log_mse_est, verbose_optimal_h, &l_OK);
            if(l_OK == false){
                if(OK == nullptr) FUNCERR("Encountered error computing smoothed function");
                FUNCWARN("Unable to compute smoothed function");
                return;
            }

            const double c = NPRLL::Find_C(h_optimal, 95.0, data, &l_OK);
            {
              samples_1D<double> plot_l, plot_u;
              for(auto x = xmin; x < xmax; x += 2.0*avg_dx){
                  const double fval = NPRLL::Get_Smoothed_at_X(h_optimal, x, data, &l_OK);
                  if(l_OK == false) continue;
                  const double band = NPRLL::Get_Conf_at_X(h_optimal, conf_h_optimal, c, x, log_mse_est, data, &l_OK);
                  if(l_OK == false) continue;
    
                  plot_l.push_back(vec2<double>(x,fval - band));
                  plot_u.push_back(vec2<double>(x,fval + band));
              }
              plot_shtl["Lower band - 95%"] = plot_l;
              plot_shtl["Upper band - 95%"] = plot_u;
            }
   
            if(plotter != nullptr){
                plotter->Set_Global_Title(title + "NPRLL"); //User can override this later.
                plotter->Insert_map_of_string_and_samples_1D(plot_shtl);
            }else{ 
                Plot2_Map_of_Samples_1D(plot_shtl.begin(), plot_shtl.end(), title + "NPRLL");
            }
        }

        if(OK != nullptr) *OK = true;
        return;
    }

} //end of namespace NPRLL.


//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------- Architecture-independant Non-cryptographic Hashes --------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
uint64_t Consistent_Hash_64(const std::string &in){
    if(in.empty()) return static_cast<uint64_t>(1337);

    SpookyHash H;
    const void *p = static_cast<const void *>(const_cast<char *>(in.c_str()));
    const uint64_t seed(1337331337);
    return H.Hash64(p, in.size(), seed);
}

//MD5 hash.
std::unique_ptr<uint8_t[]> MD5_Hash(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t bytecnt, std::string *hash){
    if(OK != nullptr) *OK = false;
    if(hash == nullptr){
        if(OK == nullptr) FUNCERR("Passed a nullptr instead of a space to store resulting hash. Cannot continue");
        FUNCWARN("Passed a nullptr instead of a space to store resulting hash. Bailing");
        return std::move(in);
    }
    if(!hash->empty()){
        FUNCWARN("Passed a non-empty hash store. Assuming appending hashes is desired");
    }

    MD5::Context working;
    MD5::Init(&working);
    MD5::Update(&working, (void *)(in.get()), bytecnt);

    unsigned char *result = new unsigned char[16+1];
    result[16] = '\0';
    MD5::Final(result, &working);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for(int i = 0; i < 16; ++i) ss << std::setw(2) << static_cast<unsigned>(result[i]);
    delete[] result;
    *hash += ss.str();

    if(OK != nullptr) *OK = true;
    return std::move(in);
}


//----------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Fitting Algorithms ----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------


//This driver is wrapped by the other routines. Preferentially use them.
std::tuple<std::list<double>,double,double,long int,double,double,double,std::list<double>>
Ygor_Fit_Driver(bool *wasOK,
             const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
             const std::list<std::list<double>> &data,
             const std::list<double> &vars,
             uint32_t fitflags,
             bool Verbose,
             double char_len,
             int max_iters,
             double ftol ){


    if(wasOK == nullptr) FUNCERR("Passed a nullptr bool. Unable to signal whether evaluation worked or not");
    *wasOK = false;
    std::tuple<std::list<double>,double,double,long int,double,double,double,std::list<double>> out;

    // ----------------- Sanity checking -------------------
    if(!f || data.empty() || vars.empty()){
        FUNCWARN("Given insufficient or incomplete information to perform fit. Bailing");
        return out;

    }else if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::DIM2) && !isininc(2,data.front().size(),3)){
        FUNCWARN("Given too much or too little data for 2D fit. Bailing");
        return out;

    }else if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::DIM3) && !isininc(3,data.front().size(),4)){
        FUNCWARN("Given too much or too little data for 3D fit. Bailing");
        return out;

    }else if(!BITMASK_BITS_ARE_SET(fitflags,YGORFIT::LSS) && !BITMASK_BITS_ARE_SET(fitflags,YGORFIT::LMS)){
        FUNCWARN("Need to specify least-sum of squares or least-median of squares. Bailing");
        return out;

    }else{
        //All rows have same number of cols.
        auto it = data.begin();
        size_t numcols = it->size();
        do{
            if(numcols != it->size()){
                FUNCWARN("Found two rows which differ in number of columns. Please pad to identical sizes. Bailing");
                return out;
            }
            numcols = it->size();
        }while((++it) != data.end());
    }


    // ---------------- Setup -----------------
    const auto DOF = static_cast<long int>(data.size()) - static_cast<long int>(vars.size()); 
    const size_t numb_of_fit_parameters = vars.size();
    const auto N_COLS = data.front().size();

    // ---------------- 'sticky' statistical quantities for later use ------------------- 
    //We capture the best when we see them in case the max number of iterations is exceeded
    // and the routine craps out prematurely. In theory, the best should be the last (when 
    // ftol is achieved).
    bool Performed_First_Pass = false; //On the first pass through (only), we will compute this for later use.
    double F_data_mean = 0.0;
    double best_SStot = std::numeric_limits<double>::max();  // = sum_i (fi - fmean)^2.
    double best_SSres = std::numeric_limits<double>::max();  // = sum_i (fi - f_meas_i)^2  aka ~what we are minimizing.
    double best_WSSR  = std::numeric_limits<double>::max();  // same as SSres but contains a weighting factor. This what we are minimizing.
    std::list<double> best_SRs;

    // ----------------- minimization lambda ------------------
    bool failure_inside_func_to_min = false;   //Escape signal. If this is set (within func_to_min), iteration halts asap.
    auto func_to_min = [&](double p[]) -> double {
        //We want to minimize WSSR = 'weighted sum of squares of residuals' = 'weighted residual sum of squares'
        // by adjusting A,B,C,...  The unweighted SSres is equivalent to WSSR when wi = 1.0;
        double SStot(0.0), SSres(0.0), WSSR(0.0);
        std::list<double> SRs;

        //Iterate over rows of data.
        for(const auto & r_it : data){
            std::list<double> X; //x-values. This will always be 1 for f(x), 2 for f(x,y), etc..
            std::list<double> P; //Parameters. We just convert from double[] to std::list.
            for(size_t i=0; i<numb_of_fit_parameters; ++i) P.push_back(p[i]);
            double W(0.0);
            double F_data;

            if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::DIM2) && (N_COLS == 2)){
                // Data:   <X_i> <F_i>
                X.push_back(r_it.front());
                F_data = r_it.back();
                W = 1.0; //Effectively no weighting.
         
            }else if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::DIM2) && (N_COLS == 3)){
                // Data:   <X_i> <F_i> <σ_F_i>
                X.push_back(r_it.front());
                F_data = *std::next(r_it.begin());
                W = r_it.back();

            }else if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::DIM3) && (N_COLS == 3)){
                // Data:   <X_i> <Y_i> <F_i>
                X.push_back(*std::next(r_it.begin(),0));
                X.push_back(*std::next(r_it.begin(),1));
                F_data = r_it.back();
                W = 1.0; //Effectively no weighting.

            }else if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::DIM3) && (N_COLS == 4)){
                // Data:   <X_i> <Y_i> <F_i> <σ_F_i>
                X.push_back(*std::next(r_it.begin(),0));
                X.push_back(*std::next(r_it.begin(),1));
                F_data = *std::next(r_it.begin(),2);
                W = r_it.back();

            }else{
                FUNCWARN("Currently cannot understand what type of analysis you are asking for. Implement it here");
                failure_inside_func_to_min = true;
                return -1.0;

            }
 
            const double F_func = f(X,P);
            const auto SR = (F_func - F_data)*(F_func - F_data); //Squared residual.
            SRs.push_back(SR);

            //Uncertainties override.
            if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::UNCERT_CAUCHY)){
                W = (1.0 + 4.0*SR); //Cauchy distribution. Should help LSS reject outliers better.
            }

            //Sanity checking.
            if(W == 0.0){
                FUNCWARN("Somehow ended up with zero uncertainty. This routine cannot handle zero uncertainty");
                failure_inside_func_to_min = true;
                return -1.0;
            }

            //Stats storage.
            WSSR += SR/(W*W);
            if(!Performed_First_Pass){ 
                F_data_mean += F_data;
            }else{
                SSres += SR; //+=(func - func_data)*(func - func_data)
                SStot += (F_data - F_data_mean)*(F_data - F_data_mean);
            }
        }
   
        //Stick the Sticky statistics, if appropriate.
        if(!Performed_First_Pass){
            Performed_First_Pass = true;
            F_data_mean /= static_cast<double>(data.size());
        }else if(WSSR < best_WSSR){
            best_WSSR  = WSSR;
            best_SSres = SSres;
            best_SStot = SStot;
            best_SRs   = SRs;
        }

        //What does our objective function return?
        if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::LSS)){
            //Regular least-squares approach.
            return WSSR;
        }else if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::LMS)){
            //NOTE: All statistics using this approach will be GARBAGE! (Use bootstrap instead!)
            return Stats::Median(SRs);
        }

        //------ We should never get here. -----
        FUNCERR("Objective function not properly specified. Cannot continue");
        return -1.0 - DOF;
    };

    //----------------- Minimization Setup -----------------
    //These are the parameters which will be passed in to the function.
    std::unique_ptr<double> min_params = nullptr;
    min_params.reset(new double[numb_of_fit_parameters]);
    
    //Fill the parameters with some best-guesses.
    {
      auto varit = vars.begin();
      for(size_t i=0; i<numb_of_fit_parameters; ++i, ++varit){
          (min_params.get())[i] = *varit;
      }
    }

    //NMSimplex<double> minimizer(numb_of_fit_parameters, characteristic length, max number of iters, ftol);
    NMSimplex<double> minimizer(numb_of_fit_parameters, char_len, max_iters, ftol);
    minimizer.init(min_params.get(), func_to_min);

    //-------------- Minimization computation ----------------
    while(minimizer.iter() == 0){
        if(failure_inside_func_to_min == true){
            FUNCWARN("Encountered failure whilst trying to evaluate function to be minimized. Cannot continue");
            return out;
        }
        if(Verbose){
            std::cout.precision(10);
            FUNCINFO("Minimization: At iteration " << minimizer.iteration << " the lowest value is " << minimizer.func_vals[minimizer.curr_min]);
        }
    }

    if(minimizer.iter() == -1){
        FUNCERR("Min scheme exited due to error. Maybe the initial params were not set?");
    }else if(minimizer.iter() ==  1){
        FUNCWARN("Min scheme finished due to max num of iters being exceeded.");
        if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::NON_CONVERG_IS_ERR)){
            *wasOK = false;
            return out;
        }
    }else if((minimizer.iter() ==  2) && Verbose) FUNCINFO("Min scheme completed due to ftol < ftol_min.");

    std::list<double> best_params;
    minimizer.get_params(min_params.get());
    for(size_t i=0; i<numb_of_fit_parameters; ++i){
        best_params.push_back( (min_params.get())[i] );
    }

    //------------ output synthesis ------------- 
    if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::LSS)){
        const auto chi_sq = best_WSSR;
        const auto red_chi_sq = chi_sq/static_cast<double>(DOF);
        const auto Qvalue = Stats::Q_From_ChiSq_Fit(chi_sq, static_cast<double>(DOF));
 
        //Not sure which is more useful. See http://en.wikipedia.org/wiki/Coefficient_of_determination. 
        const auto raw_coeff_deter = 1.0 - best_SSres/best_SStot; //Uses unweighted SSres, not weighted SSres. Use this if worried about weight scaling.
        const auto mod_coeff_deter = 1.0 - best_WSSR/best_SStot;  //Uses weighted SSres. This is the consistent choice. Use this if in doubt.
        //best_SRs contains an unordered listing of all squared-residuals and is suitable for inspection/verification of uncertainties.
        out = std::make_tuple(best_params, chi_sq, Qvalue, DOF, red_chi_sq, raw_coeff_deter, mod_coeff_deter, best_SRs);
    }else if(BITMASK_BITS_ARE_SET(fitflags,YGORFIT::LMS)){
        //NOTE: If you are reading this and wondering what to do for stats: "Use your bootstraps, Luke!"
        const auto chi_sq = -1.0;  
        const auto red_chi_sq = -1.0;
        const auto Qvalue = -1.0;
        const auto raw_coeff_deter = -1.0;
        const auto mod_coeff_deter = -1.0;
        out = std::make_tuple(best_params, chi_sq, Qvalue, DOF, red_chi_sq, raw_coeff_deter, mod_coeff_deter, best_SRs);

    }else{
        FUNCERR("Unspecified fitting method. Unable to compute any stats or continue");
    }
    *wasOK = true;
    return out;
}


std::tuple<std::list<double>,double,double,long int,double,double,double,std::list<double>>
Ygor_Fit_LSS(bool *wasOK,
             const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
             const std::list<std::list<double>> &data,        const std::list<double> &vars,
             long int dim,  bool Verbose/*=false*/,  double char_len/*=0.6*/,  int max_iters/*=1500*/,
             double ftol/*=1E-6*/ ){

    uint32_t fitflags(0);
    if(dim == 2){        fitflags |= YGORFIT::DIM2;
    }else if(dim == 3){  fitflags |= YGORFIT::DIM3;
    }else{
        FUNCERR("Cannot handle dimensions other than 2 [f(x)] or 3 [f(x,y)]. Cannot proceed");
    }
    fitflags |= YGORFIT::LSS;

    return Ygor_Fit_Driver(wasOK,f,data,vars,fitflags,Verbose,char_len,max_iters,ftol);
}

std::tuple<std::list<double>,std::list<double>>
Ygor_Fit_LMS(bool *wasOK,
             const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
             const std::list<std::list<double>> &data,        const std::list<double> &vars,
             long int dim,  bool Verbose/*=false*/,  double char_len/*=0.6*/,  int max_iters/*=1500*/,
             double ftol/*=1E-6*/ ){

    //NOTE: This is *NOT* the least-median:  min[median( (F_i - f(x,y,..;A,B,..))   )]  but rather
    //      the least-median-of-squares:     min[median( (F_i - f(x,y,..;A,B,..))^2 )]. There is a 
    //      distinction. The LMS is more robust than the LM. See the article "Least Median of 
    //      Squares Regression" by PETER J. ROUSSEEUW (1984) for more info and some history. 
    //

    uint32_t fitflags(0);
    if(dim == 2){        fitflags |= YGORFIT::DIM2;
    }else if(dim == 3){  fitflags |= YGORFIT::DIM3;
    }else{
        FUNCERR("Cannot handle dimensions other than 2 [f(x)] or 3 [f(x,y)]. Cannot proceed");
    }
    fitflags |= YGORFIT::LMS;

    auto tup = Ygor_Fit_Driver(wasOK,f,data,vars,fitflags,Verbose,char_len,max_iters,ftol);
    const auto best_params = std::get<0>(tup);
    const auto best_SRs    = std::get<7>(tup);
    return std::make_tuple(best_params,best_SRs);
}




std::list<std::list<double>> Ygor_Fit_Bootstrap_Driver(bool *wasOK,
         const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
         const std::list<std::list<double>> &data,  const std::list<double> &vars, uint32_t fitflags,
         long int N,   double char_len,   int max_iters,    double ftol){

    *wasOK = false;
    std::list<std::list<double>> out;
    for(size_t i = 0; i < vars.size(); ++i) out.push_back(std::list<double>());

    long int MAX_N_ERRS = 100*N;  //Will happen if single row is picked or some other unreasonable scenario.
    long int N_ERRS = 0;

    std::random_device rdev;
    std::mt19937 re( rdev() ); //Random engine.
    std::uniform_int_distribution<> rd(0, data.size()-1); //Random distribution.

    //Fill up the substitute data buffer with randomly-chosen values.
    std::list<std::list<double>> sub_data;
    while(sub_data.size() != data.size()){
        const auto nth = rd(re);
        sub_data.push_back( *std::next(data.begin(),nth) );
    }

FUNCWARN("++++++++ USING RANDOMIZED BEST GUESSES. MIGHT WANT TO DISABLE THIS +++++++++");
FUNCWARN("+++++++ IN EITHER CASE, PLUMB IT IN PROPERLY. IT IS JUST BOLTED ON. ++++++++");

    //Perform fits with randomly-selected data until we have N samples of the parameters.
    time_mark Began;
    while(true){
        //Select a row to discard and a row to inject. Try to get ~10% of the rows each time
        for(double x = 0.0; x < 0.1*static_cast<double>(sub_data.size()); x+=1.0){
            const auto nth = rd(re), mth = rd(re);
            sub_data.erase( std::next(sub_data.begin(),nth) );
            sub_data.push_back( *std::next(data.begin(),mth) );
        }
        std::list<double> fit_params;

//Generate a RANDOMIZED set of starting parameters.
// Ideally, this would use realtime bootstrap variances to estimate the width of the 
// sample distribution to use. I just threw this in to gain some insight into the
// stability of the bootstrap process and the parameter search space.
std::list<double> randomized_vars;
{
  for(double var : vars){
      std::uniform_real_distribution<double> rd_t(0.01*var,1.99*var); //Can't I do something more intelligent here?
      const double val = rd_t(re);
      randomized_vars.push_back(val);
  }
}

//        auto tup = Ygor_Fit_Driver(wasOK,f,sub_data,vars,fitflags,false,char_len,max_iters,ftol);
        auto tup = Ygor_Fit_Driver(wasOK,f,sub_data,randomized_vars,fitflags,false,char_len,max_iters,ftol);
        fit_params = std::get<0>(tup);

        if(*wasOK == false){
            ++N_ERRS;
        }else if((out.front().size() % 50) == 5){ //Can get really annoying, but I guess it can be easily filtered.
            const auto dt = static_cast<double>(Began.Diff_in_Seconds(time_mark()));
            const auto dbl_n  = static_cast<double>(out.front().size()) + 0.001;
            const auto dbl_N  = static_cast<double>(N);
            
            FUNCINFO("Estimated remaining time: " << ((dbl_N/dbl_n)-1.0)*dt);
        }
        if(N_ERRS >= MAX_N_ERRS){
            *wasOK = false;
            return out;
        }

        //Get the data.
        {
          auto fpit = fit_params.begin(); 
          auto oit  = out.begin();
          while((fpit != fit_params.end()) && (oit != out.end())){
              oit->push_back(*fpit);
              ++fpit;
              ++oit;
          }
        }
        if(static_cast<long int>(out.front().size()) >= N) break;
    }

    //All rows have same number of cols.
    {
      auto it = out.begin();
      size_t numcols = it->size();
      do{
          if(numcols != it->size()){
              FUNCWARN("Failed to produce N samples of fitting parameters due to programming error");
              return out;
          }
          numcols = it->size();
      }while((++it) != out.end());
    }

    *wasOK = true;
    return out;
}



std::list<std::list<double>>
Ygor_Fit_Bootstrap_LSS(bool *wasOK,
             const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
             const std::list<std::list<double>> &data,  const std::list<double> &vars, long int dim, 
             long int N,  double char_len,  int max_iters, double ftol){

    uint32_t fitflags(0);
    if(dim == 2){        fitflags |= YGORFIT::DIM2;
    }else if(dim == 3){  fitflags |= YGORFIT::DIM3;
    }else{
        FUNCERR("Cannot handle dimensions other than 2 [f(x)] or 3 [f(x,y)]. Cannot proceed");
    }
    fitflags |= YGORFIT::LSS;
    fitflags |= YGORFIT::NON_CONVERG_IS_ERR;  //We want to ensure our stats aren't messed up by default params.

    return Ygor_Fit_Bootstrap_Driver(wasOK,f,data,vars,fitflags,N,char_len,max_iters,ftol);
}

std::list<std::list<double>>
Ygor_Fit_Bootstrap_LMS(bool *wasOK,
             const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
             const std::list<std::list<double>> &data,  const std::list<double> &vars, long int dim, 
             long int N,  double char_len,  int max_iters, double ftol){

    uint32_t fitflags(0);
    if(dim == 2){        fitflags |= YGORFIT::DIM2;
    }else if(dim == 3){  fitflags |= YGORFIT::DIM3;
    }else{
        FUNCERR("Cannot handle dimensions other than 2 [f(x)] or 3 [f(x,y)]. Cannot proceed");
    }
    fitflags |= YGORFIT::LMS;
    fitflags |= YGORFIT::NON_CONVERG_IS_ERR;  //We want to ensure our stats aren't messed up by default params.

    return Ygor_Fit_Bootstrap_Driver(wasOK,f,data,vars,fitflags,N,char_len,max_iters,ftol);
}




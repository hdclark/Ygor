//YgorAlgorithms.h - A collection of some custom (niche?) algorithms. This includes things like multidimensional minimization
// routines (and other 'algorithm'-sorts of classes.)

#ifndef YGOR_CUSTOM_ALGORITHMS_HC_
#define YGOR_CUSTOM_ALGORITHMS_HC_

#ifndef YGOR_CUSTOM_ALGORITHMS_HC_ATTEMPT_ASYNCHRONOUS
    #define YGOR_CUSTOM_ALGORITHMS_HC_ATTEMPT_ASYNCHRONOUS
#endif



#include <iostream>
#include <cstddef>       //Needed for ptrdiff_t
#include <cmath>
#include <functional>    //Needed to (easily) accept lambda functions in higher-order functions.
#include <algorithm>     //Needed for random_shuffle(...)
#include <random>        //Needed for std::random_device.
#include <vector>
#include <list>
#include <tuple>


//NOTE: It is *probably* better to NOT use an aggressive attempt to force asynchronicity!
// Exceptions can result otherwise, and my (more modern) laptop calls asynchronously whenever
// possible anyways. Ygor (older) desktop cannot handle it, though, and pukes out a non-catchable
// exception if forced.
// 
//#ifdef YGOR_CUSTOM_ALGORITHMS_HC_ATTEMPT_ASYNCHRONOUS
//    #include <system_error>  //Needed to launch non-deferrable std::async(..)'s.
//#endif

#include <stdexcept>

#include <future>
//#include <chrono>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorPlot.h"

//----------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------- NMSimplex ----------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//Implements the "Nelder-Mead simplex" (or "Downhill simplex," or "Amoeba") method in double precision.
//
//NOTE: This code was written mostly by referring to Numerical Recipes and the GNU Scientific Library. It is my belief
// that it does not follow so closely to either as to infringe on either of their copyrights. However, care should be 
// exercised if planning on releasing this code. Perhaps a complete re-write and/or modification would be in order.
//
//Numerical Recipes mentions that the simplex method computes the function more often than other methods and suggests 
// that Powell's method is usually more effcient.
//
//
//For usage details, refer to the test included in the Project - Utilities tests and examples directory.
//
template <class T> class NMSimplex{
    public:
       T alpha, gamma, rho, sigma;   //Used as input parameters by the method.
       int curr_min;
       int curr_max;
       int curr_scnd_max;
       int DIM;
       T *P_vecs;
       T *func_vals;        //Stores the function values at each P_vec.
       T *centroid;         //Stores (worst-point-excluded) centroid.
       T *reflec;           //Stores the reflected point. It is always calculated.
       T *expand;           //Stores the expanded point, if it is calculated.
       T temp_reflec;
       T temp_expand;
       int iteration;       //The current algorithm's iteration.
       int max_iters;
       T last_best;
       T ftol_min;
       T ftol;              //The amount the function changes by (when it does). This is used as an indicator to halt.
       int init_done;       //Whether the initialization has been called yet.
       T c_leng;            //The characteristic length of the system. IE: A rough scale (0.001, 1.0, 10000,  etc..).
//       T (*the_func)(T *);  //A reference to the function to be minimized. The (T *) is indicative that 
       std::function<T(T *)> the_func;

        //Constructor. 
        NMSimplex(int dim, T c_leng_in, int max_iters_in, T ftol_min_in){
            DIM = dim;
            c_leng = c_leng_in;
            max_iters = max_iters_in;
            ftol_min = ftol_min_in;

            //We first need to create (dim+1) vectors.
            P_vecs =    (T*)malloc((DIM+1)*DIM*sizeof(T));  //This holds all the "P" vectors.
            //This vector stores function values at each P vector.
            func_vals = (T*)malloc((DIM+1)*sizeof(T));
            //Some working space for transferring data.
            centroid  = (T*)malloc((DIM)*sizeof(T));
            reflec    = (T*)malloc((DIM)*sizeof(T));
            expand    = (T*)malloc((DIM)*sizeof(T));
            alpha = 1.0;  //Standard values for the Nelder-mead simplex method. Could maybe use some tweaking...
            gamma = 2.0;
            rho   = 0.5;
            sigma = 0.5;
            iteration = 0;
            ftol = ftol_min * 100.0;
            init_done = 0;          //This is flipped on when initialization is completed.
        }

        //Destructor.
        ~NMSimplex(void){
             free(P_vecs);
             free(func_vals);
             free(centroid);
             free(reflec);
             free(expand);
        }

        //Minimization routine (single) iteration.
        int iter(void){
           if(iteration == 0){
               //For the P* vectors other than P0, we set them equal to a linear combination
               // of P0 and the parameter unit vectors using c_leng.
               for(int i=1;i<=DIM;++i) for(int j=0;j<DIM;++j){
                   P_vecs[i*DIM+j] = P_vecs[0*DIM+j];
                   if(i-1 == j) P_vecs[i*DIM+j] += c_leng*1.0;               //The 1.0 comes from the unit vector.
               }
               //Compute all function values. This will be costly but is necessary right now.
               for(int i=0;i<=DIM;++i) func_vals[i] = the_func(&P_vecs[i*DIM]);
               //Set the ftol catch to something relevant to the system... (Better idea for this??)
               last_best = func_vals[0];
               init_done = 1;
           }
           if(iteration <= max_iters && ftol > ftol_min && init_done == 1){
               //Find the worst function value. 
               curr_max = 0;
               for(int i=1;i<=DIM;++i) if( func_vals[i] > func_vals[curr_max] ) curr_max = i;

               //Find the second worst function value.
               curr_scnd_max = 0;
               for(int i=1;i<=DIM;++i) if( i != curr_max ) if( func_vals[i] > func_vals[curr_scnd_max] ) curr_scnd_max = i;

               //Find the best function value.
               curr_min = 0;
               for(int i=1;i<=DIM;++i) if( i != curr_max ) if( i != curr_scnd_max ) if( func_vals[i] < func_vals[curr_min] ) curr_min = i;

               //Update the ftol value.
               if( last_best != func_vals[curr_min] ){ //ie: If the value is not *exactly* the previous result.
                   ftol = fabs(last_best - func_vals[curr_min]);
                   last_best = func_vals[curr_min];
               }
 
               //Calculate the centroid of all points except the worst one.
               for(int i=0;i<DIM;++i) centroid[i] = 0.0;
               for(int i=0;i<=DIM;++i) if( i != curr_max ) for(int j=0;j<DIM;++j) centroid[j] += P_vecs[i*DIM+j];
               for(int i=0;i<DIM;++i) centroid[i] /= (T)DIM;

               //Compute the reflection point.
               for(int i=0;i<DIM;++i) reflec[i] = centroid[i] + alpha*(centroid[i] - P_vecs[curr_max*DIM+i]);

               //if   f(curr_min) <= f(RECR) < f(curr_max), then replace the worst point with reflec and return.
               temp_reflec = the_func(reflec);
               if(func_vals[curr_min] <= temp_reflec  && temp_reflec < func_vals[curr_scnd_max] ){
                   for(int i=0;i<DIM;++i) P_vecs[curr_max*DIM+i] = reflec[i];
                   func_vals[curr_max] = temp_reflec;
                   ++iteration;
                   return 0;
               }

               //The expanded point is a method to try improve the reflected point (if the refl. pnt is good to begin with.)
               //It adds another function evaluation and might be able to skipped for very costly functions.
               //if the reflected point is the best point yet, then compute the expanded point.
               if(temp_reflec < func_vals[curr_min]){
                   for(int i=0;i<DIM;++i) expand[i] = centroid[i] + gamma*(centroid[i] - P_vecs[curr_max*DIM+i]);
                   //Choose the point which is best at this point.
                   temp_expand = the_func(expand);
                   if(temp_expand < temp_reflec){
                       for(int i=0;i<DIM;++i) P_vecs[curr_max*DIM+i] = expand[i];
                       func_vals[curr_max] = temp_expand;
                       ++iteration;
                       return 0;
                   }else{
                       for(int i=0;i<DIM;++i) P_vecs[curr_max*DIM+i] = reflec[i];
                       func_vals[curr_max] = temp_reflec;
                       ++iteration;
                       return 0;
                   }
               }

               //Compute the contracted point. We will re-use the reflected point array for efficiency.
               for(int i=0;i<DIM;++i) reflec[i] = centroid[i] + rho*(centroid[i] - P_vecs[curr_max*DIM+i]);
               temp_reflec = the_func(reflec);
               if(temp_reflec < func_vals[curr_max]){
                   for(int i=0;i<DIM;++i) P_vecs[curr_max*DIM+i] = reflec[i];
                   func_vals[curr_max] = temp_reflec;
                   ++iteration;
                   return 0;
               }

               //If we have made it this far, we 'shrink' the simplex. We also recompute the function 
               // for the points we have shrunk (all but the curr_min).
               for(int i=0;i<=DIM;++i) if(i != curr_min){
                   for(int j=0;j<DIM;++j) P_vecs[i*DIM+j] = P_vecs[curr_min*DIM+j] + sigma*(P_vecs[i*DIM+j]-P_vecs[curr_min*DIM+j]);
                   func_vals[i] = the_func(&P_vecs[i*DIM]);
               }
               ++iteration;
               return 0;
           //Check why the scheme is refusing to compute the next iteration.
           }else{
               if(iteration > max_iters) return 1;
               if(ftol <= ftol_min)      return 2;
               return -1;
           }
        }

        //void init(T params[], T (&the_func_in)(T *)){
        void init(T params[], std::function<T (T *)> the_func_in){
            //Set the P0 vector as the params[] array.
            for(int i=0;i<DIM;++i) P_vecs[i] = params[i];

            //Set the_func reference.
            //the_func = &the_func_in;
            the_func = the_func_in;

            init_done = 1;
            return;
        }


        void get_params(T params[]){
            curr_min = 0;
            for(int i=1;i<=DIM;++i) if( i != curr_max ) if( i != curr_scnd_max ) if( func_vals[i] < func_vals[curr_min] ) curr_min = i;
            for(int i=0;i<DIM;++i) params[i] = P_vecs[curr_min*DIM+i];
            return;
        }

        //void get_all(T (&the_func)(T *), T params[], T &func_val){
        void get_all(std::function<T (T *)> the_func, T params[], T &func_val){
            curr_min = 0;
            for(int i=1;i<=DIM;++i) if( i != curr_max ) if( i != curr_scnd_max ) if( func_vals[i] < func_vals[curr_min] ) curr_min = i;
            for(int i=0;i<DIM;++i) params[i] = P_vecs[curr_min*DIM+i];
            func_val = the_func(params);
            return;
        }

        void printout(void){
            std::cout << "The function values are currently: ";
            for(int i=0;i<=DIM;++i) std::cout << func_vals[i] << "  ";
            std::cout << " at iteration " << iteration << std::endl;
            return;
        }
};



//----------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------- For_Each_In_Parallel -----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//Given two iterators and a std::function<void(...::iterator )>, this routine will perform the function on each element in parallel.
//
//Execution will block (ie. wait for all tasks to finish) before returning.

//template <class T>
//void For_Each_In_Parallel(T it, T end, std::function< void ( T )> Task){
//    if(! Task ){
//        FUNCWARN("Task passed in is not valid. Unable to perform any task. Continuing");
//        return;
//    }

template <class T, class Function>
void For_Each_In_Parallel(T it, T end, Function Task){

    //Launch the tasks, ensuring to keep the handles so we can tell when the task is completed.
    std::list< std::future<void> > handle_keeper;
    for( ; it != end; ++it){
        //NOTE: We run into problems if we do not OR std::launch::deferred on some platforms. From cpp-info:
        //
        // "...Throws std::system_error with error condition std::errc::resource_unavailable_try_again if 
        //    the launch policy is std::launch::async and the implementation is unable to start a new thread."
        //
        //  ..however, if we use  std::launch::async | std::launch::deferred  then we will mostly get lazy 
        //  evaluation (anecdotally.) Therefore, we launch as many non-deferrables as we can until an error 
        //  is thrown and THEN we switch to deferrables.

        decltype(  std::async( std::launch::async, Task, it )  )  newfuture;
        #ifdef YGOR_CUSTOM_ALGORITHMS_HC_ATTEMPT_ASYNCHRONOUS
            //try{
//                newfuture = std::async( std::launch::async, Task, it );
                newfuture = std::async( std::launch::async | std::launch::deferred, Task, it );

                handle_keeper.push_back( std::move( newfuture ) );

            //}catch(const std::system_error &e){
                //...
            //}catch(const std::exception &e){
                //...
            //}
        #else
            Task(it);    
//            newfuture = std::async( std::launch::async | std::launch::deferred, Task, it );
//            handle_keeper.push_back( std::async( std::launch::async | std::launch::deferred, Task, it ) );

        #endif

    };

    //Wait on the tasks in the same order we have launched them.
    for(auto & h_it : handle_keeper){
        h_it.get();
    }

    return;
}


//----------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Shuffle Algorithms ----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
template<typename T> void shuffle_list_consistently( std::list<T> &mylist ){
    //Uses a consistent scheme to shuffle. Repeated calls will return the same order.
    //
    //NOTE: I don't know how it is implemented, so do not depend on the consistency!
    std::vector<T> v( mylist.begin(), mylist.end() );
    std::random_shuffle( v.begin(), v.end() );
    mylist.assign( v.begin(), v.end() );
    return;
}

template<typename T> void shuffle_list_randomly( std::list<T> &mylist ){
    //Uses a ("truly random") random device to shuffle. The shuffling will be different upon repeated calls.
    std::random_device rd;
    auto shuffle_func = [&rd](ptrdiff_t i){ return rd()%i; };  //This *may* not be suitable for large lists. See rd.max() for upper bound.

    std::vector<T> v( mylist.begin(), mylist.end() );
    std::shuffle( v.begin(), v.end(), std::mt19937(std::random_device()()));
    mylist.assign( v.begin(), v.end() );
    return;
}

template<typename T> void shuffle_list( std::list<T> &mylist ){
    //Default 'shuffle' is a consistent one. It is not a true random shuffle, but is reproducable with repeated calls.
    //
    //NOTE: I don't know how it is implemented, so do not depend on the consistency!
    shuffle_list_consistently(mylist);
    return;
}



//----------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------- Nonparametric Regression ----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//These routines implement 'local linear nonparametric regression' or smoothing on arbitrary (ordered) 1D data. The 
// implementation is based on the chapter 'Nonparametric regression' from Larry Wasserman's "All of Nonparametric Statistics".
// Everything was kept outside of a class for sanity and flexibility. Down the road, higher-order p versions might be 
// implemented (currently only p=1 is supported).
//
//See the test files for some examples of how to use these functions.
//
//NOTES: 
//  1) These routines require data to be ordered (I think lowest-to-highest x, but it might not be a problem with high-to-low).
//  2) These routines require a decent amount of data to function. They are not worthwhile for small amounts of data (ie. a 
//     few points) and should not be used in those cases anyways!
//  3) This is Smoothing/REGRESSION. Do *NOT* extrapolate using these routines. You'd have to be crazy to think it would work!
//  4) A very bad integration is performed when determining k0 (used for computing 'c'). It compares ~ok with Wasserman's 
//     example numbers, but is nowhere near exact (or stable). It would be a good idea to take the confidence bars with a 
//     grain of salt. In fact, only first-order terms are used / normal errors are assumed / some assumptions are slightly
//     violated in most cases. This will require a redo if it becomes a problem!
//     Currently, the percent error in computed c's is asymptotically off by 5-10% in most cases (at 95% confidence interval).
//     It is safest to compute 95% or 99% confidence intervals.
//
//General Usage Idea:
//  Grab some data. Choose a starting h (smoothing parameter) to encompass about 100x the normal point spacing. Choose a scale
//  for h to be about 10-50x the normal point spacing. Perform Find_Optimal_H(...) routine to find the optimal smoothing h.
//  Now, use the Get_Smoothed_at_X(...) function to evaluate the smoothed function at whatever x is desired.
//
//  If desired, one can proceed to find X% confidence bars (for the fit - NOT for the data!). This is a fairly arduous task.
//  See the examples to see how it is done. Do not rely on the confidence bands for anything important. They are not perfect.
//
namespace NPRLL { //NPRLL - Non-Parametric Regression: Local Linear Smoothing.
    
    //------- Functions to get a single value of a distribution ---------
    double Get_Smoothed_at_X(double h, double x, const samples_1D<double> &in, bool *OK);
    double Get_Conf_at_X(double h, double conf_h, double c, double x, const samples_1D<double> &log_mse_data_in, const samples_1D<double> &data, bool *OK);
    
    //------- Routines which 'score' a given parameter in terms of some risk function. --------
    //These typically are very similar. The user won't need these unless performing a visual inspection.
    double Get_Cross_Validation_Leave_One_Out(double h, const samples_1D<double> &in);
    double Get_Cross_Validation_Generalized_CV(double h, const samples_1D<double> &in);
    
    //------- Routines to find or estimate parameters --------
    double Find_Optimal_H(double h_0, double h_scale, double cv_tol, long int N_iters, const samples_1D<double> &in, bool show_info, bool *OK);
    
    //This function computes an estimate of 'c' - the 'confidence' scale which is used in computing the confidence bands.
    double Find_C(double h, double confidence, const samples_1D<double> &in, bool *OK);
    
    
    //This function takes the data's optimal h and returns log(MSE) data which is itself suitable for regression.
    samples_1D<double> Log_of_MSE(double h, const samples_1D<double> &in, bool *OK);
    
    //------- Canned routines for more easily sampling the smoothed function/confidence bands. --------
    //Return constant-separation (dx) smoothed function data.
    samples_1D<double> Get_Smoothed_Evenly_Spaced(double h, double dx, const samples_1D<double> &in, bool *OK);
    //For the given input (x_i,y_i), generate a smoothed output (x_i,s_y_i) on the same x_i.
    samples_1D<double> Get_Smoothed_at_Xi(double h, const samples_1D<double> &in, bool *OK);
    
    //------- Convenience, all-in-one routines ---------
    samples_1D<double> Attempt_Auto_Smooth(const samples_1D<double> &in, bool *OK); //Gives back the data.

    //Does several things. Useful for exploring the data. Outputs a lot of stuff, though.
    void Attempt_Auto_Analysis(const samples_1D<double> &in, const std::string &Title = "", bool only_show_final = false, Plotter2 *plotter = nullptr, bool *OK = nullptr);

} //end of namespace NPRLL.


//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------- Architecture-independant Non-cryptographic Hashes --------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//For most purposes, the std::hash would be sufficient. When talking across a network (to same-endian machines) these will
// produce identical results, whereas the C++11 standard hashes are not required to.
//
//Currently using SpookyHash (in External). 
uint64_t Consistent_Hash_64(const std::string &in);

//MD5 hash. Implemented in External.
std::unique_ptr<uint8_t[]> MD5_Hash(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t bytecnt, std::string *hash);


//----------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Fitting Algorithms ----------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//These routines perform simple (somewhat constrained) numerical fitting to a function. Constraints, if needed must be 
// imposed manually in the objective function.
//
// These routines replace those in PolyCAS which are much slower and less focused on symbolic computation.
//
// NOTE: These routines require an objective function, NOT a cost function. The difference is as follows. For a general 
//       least-squares optimization problem, we have:
//           minimize [  sum_i (Datum_i - f(x,y,...;A,B,...))^2  ] via [A,B,...].
//       Here the COST function is sum_i (Datum_i - f(x,y,...;A,B,...))^2  and the OBJECTIVE function is f(x,y,...;A,B,...).
//
//       (Why the objective function and not the cost function? We can then control the norm internally, and offer LMS too.)
//
// NOTE: The number of dimensions specified by the user includes one for the Datum_i. So a scalar f(x) would be TWO 
//       dimensions, and f(x,y) would be THREE dimensions. The number of free parameters is not part of this dimensionality!
//
//  For fitting of type f(x) :
//      -If there are 2 columns, weighting is not considered.
//      -If there are 3 columns, it is assumed they are <Xi,Yi,Uncertainty in Yi>
//      -The functional form must ONLY accept parameters as arguments, and return a double.
//      -The input parameters are taken as initial parameters AND used to specify the number of fitting parameters to use.
//
namespace YGORFIT { 
    const uint32_t DIM2 = (1 << 0);  //2-dimensional (with or without user-provided Y_i uncertainties).
    const uint32_t DIM3 = (1 << 1);  //3-dimensional (with or without user-provided Z_i uncertainties).
  
    const uint32_t LSS =  (1 << 2);  //Least sum of squares.  The generally-recommended version.
    const uint32_t LMS =  (1 << 3);  //Least-median squares.  This version is more robust wrt outliers.

    const uint32_t NON_CONVERG_IS_ERR = (1 << 4); //Report failure if it didn't converge. Default is "no error".

    const uint32_t UNCERT_CAUCHY = (1 << 5); //Override uncertainties and use a Cauchy distribution instead (see source).
} //end of namespace YGORFIT.

//This is an internal function and should only be used if you know what you're doing (and how it will affect the reported
// stats and convergence).
std::tuple<std::list<double>,double,double,long int,double,double,double,std::list<double>>
Ygor_Fit_Driver(bool *wasOK,
    const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
    const std::list<std::list<double>> &data,   const std::list<double> &vars,   uint32_t fitflags,
    bool Verbose, double char_len,  int max_iters,  double ftol);


//Run-of-the-mill numerical "Least-squares" or "Method of least-squares" or "Least sum of squares".
std::tuple<std::list<double>,double,double,long int,double,double,double,std::list<double>>
Ygor_Fit_LSS(bool *wasOK,
         const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
         const std::list<std::list<double>> &data,  const std::list<double> &vars,     long int dim,
         bool Verbose = false,
         double char_len = 0.6,
         int max_iters = 1500,
         double ftol = 1E-6);

//Numerical "Least-median-of-squares".
std::tuple<std::list<double>,std::list<double>>
Ygor_Fit_LMS(bool *wasOK,
         const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
         const std::list<std::list<double>> &data,  const std::list<double> &vars,     long int dim,
         bool Verbose = false,
         double char_len = 0.6,
         int max_iters = 2500,
         double ftol = 1E-6);

//Performs bootstrap analysis and provides N estimates of the fit parameters. Use this to construct a 
// mean/median/uncertainty estimate/whatever. See nice writeup in A.J. Blanco's 2005 paper 'Dose-Volume
// modeling of parotid salivary function'
std::list<std::list<double>> Ygor_Fit_Bootstrap_Driver(bool *wasOK,
         const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
         const std::list<std::list<double>> &data,  const std::list<double> &vars,  uint32_t fitflags,
         long int N,  double char_len,  int max_iters,  double ftol);

std::list<std::list<double>> Ygor_Fit_Bootstrap_LSS(bool *wasOK,
         const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
         const std::list<std::list<double>> &data,  const std::list<double> &vars,  long int dim,
         long int N = 5000,
         double char_len = 0.6,
         int max_iters = 1500,
         double ftol = 1E-6);

std::list<std::list<double>> Ygor_Fit_Bootstrap_LMS(bool *wasOK,
         const std::function<double (const std::list<double> &X, const std::list<double> &Vars)> &f,
         const std::list<std::list<double>> &data,  const std::list<double> &vars,  long int dim,
         long int N = 5000,
         double char_len = 0.6,
         int max_iters = 2500,
         double ftol = 1E-6);


#endif 

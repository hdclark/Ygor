//Test_Math3.cc.

#include <iostream>
#include <cmath>
#include <list>
#include <functional>
#include <tuple>
#include <vector>
#include <random>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorStats.h"
#include "YgorPlot.h" //For visual comparison. Hard to test the output!

int main(int argc, char **argv){
   
    {
      FUNCINFO("---- Testing interpolation: lowest-x first orientation ----"); 
      class samples_1D<double> testarray( { vec2<double>(0.0, 0.0), 
                                            vec2<double>(1.0, 1.0),
                                            vec2<double>(2.0, 0.0),
                                            vec2<double>(3.0, 2.0) } );

      FUNCINFO("At x = -1E9, F = " << testarray.Interpolate_Linearly(-1E9 )[2] << "  should be 0.0");
      FUNCINFO("At x = 0.0,  F = " << testarray.Interpolate_Linearly( 0.0 )[2] << "  should be 0.0");
      FUNCINFO("At x = 0.5,  F = " << testarray.Interpolate_Linearly( 0.5 )[2] << "  should be 0.5");
      FUNCINFO("At x = 1.0,  F = " << testarray.Interpolate_Linearly( 1.0 )[2] << "  should be 1.0");
      FUNCINFO("At x = 1.5,  F = " << testarray.Interpolate_Linearly( 1.5 )[2] << "  should be 0.5");
      FUNCINFO("At x = 2.0,  F = " << testarray.Interpolate_Linearly( 2.0 )[2] << "  should be 0.0");
      FUNCINFO("At x = 2.25, F = " << testarray.Interpolate_Linearly( 2.25 )[2] << "  should be 0.5");
      FUNCINFO("At x = 3.0,  F = " << testarray.Interpolate_Linearly( 3.0 )[2] << "  should be 2.0");
      FUNCINFO("At x = 1E9,  F = " << testarray.Interpolate_Linearly( 1E9 )[2] << "  should be 0.0");
    }

    {
      FUNCINFO("---- Testing interpolation: lowest-x last orientation ----");
      //Note: the data should be sorted into lowest-x first order.
      class samples_1D<double> testarray( { vec2<double>(3.0, 2.0), 
                                            vec2<double>(2.0, 0.0),
                                            vec2<double>(1.0, 1.0),
                                            vec2<double>(0.0, 0.0) } );

      FUNCINFO("At x = -1E9, F = " << testarray.Interpolate_Linearly(-1E9 )[2] << "  should be 0.0");
      FUNCINFO("At x = 0.0,  F = " << testarray.Interpolate_Linearly( 0.0 )[2] << "  should be 0.0");
      FUNCINFO("At x = 0.5,  F = " << testarray.Interpolate_Linearly( 0.5 )[2] << "  should be 0.5");
      FUNCINFO("At x = 1.0,  F = " << testarray.Interpolate_Linearly( 1.0 )[2] << "  should be 1.0");
      FUNCINFO("At x = 1.5,  F = " << testarray.Interpolate_Linearly( 1.5 )[2] << "  should be 0.5");
      FUNCINFO("At x = 2.0,  F = " << testarray.Interpolate_Linearly( 2.0 )[2] << "  should be 0.0");
      FUNCINFO("At x = 2.25, F = " << testarray.Interpolate_Linearly( 2.25 )[2] << "  should be 0.5");
      FUNCINFO("At x = 3.0,  F = " << testarray.Interpolate_Linearly( 3.0 )[2] << "  should be 2.0");
      FUNCINFO("At x = 1E9,  F = " << testarray.Interpolate_Linearly( 1E9 )[2] << "  should be 0.0");
    }

    {
      FUNCINFO("---- Testing how interpolation handles uncertainties ----");
      //One assumes normal errs. Two makes no assumption.
      samples_1D<double> in1( { { 0.0,   0.1,   1.0,  0.1 },
                                { 0.3,   0.1,   1.2,  0.1 },    
                                { 0.6,   0.1,   1.5,  0.2 },    
                                { 1.0,  0.05,   1.9,  0.2 },    
                                { 1.1,  0.05,   2.0,  0.2 },    
                                { 1.2,  0.05,   2.0,  0.2 },    
                                { 1.9,   0.0,   1.8,  0.0 } } );
      samples_1D<double> in2(in1);    
      in2.uncertainties_known_to_be_independent_and_random = true;

      //Interpolate the samples at a fine level to see how the uncertainties compare.
      samples_1D<double> out1, out2;
      const bool skip_sort = true;
      for(double t = 0.0; t <= 2.0 ; t += 0.01){
          const auto F1 = in1.Interpolate_Linearly(t);
          const auto F2 = in2.Interpolate_Linearly(t);
          out1.push_back( { F1[0] + 0.0025, 0.0, F1[2], F1[3] }, skip_sort);
          out2.push_back( { F2[0] - 0.0025, 0.0, F2[2], F2[3] }, skip_sort);
      }    

      Plotter2 toplot;
      toplot.Set_Global_Title("Comparison of uncertainty propagation - assumption of normality vs. no assumption");
      toplot.Insert_samples_1D(out1, "No assumptions"); //, "linespoints");
      toplot.Insert_samples_1D(out2, "Assumed normality"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }




    {
    FUNCINFO("---- Integrating some rectangles. ----");
    class samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                  vec2<double>(1.0, 1.0) } );

    class samples_1D<double> g( { vec2<double>(0.0, 1.0),
                                  vec2<double>(1.0, 1.0) } );

    FUNCINFO("Integrating the overlap of f with g gives: " << f.Integrate_Overlap(g)[0] << "  should be 1.0");
    FUNCINFO("Integrating the overlap of g with f gives: " << g.Integrate_Overlap(f)[0] << "  should be 1.0");
    }


    {
    FUNCINFO("---- Integrating some rectangles - g defined backwards. ----");
    class samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                  vec2<double>(1.0, 1.0) } );

    class samples_1D<double> g( { vec2<double>(1.0, 1.0),
                                  vec2<double>(0.0, 1.0) } );

    FUNCINFO("Integrating the overlap of f with g gives: " << f.Integrate_Overlap(g)[0] << "  should be 1.0");
    FUNCINFO("Integrating the overlap of g with f gives: " << g.Integrate_Overlap(f)[0] << "  should be 1.0");
    }

    {
    FUNCINFO("---- Integrating some more exotic structures. ----");
    class samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                  vec2<double>(1.0, 1.0),
                                  vec2<double>(2.0, 1.0),
                                  vec2<double>(3.0, 1.0) } );

    class samples_1D<double> g( { vec2<double>(0.0, 1.0),
                                  vec2<double>(1.0, 1.0) } );

    FUNCINFO("Integrating the overlap of f with g gives: " << f.Integrate_Overlap(g)[0] << "  should be 1.0");
    FUNCINFO("Integrating the overlap of g with f gives: " << g.Integrate_Overlap(f)[0] << "  should be 1.0");
    }


    {
    FUNCINFO("---- Integrating some more exotic structures. ----");
    class samples_1D<double> f( { vec2<double>(0.0, 0.0),
                                  vec2<double>(1.0, 2.0),
                                  vec2<double>(2.0, 2.0),
                                  vec2<double>(3.0, 0.0) } );

    class samples_1D<double> g( { vec2<double>(0.0, 9.0),
                                  vec2<double>(1.0, 0.3),
                                  vec2<double>(2.0, 0.3),
                                  vec2<double>(3.0, 9.0) } );

    FUNCINFO("Integrating the overlap of f with g gives: " << f.Integrate_Overlap(g)[0] << " ");
    FUNCINFO("Integrating the overlap of g with f gives: " << g.Integrate_Overlap(f)[0] << " ");
    }


    {
    FUNCINFO("---- Integrating some sampled trig functions ----");
    class samples_1D<double> f, g;
    for(auto x = 0.0; x < 2.0*M_PI; x += 0.005){
        f.push_back( vec2<double>( x, sin(x) ) );
        g.push_back( vec2<double>( x, cos(x) ) );
    }

    FUNCINFO("Integrating the overlap of f with g gives: " << f.Integrate_Overlap(g)[0] << "  should be 0.0");
    FUNCINFO("Integrating the overlap of g with f gives: " << g.Integrate_Overlap(f)[0] << "  should be 0.0");
    }

    {
    FUNCINFO("---- Integrating some sampled trig functions ----");
    class samples_1D<double> f, g;
    for(auto x = 0.0; x < 2.0*M_PI; x += 0.005){
        f.push_back( vec2<double>( x, sin(x)*sin(x) + cos(x)*sin(x+0.1) ) );
        g.push_back( vec2<double>( x, sin(x+0.3) + cos(x/10.0) ) );
    }

    FUNCINFO("Integrating the overlap of f with g gives: " << f.Integrate_Overlap(g)[0] << "  should be 3.286585981855955");
    FUNCINFO("Integrating the overlap of g with f gives: " << g.Integrate_Overlap(f)[0] << "  should be 3.286585981855955");
    }

    {
      FUNCINFO("---- Integrating some sampled trig functions with normal uncertainties ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f, g;
      double dx = 0.005;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          f.push_back( { x, 0.5*dx, sin(x)*sin(x) + cos(x)*sin(x+0.1), x*0.1  }, inhibit_sort );
          g.push_back( { x, 0.5*dx, sin(x+0.3) + cos(x/10.0)         , x*0.05 }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = true;
      g.uncertainties_known_to_be_independent_and_random = true;

      Plotter2 toplot;
      toplot.Set_Global_Title("Integration with uncertainties f times g");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(g, "g"); //, "linespoints");
      //toplot.Plot();  //Looks cool!
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;

      const auto F = f.Integrate_Overlap(g);
      const auto G = g.Integrate_Overlap(f);

      FUNCINFO("Integrating the overlap of f with g gives: " << F[0] << " +- " << F[1] << "  should be 3.286585981855955");
      FUNCINFO("Integrating the overlap of g with f gives: " << G[0] << " +- " << G[1] << "  should be 3.286585981855955");
    }

    {
      FUNCINFO("---- Integrating some sampled trig functions with noassumptions uncertainties ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f, g;
      double dx = 0.005;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          f.push_back( { x, 0.5*dx, sin(x)*sin(x) + cos(x)*sin(x+0.1), x*0.1  }, inhibit_sort );
          g.push_back( { x, 0.5*dx, sin(x+0.3) + cos(x/10.0)         , x*0.05 }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = false;
      g.uncertainties_known_to_be_independent_and_random = false;

      Plotter2 toplot;
      toplot.Set_Global_Title("Integration with uncertainties f times g");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(g, "g"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;

      const auto F = f.Integrate_Overlap(g);
      const auto G = g.Integrate_Overlap(f);

      FUNCINFO("Integrating the overlap of f with g gives: " << F[0] << " +- " << F[1] << "  should be 3.286585981855955");
      FUNCINFO("Integrating the overlap of g with f gives: " << G[0] << " +- " << G[1] << "  should be 3.286585981855955");
    }
 

    {
      FUNCINFO("---- Testing the normalization routine. ----");
      class samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                    vec2<double>(1.0, 1.0),
                                    vec2<double>(2.0, 1.0),
                                    vec2<double>(3.0, 1.0) } );

      FUNCINFO("Initial samples: ");
      for(const auto &P : f.samples) std::cout << P[0] << " " << P[2] << ", ";
      std::cout << std::endl;
      FUNCINFO(" ..and the self overlap is: " << f.Integrate_Overlap(f)[0] );

      f.Normalize_wrt_Self_Overlap();

      FUNCINFO("Normalized: ");
      for(const auto &P : f.samples) std::cout << P[0] << " " << P[2] << ", ";
      std::cout << std::endl;
      FUNCINFO(" ..and the self overlap is: " << f.Integrate_Overlap(f)[0] );
    }
    { 
      FUNCINFO("---- Testing the normalization routine. ----");
      class samples_1D<double> f( { { -1.0, 1.0, 0.0, 1.0 },
                                    {  0.0, 0.5, 1.0, 0.3 },
                                    {  1.0, 2.5, 2.0, 1.3 },
                                    {  2.0, 0.1, 4.0, 0.1 } } );
      
      FUNCINFO("Initial samples: "); 
      for(const auto &P : f.samples) std::cout << P[0] << " " << P[2] << ", ";
      std::cout << std::endl;
      FUNCINFO(" ..and the self overlap is: " << f.Integrate_Overlap(f)[0] );
      FUNCINFO(" (should be 12.0)"); 
      f.Normalize_wrt_Self_Overlap();
      
      FUNCINFO("Normalized: ");
      for(const auto &P : f.samples) std::cout << P[0] << " " << P[2] << ", ";
      std::cout << std::endl;
      FUNCINFO(" ..and the self overlap is: " << f.Integrate_Overlap(f)[0] );
    }

    {
      FUNCINFO("---- Testing the summation routine. ----");
      samples_1D<double> f( { vec2<double>(0.0, 1.0),
                              vec2<double>(1.0, 1.0),
                              vec2<double>(2.0, 1.0),
                              vec2<double>(3.0, 1.0) } );

      samples_1D<double> g( { vec2<double>(0.0, 1.0),
                              vec2<double>(1.0, 1.0),
                              vec2<double>(2.0, 1.0),
                              vec2<double>(3.0, 1.0) } );

      FUNCINFO("Summing two sampled functions. Should have four terms which are all 2.0");
      f = f.Sum_With(g);
      for(auto it = f.samples.begin(); it != f.samples.end(); ++it){
          std::cout << (*it)[0] << "," << (*it)[2] << "  ";
      }
      std::cout << std::endl;
    }
    {
      FUNCINFO("---- Testing the summation routine. ----");
      samples_1D<double> f( { vec2<double>(0.00, 0.00),
                              vec2<double>(0.25, 0.25),
                              vec2<double>(0.50, 0.50),
                              vec2<double>(0.75, 0.75) } );

      samples_1D<double> g( { vec2<double>(0.0, 1.0),
                              vec2<double>(1.0, 1.0),
                              vec2<double>(2.0, 1.0),
                              vec2<double>(3.0, 1.0) } );

      FUNCINFO("Summing two sampled functions. Should have seven terms ");
      f = f.Sum_With(g);
      for(auto it = f.samples.begin(); it != f.samples.end(); ++it){
          std::cout << (*it)[0] << "," << (*it)[2] << "  ";
      }
      std::cout << std::endl;
    }

    {
      FUNCINFO("---- Testing the summation routine's uncertainty handling. ----");
      samples_1D<double> f( { { 0.0, 0.1, 1.0, 0.1 },
                              { 1.0, 0.2, 1.0, 0.2 },
                              { 2.0, 0.4, 1.0, 0.3 },
                              { 3.0, 0.6, 1.0, 0.4 } } );

      samples_1D<double> g( { { 0.0, 0.1, 1.0, 0.1 },
                              { 1.0, 0.2, 1.0, 0.2 },
                              { 2.0, 0.2, 1.0, 0.2 },
                              { 3.0, 0.1, 1.0, 0.1 } } );

      samples_1D<double> h = f.Sum_With(g);

      //Interpolate the samples at a fine level to see how the uncertainties compare.
      samples_1D<double> h_fine;
      const bool skip_sort = true;
      for(double t = -1.0; t <= 4.0 ; t += 0.01){
          h_fine.push_back(h.Interpolate_Linearly(t), skip_sort);
      }

      Plotter2 toplot;
      toplot.Set_Global_Title("Summation uncertainty propagation");
      toplot.Insert_samples_1D(f, "f");
      toplot.Insert_samples_1D(g, "g");;
      toplot.Insert_samples_1D(h_fine, "h = f + g (fine)");;
      toplot.Insert_samples_1D(h, "h = f + g");;
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }

    {
      FUNCINFO("---- Testing the summation routine's uncertainty handling (normality assumption). ----");
      samples_1D<double> f( { { 0.0, 0.1, 1.0, 0.1 },
                              { 1.0, 0.2, 1.0, 0.2 },
                              { 2.0, 0.4, 1.0, 0.3 },
                              { 3.0, 0.6, 1.0, 0.4 } } );
      f.uncertainties_known_to_be_independent_and_random = true;

      samples_1D<double> g( { { 0.0, 0.1, 1.0, 0.1 },
                              { 1.0, 0.2, 1.0, 0.2 },
                              { 2.0, 0.2, 1.0, 0.2 },
                              { 3.0, 0.1, 1.0, 0.1 } } );
      g.uncertainties_known_to_be_independent_and_random = true;

      samples_1D<double> h = f.Sum_With(g);

      //Interpolate the samples at a fine level to see how the uncertainties compare.
      samples_1D<double> h_fine;
      const bool skip_sort = true;
      for(double t = -1.0; t <= 4.0 ; t += 0.01){
          h_fine.push_back(h.Interpolate_Linearly(t), skip_sort);
      }

      Plotter2 toplot;
      toplot.Set_Global_Title("Summation uncertainty propagation");
      toplot.Insert_samples_1D(f, "f");
      toplot.Insert_samples_1D(g, "g");;
      toplot.Insert_samples_1D(h_fine, "h = f + g (fine)");;
      toplot.Insert_samples_1D(h, "h = f + g");;
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }

    {
      FUNCINFO("---- Testing the summation routine's uncertainty handling (normality assumption). ----");
      samples_1D<double> f( { { 0.5, 0.0, 1.0, 0.0 },
                              { 1.5, 0.0, 1.0, 0.0 },
                              { 2.5, 0.0, 1.0, 0.0 },
                              { 3.5, 0.0, 1.0, 0.0 } } );
      f.uncertainties_known_to_be_independent_and_random = true;

      samples_1D<double> g( { { 0.0, 0.1, 1.0, 0.1 },
                              { 1.0, 0.1, 1.0, 0.1 },
                              { 2.0, 0.2, 1.0, 0.2 },
                              { 3.0, 0.2, 1.0, 0.2 } } );
      g.uncertainties_known_to_be_independent_and_random = true;

      samples_1D<double> h = f.Sum_With(g);

      //Interpolate the samples at a fine level to see how the uncertainties compare.
      samples_1D<double> h_fine;
      const bool skip_sort = true;
      for(double t = -1.0; t <= 4.0 ; t += 0.01){
          h_fine.push_back(h.Interpolate_Linearly(t), skip_sort);
      }

      Plotter2 toplot;
      toplot.Set_Global_Title("Summation uncertainty propagation");
      toplot.Insert_samples_1D(f, "f");
      toplot.Insert_samples_1D(g, "g");;
      toplot.Insert_samples_1D(h_fine, "h = f + g (fine)");;
      toplot.Insert_samples_1D(h, "h = f + g");;
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }

    {
      FUNCINFO("---- Testing the summation routine's uncertainty handling (normality assumption). ----");
      samples_1D<double> f( { { 0.5, 0.0, 1.0, 0.0 },
                              { 1.5, 0.0, 1.0, 0.0 },
                              { 2.5, 0.0, 1.0, 0.0 },
                              { 3.5, 0.0, 1.0, 0.0 } } );
      f.uncertainties_known_to_be_independent_and_random = false;

      samples_1D<double> g( { { 0.0, 0.1, 1.0, 0.1 },
                              { 1.0, 0.1, 1.0, 0.1 },
                              { 2.0, 0.2, 1.0, 0.2 },
                              { 3.0, 0.2, 1.0, 0.2 } } );
      g.uncertainties_known_to_be_independent_and_random = false;

      samples_1D<double> h = f.Sum_With(g);

      //Interpolate the samples at a fine level to see how the uncertainties compare.
      samples_1D<double> h_fine;
      const bool skip_sort = true;
      for(double t = -1.0; t <= 4.0 ; t += 0.01){
          h_fine.push_back(h.Interpolate_Linearly(t), skip_sort);
      }

      Plotter2 toplot;
      toplot.Set_Global_Title("Summation uncertainty propagation");
      toplot.Insert_samples_1D(f, "f");
      toplot.Insert_samples_1D(g, "g");;
      toplot.Insert_samples_1D(h_fine, "h = f + g (fine)");;
      toplot.Insert_samples_1D(h, "h = f + g");;
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }


    {
      FUNCINFO("---- Testing the mean and average routines. ----");
      samples_1D<double> f( { { 0.5, 0.0, 1.1, 0.0 },
                              { 1.0, 0.0, 1.5, 0.0 },
                              { 1.1, 0.0, 1.2, 0.0 },
                              { 3.5, 0.0, 1.3, 0.0 } } );
      const auto meanx = f.Mean_x();
      const auto avgx  = f.Average_x();
      const auto meany = f.Mean_y();
      const auto avgy  = f.Average_y();

      std::cout << "f = " << f << std::endl;

      std::cout << " mean_x = " << meanx[0] << " +- " << meanx[1] << std::endl;
      std::cout << " avg_x  = " << avgx[0]  << " +- " << avgx[1]  << std::endl;

      std::cout << " mean_y = " << meany[0] << " +- " << meany[1] << std::endl;
      std::cout << " avg_y  = " << avgy[0]  << " +- " << avgy[1]  << std::endl;
    }

    {
      FUNCINFO("---- Testing the Integrate_Over_Kernel_exp() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f, g;
      double dx = 0.01;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.5*dx, f_at_x, f_at_x*0.1  }, inhibit_sort );
          g.push_back( { x, 0.5*dx, f_at_x, f_at_x*0.1  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = true;
      g.uncertainties_known_to_be_independent_and_random = false;

      Plotter2 toplot;
      toplot.Set_Global_Title("Integration over an exponential kernel");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;

      const auto F = f.Integrate_Over_Kernel_exp(0.0, 2.0*M_PI, { -0.5, 0.1 }, { 0.0, 0.01 });
      FUNCINFO("Integrating over an exp kernel gives: " << F[0] << " +- " << F[1] << " should be 3.935600016899443");
      const auto G = g.Integrate_Over_Kernel_exp(0.0, 2.0*M_PI, { -0.5, 0.1 }, { 0.0, 0.01 });
      FUNCINFO("Integrating over an exp kernel gives: " << G[0] << " +- " << G[1] << " should be 3.935600016899443");
    }

    { 
      FUNCINFO("---- Testing the Integrate_Over_Kernel_exp() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f,g;
      double dx = 0.01; 
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.5*dx, f_at_x, 0.1  }, inhibit_sort );
          g.push_back( { x, 0.5*dx, f_at_x, 0.1  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = true;
      g.uncertainties_known_to_be_independent_and_random = false;
      
      Plotter2 toplot;
      toplot.Set_Global_Title("Integration over an exponential kernel");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
      
      const auto F = f.Integrate_Over_Kernel_exp(0.0, 2.0*M_PI, { 2.3, 0.1 }, { 1.0, 0.01 });
      FUNCINFO("Integrating over an exp kernel gives: " << F[0] << " +- " << F[1] << " should be 6383992.946870714");
      const auto G = g.Integrate_Over_Kernel_exp(0.0, 2.0*M_PI, { 2.3, 0.1 }, { 1.0, 0.01 });
      FUNCINFO("Integrating over an exp kernel gives: " << G[0] << " +- " << G[1] << " should be 6383992.946870714");
    }


    {
      FUNCINFO("---- Testing the Weighted_Mean_x routine. ----");
      class samples_1D<double> dat({ { 11.0, 1.0, 10.0, 3.0 },
                                     { 12.0, 1.0, 11.0, 1.0 },
                                     { 10.0, 3.0, 12.0, 1.0 } });
      dat.uncertainties_known_to_be_independent_and_random = true;

      const auto wmx = dat.Weighted_Mean_x();
      const auto wmy = dat.Weighted_Mean_y();
      FUNCINFO("Weighted mean on x = " << wmx[0] << " +- " << wmx[1] << " should be 11.4211 +- 0.688247");
      FUNCINFO("Weighted mean on y = " << wmy[0] << " +- " << wmy[1] << " should be 11.4211 +- 0.688247");
    }

    {
      FUNCINFO("---- Testing the Aggregate_Equal_Sized_Bins_Weighted_Mean() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f;
      double dx = 0.1;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.5*dx, f_at_x, 0.1*f_at_x  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = true;

      const bool as_histogram = true;
      const auto fhist = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10,  as_histogram);
      const auto fbins = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10, !as_histogram);

      Plotter2 toplot;
      toplot.Set_Global_Title("Aggregate_Equal_Sized_Bins_Weighted_Mean");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(fhist, "fhist", "lines");
      toplot.Insert_samples_1D(fbins, "fbins"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }
    {
      FUNCINFO("---- Testing the Aggregate_Equal_Sized_Bins_Weighted_Mean() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f;
      double dx = 0.1;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.5*dx, f_at_x, 0.1*f_at_x  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = false;

      const bool as_histogram = true;
      const auto fhist = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10,  as_histogram);
      const auto fbins = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10, !as_histogram);

      Plotter2 toplot;
      toplot.Set_Global_Title("Aggregate_Equal_Sized_Bins_Weighted_Mean");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(fhist, "fhist", "lines");
      toplot.Insert_samples_1D(fbins, "fbins"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }

    {
      FUNCINFO("---- Testing the Aggregate_Equal_Sized_Bins_Weighted_Mean() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f;
      double dx = 0.1;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.0, f_at_x, 0.0  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = true;

      const bool as_histogram = true;
      const auto fhist = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10,  as_histogram);
      const auto fbins = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10, !as_histogram);

      Plotter2 toplot;
      toplot.Set_Global_Title("Aggregate_Equal_Sized_Bins_Weighted_Mean");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(fhist, "fhist", "lines");
      toplot.Insert_samples_1D(fbins, "fbins"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }

    {
      FUNCINFO("---- Testing the Aggregate_Equal_Sized_Bins_Weighted_Mean() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f;
      double dx = 0.1;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.0, f_at_x, 1.0  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = true;

      const bool as_histogram = true;
      const auto fhist = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10,  as_histogram);
      const auto fbins = f.Aggregate_Equal_Sized_Bins_Weighted_Mean(10, !as_histogram);

      Plotter2 toplot;
      toplot.Set_Global_Title("Aggregate_Equal_Sized_Bins_Weighted_Mean");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(fhist, "fhist", "lines");
      toplot.Insert_samples_1D(fbins, "fbins"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }

    {
      FUNCINFO("---- Testing the Aggregate_Equal_Datum_Bins_Weighted_Mean() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f;
      double dx = 0.1;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.1*x, f_at_x, 0.2*f_at_x  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = true;

      const auto fbins = f.Aggregate_Equal_Datum_Bins_Weighted_Mean(10);

      Plotter2 toplot;
      toplot.Set_Global_Title("Aggregate_Equal_Datum_Bins_Weighted_Mean");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(fbins, "fbins"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }

    {
      FUNCINFO("---- Testing the Aggregate_Equal_Datum_Bins_Weighted_Mean() routine. ----");
      const bool inhibit_sort = true;
      class samples_1D<double> f;
      double dx = 0.1;
      for(auto x = 0.0; x < 2.0*M_PI; x += dx){
          const auto f_at_x = sin(x)*sin(x) + cos(x)*sin(x+0.1) + sin(x+0.3) + cos(x/10.0);
          f.push_back( { x, 0.3*dx, f_at_x, 0.1*f_at_x  }, inhibit_sort );
      }
      f.uncertainties_known_to_be_independent_and_random = false;

      const auto fbins = f.Aggregate_Equal_Datum_Bins_Weighted_Mean(10);

      Plotter2 toplot;
      toplot.Set_Global_Title("Aggregate_Equal_Datum_Bins_Weighted_Mean");
      toplot.Insert_samples_1D(f, "f"); //, "linespoints");
      toplot.Insert_samples_1D(fbins, "fbins"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }


    //Test the spearman rank correlation.
    {
      FUNCINFO("---- Testing the Spearmans_Rank_Correlation_Coefficient(). ----");
      class samples_1D<double> dat({ vec2<double>( 0.8,  1.0),
                                     vec2<double>( 1.2,  2.0),
                                     vec2<double>( 1.2,  3.0),
                                     vec2<double>( 2.3,  4.0),
                                     vec2<double>(18.0,  5.0) });

      const auto stats = dat.Spearmans_Rank_Correlation_Coefficient();
      FUNCINFO("Spearman's rank correlation coefficient for this data is " << std::get<0>(stats)
            << ", The number of samples = " << std::get<1>(stats) 
            << ", z-value = " << std::get<2>(stats) 
            << ", and t-value = " << std::get<3>(stats));
      FUNCINFO(" Should be 0.9747, 5, ???, ???");
    }

    {
      FUNCINFO("---- Testing the Spearmans_Rank_Correlation_Coefficient(). ----");
      class samples_1D<double> dat({ vec2<double>(106.0,  7.0),
                                     vec2<double>( 86.0,  0.0),
                                     vec2<double>(100.0, 27.0),
                                     vec2<double>(101.0, 50.0),
                                     vec2<double>( 99.0, 28.0),
                                     vec2<double>(103.0, 29.0),
                                     vec2<double>( 97.0, 20.0),
                                     vec2<double>(113.0, 12.0),
                                     vec2<double>(112.0,  6.0),
                                     vec2<double>(110.0, 17.0) });

      const auto stats = dat.Spearmans_Rank_Correlation_Coefficient();
      FUNCINFO("Spearman's rank correlation coefficient for this data is " << std::get<0>(stats)
            << ", The number of samples = " << std::get<1>(stats) 
            << ", z-value = " << std::get<2>(stats)  
            << ", and t-value = " << std::get<3>(stats));
      FUNCINFO(" Should be -0.1758, 10, ???, -0.5");
    }

    {
      FUNCINFO("---- Testing the Linear_Least_Squares_Regression(). ----");
      samples_1D<double> dat({ vec2<double>( 1.00, 2.0000),
                               vec2<double>( 1.05, 3.0500),
                               vec2<double>( 1.10, 10.600),
                               vec2<double>( 1.40, 1000.0) });
      auto res = dat.Linear_Least_Squares_Regression();
      decltype(res) actual;
      actual.slope = 2699.98;
      actual.sigma_slope = 431.2;
      actual.intercept = -2817.32;
      actual.sigma_intercept = 495.1;
      actual.N = 4;
      actual.dof = 2;
      actual.sigma_f = 134.2215;
      //actual.sum_sq_res = ???;
      actual.covariance = 65.39016;
      actual.lin_corr = 0.975;
      actual.tvalue = 6.261;
      actual.pvalue = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(actual.lin_corr,actual.N);

      std::cout << res.comparison_table(actual);
    }
    {
      FUNCINFO("---- Testing the Weighted Linear_Least_Squares_Regression(). ----");
      std::cout << "Case 1. Zero uncertainties, Zero sigma_x_i." << std::endl;
      samples_1D<double> dat({ { 1.00, 0.00, 2.0000, 0.00 },
                               { 1.05, 0.00, 3.0500, 0.00 },
                               { 1.10, 0.00, 10.600, 0.00 },
                               { 1.40, 0.00, 1000.0, 0.00 } });
      auto res = dat.Weighted_Linear_Least_Squares_Regression();
      decltype(res) actual;
      actual.slope = 2699.98;
      actual.sigma_slope = 0.0;
      actual.intercept = -2817.32;
      actual.sigma_intercept = 0.0;
      actual.N = 4;
      actual.dof = 2;
      actual.sigma_f = 134.2215;
      //actual.chi-square = ???;
      actual.covariance = 65.39016;
      actual.lin_corr = 0.975;
      //actual.qvalue = ???;
      
      std::cout << res.comparison_table(actual);
    }
    {
      FUNCINFO("---- Testing the Weighted Linear_Least_Squares_Regression(). ----");
      std::cout << "Case 2. Equal, non-zero sigma_f_i, Zero sigma_x_i." << std::endl;
      samples_1D<double> dat({ { 1.00, 0.00, 2.0000, 1.00 },
                               { 1.05, 0.00, 3.0500, 1.00 },
                               { 1.10, 0.00, 10.600, 1.00 },
                               { 1.40, 0.00, 1000.0, 1.00 } });
      auto res = dat.Weighted_Linear_Least_Squares_Regression();
      decltype(res) actual;
      actual.slope = 2699.98;
      actual.sigma_slope = 3.21288;
      actual.intercept = -2817.32;
      actual.sigma_intercept = 3.68869;
      actual.N = 4;
      actual.dof = 2;
      actual.sigma_f = 134.2215;
      //actual.chi-square = ???;
      actual.covariance = 65.39016;
      actual.lin_corr = 0.975;
      //actual.qvalue = ???;

      std::cout << res.comparison_table(actual);
    }
    {
      FUNCINFO("---- Testing the Weighted Linear_Least_Squares_Regression(). ----");
      std::cout << "Case 3. Equal, non-zero sigma_f_i, equal to the computed (average) sigma_f, Zero sigma_x_i." << std::endl;
      samples_1D<double> dat({ { 1.00, 0.00, 2.0000, 134.221 },
                               { 1.05, 0.00, 3.0500, 134.221 },
                               { 1.10, 0.00, 10.600, 134.221 },
                               { 1.40, 0.00, 1000.0, 134.221 } });
      auto res = dat.Weighted_Linear_Least_Squares_Regression();
      decltype(res) actual;
      actual.slope = 2699.98;
      actual.sigma_slope = 431.2;
      actual.intercept = -2817.32;
      actual.sigma_intercept = 495.1;
      actual.N = 4;
      actual.dof = 2;
      actual.sigma_f = 134.2215;
      //actual.chi-square = ???;
      actual.covariance = 65.39016;
      actual.lin_corr = 0.975;
      //actual.qvalue = ???;

      std::cout << res.comparison_table(actual);
    }
    {
      FUNCINFO("---- Testing the Weighted Linear_Least_Squares_Regression(). ----");
      std::cout << "Case 4. Equal, non-zero sigma_f_i, equal to the computed (average) sigma_f, Non-equal, non-zero sigma_x_i." << std::endl;
      samples_1D<double> dat({ { 1.00, 0.10, 2.0000, 134.221 },
                               { 1.05, 0.20, 3.0500, 134.221 },
                               { 1.10, 0.20, 10.600, 134.221 },
                               { 1.40, 0.90, 1000.0, 134.221 } });
      auto res = dat.Weighted_Linear_Least_Squares_Regression();
      decltype(res) actual;
      //actual.slope = ???;
      //actual.sigma_slope = ???;
      //actual.intercept = ???;
      //actual.sigma_intercept = ???;
      actual.N = 4;
      actual.dof = 2;
      //actual.sigma_f = ???;
      //actual.chi-square = ???;
      //actual.covariance = ???;
      //actual.lin_corr = ???;
      //actual.qvalue = ???;

      std::cout << res.comparison_table(actual);
    }
    {
      FUNCINFO("---- Testing iterated Weighted Linear_Least_Squares_Regression(). ----");
      samples_1D<double> dat({ { 1.00, 0.10, 2.0000, 134.221 },
                               { 1.05, 0.20, 3.0500, 134.221 },
                               { 1.10, 0.20, 10.600, 134.221 },
                               { 1.40, 0.90, 1000.0, 134.221 } });
      double slope_guess = (1000.0 - 2.0)/(1.40 - 1.00); //First and last points. 
      bool wasOK;
      for(int i = 0; i < 10; ++i){
          auto res = dat.Weighted_Linear_Least_Squares_Regression(&wasOK, &slope_guess);
          if(!wasOK) FUNCERR("Failed to compute weighted linear regression");
          decltype(res) actual;
          std::cout << res.display_table() << std::endl;
          slope_guess = res.slope; //Use the last computed slope as the next guess.
      }
    }
    {
      FUNCINFO("---- Testing the histogramming routines. ----");
      
      //Create a list of numbers sampled from some distribution.
      std::list<double> nums;

      std::random_device rd;
      std::default_random_engine generator(rd());
      std::normal_distribution<double> distribution(5.0,2.0); //Gaussian/normal.
      for(long int i=0; i<10000; ++i){
          double anumber = distribution(generator);
          nums.push_back(anumber);
      }

      const long int bins = 50;
      const bool withbins = true;
      const auto hista = Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(nums, bins,  withbins);
      const auto histb = Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(nums, bins, !withbins);

      Plotter2 toplot;
      toplot.Set_Global_Title("Bag_of_numbers_to_N_equal_bin_samples_1D_histogram");
      toplot.Insert_samples_1D(hista, "with bins", "linespoints");
      toplot.Insert_samples_1D(histb, "without bins"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }


    {
      FUNCINFO("---- Testing the histogramming routines. ----");

      //std::random_device rd;
      std::default_random_engine gen; //(rd());
      std::normal_distribution<double>  xdist( 5.0,2.0); //Gaussian/normal. {center,width}.
      std::normal_distribution<double> sxdist( 0.0,1.0); //Gaussian/normal. {center,width}.
      std::normal_distribution<double>  ydist(50.0,8.0); //Gaussian/normal. {center,width}.
      std::normal_distribution<double> sydist( 0.0,5.0); //Gaussian/normal. {center,width}.

      const bool inhibit_sort = true;
      samples_1D<double> adist;
      for(long int i=0; i<20000; ++i){
          adist.push_back({ xdist(gen), sxdist(gen), ydist(gen), sydist(gen) }, inhibit_sort);
      }
      adist.stable_sort();

      const long int bins = 50;
      const bool withbins = true;
      const auto hista = adist.Histogram_Equal_Sized_Bins(bins,  withbins);
      const auto histb = adist.Histogram_Equal_Sized_Bins(bins, !withbins);

      Plotter2 toplot;
      toplot.Set_Global_Title("Histogram_Equal_Sized_Bins");
      toplot.Insert_samples_1D(hista, "", "linespoints");
      toplot.Insert_samples_1D(histb, "histogram"); //, "linespoints");
      //toplot.Plot();
      //toplot.Plot_as_PDF("/tmp/out.pdf");
      //std::cout << toplot.Dump_as_String() << std::endl;
    }


 
    return 0;
}

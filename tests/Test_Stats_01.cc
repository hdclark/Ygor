#include <iostream>
#include <cmath>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorStats.h"


int main(int argc, char **argv){

    { 
      double meanA = 8.605186092;
      double varA  = 0.801379847;
      double numA  = 102;
  
      double meanB = 8.132293417;
      double varB  = 0.8604915825;
      double numB  = 102;
  
      std::cout << "P = ";
      std::cout << Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA,varA,numA,meanB,varB,numB);
      std::cout << std::endl;
  
      varA  = std::sqrt(varA);
      std::cout << "P = ";
      std::cout << Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA,varA,numA,meanB,varB,numB);
      std::cout << std::endl;
  
      meanA = meanB;
      varA  = varB;
      std::cout << "P = ";
      std::cout << Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA,varA,numA,meanB,varB,numB); 
      std::cout << std::endl;
  
      meanA = meanB + 20.0;
      varA  = varB;
      std::cout << "P = ";
      std::cout << Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA,varA,numA,meanB,varB,numB);
      std::cout << std::endl;
  
      meanA = meanB + 20.0;
      varA  = 2.0*varB;
      std::cout << "P = ";
      std::cout << Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA,varA,numA,meanB,varB,numB);
      std::cout << std::endl;
    } 

    {
      YLOGINFO("---- Comparing P_From_Pearsons_Linear_Correlation_Coeff_2Tail() to table values ----");
      double pval;
      
      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.3, 6.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.56");

      //pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.3, 100.0);
      //YLOGINFO("P-value = " << pval << "   <--- should be close to 0.14");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.8, 3.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.41");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.1, 12.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.76");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(1.0, 5.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.0");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(1.0, 35.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.0");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.0, 5.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 1.0");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.0, 35.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 1.0");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.4, 19.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.09");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.6, 25.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.002");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.5, 40.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.001");

      pval = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.7, 12.0);
      YLOGINFO("P-value = " << pval << "   <--- should be close to 0.011");



    }

    return 0;
}

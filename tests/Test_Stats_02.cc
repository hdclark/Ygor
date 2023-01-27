
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <functional>
#include <sstream>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorStats.h"

int main(int argc, char **argv){

    samples_1D<double> parotid, masseter, pharynx;

    {
      std::stringstream ss;
      ss << "(samples_1D. normalityassumed= 0 num_samples= 160 0 0 1.01562 0 3.839 0 1.06381 0 7.678 0 1.05854 0 11.517 0 1.06484 0 15.356 0 1.08508 0 19.195 0 1.12542 0 23.034 0 1.14002 0 26.873 0 1.18045 0 30.712 0 1.22493 0 34.552 0 1.3279 0 38.391 0 1.45237 0 42.23 0 1.5204 0 46.069 0 1.59349 0 49.908 0 1.62751 0 53.747 0 1.64555 0 57.586 0 1.63464 0 61.425 0 1.58377 0 65.264 0 1.54239 0 69.103 0 1.47915 0 72.943 0 1.42672 0 76.782 0 1.34445 0 80.621 0 1.24747 0 84.46 0 1.14485 0 88.299 0 1.09533 0 92.138 0 0.995995 0 95.977 0 0.887188 0 99.816 0 0.906273 0 103.655 0 0.900058 0 107.494 0 0.910562 0 111.334 0 0.939718 0 115.173 0 0.954063 0 119.012 0 0.943601 0 122.851 0 0.922706 0 126.69 0 0.904143 0 130.529 0 0.904529 0 134.368 0 0.907703 0 138.207 0 0.897426 0 142.046 0 0.875949 0 145.885 0 0.841873 0 149.725 0 0.837786 0 153.564 0 0.880344 0 157.403 0 0.884444 0 161.242 0 0.88573 0 165.081 0 0.913688 0 168.92 0 0.951541 0 172.759 0 0.962066 0 176.598 0 1.0138 0 180.437 0 1.04106 0 184.276 0 1.05357 0 188.115 0 1.08916 0 191.955 0 1.10095 0 195.794 0 1.10983 0 199.633 0 1.11512 0 203.472 0 1.12667 0 207.311 0 1.13829 0 211.15 0 1.1238 0 214.989 0 1.11381 0 218.828 0 1.09584 0 222.667 0 1.08974 0 226.506 0 1.06554 0 230.346 0 1.07161 0 234.185 0 1.03179 0 238.024 0 1.02063 0 241.863 0 1.01264 0 245.702 0 0.988766 0 249.541 0 0.963231 0 253.38 0 0.952252 0 257.219 0 0.955826 0 261.058 0 0.919561 0 264.897 0 0.875694 0 268.737 0 0.885181 0 272.576 0 1.06365 0 276.415 0 1.07132 0 280.254 0 1.08476 0 284.093 0 1.09875 0 287.932 0 1.13463 0 291.771 0 1.13676 0 295.61 0 1.16989 0 299.449 0 1.16298 0 303.288 0 1.17537 0 307.128 0 1.1725 0 310.967 0 1.16526 0 314.806 0 1.17836 0 318.645 0 1.16088 0 322.484 0 1.18485 0 326.323 0 1.23181 0 330.162 0 1.06401 0 334.001 0 1.0744 0 337.84 0 1.07711 0 341.679 0 1.077 0 345.518 0 1.05201 0 349.358 0 1.05633 0 353.197 0 1.02654 0 357.036 0 1.0366 0 360.875 0 1.03006 0 364.714 0 1.03278 0 368.553 0 1.03018 0 372.392 0 1.01966 0 376.231 0 1.07118 0 380.07 0 1.05291 0 383.909 0 1.04731 0 387.749 0 1.09475 0 391.588 0 1.11594 0 395.427 0 1.11108 0 399.266 0 1.12197 0 403.105 0 1.14387 0 406.944 0 1.14102 0 410.783 0 1.16803 0 414.622 0 1.23655 0 418.461 0 1.24317 0 422.3 0 1.2705 0 426.14 0 1.27493 0 429.979 0 1.29292 0 433.818 0 1.27613 0 437.657 0 1.2733 0 441.496 0 1.25583 0 445.335 0 1.32791 0 449.174 0 1.32334 0 453.013 0 1.32701 0 456.852 0 1.33017 0 460.691 0 1.31874 0 464.531 0 1.41039 0 468.37 0 1.43935 0 472.209 0 1.41345 0 476.048 0 1.43912 0 479.887 0 1.41073 0 483.726 0 1.42082 0 487.565 0 1.42486 0 491.404 0 1.44946 0 495.243 0 1.43009 0 499.082 0 1.40927 0 502.921 0 1.34882 0 506.761 0 1.31893 0 510.6 0 1.31651 0 514.439 0 1.30697 0 518.278 0 1.29097 0 522.117 0 1.22789 0 525.956 0 1.18106 0 529.795 0 1.15615 0 533.634 0 1.12064 0 537.473 0 1.13669 0 541.312 0 1.09394 0 545.152 0 1.12306 0 548.991 0 1.06674 0 552.83 0 1.0828 0 556.669 0 1.09425 0 560.508 0 1.08987 0 564.347 0 1.09982 0 568.186 0 1.09752 0 572.025 0 1.09647 0 575.864 0 1.15553 0 579.703 0 1.15264 0 583.543 0 1.16331 0 587.382 0 1.5941 0 591.221 0 1.64549 0 595.06 0 1.69685 0 598.899 0 1.75094 0 602.738 0 1.75746 0 606.577 0 1.82595 0 610.416 0 1.85449 0 )";
        ss >> parotid;
    }

    {
        std::stringstream ss;
        ss << "(samples_1D. normalityassumed= 0 num_samples= 160 0 0 0.124245 0 3.839 0 0.113352 0 7.678 0 0.1052 0 11.517 0 0.100014 0 15.356 0 0.0951938 0 19.195 0 0.0905557 0 23.034 0 0.0866533 0 26.873 0 0.0867034 0 30.712 0 0.134324 0 34.552 0 0.25156 0 38.391 0 0.40273 0 42.23 0 0.534563 0 46.069 0 0.656232 0 49.908 0 0.749597 0 53.747 0 0.769374 0 57.586 0 0.810791 0 61.425 0 0.823305 0 65.264 0 0.784526 0 69.103 0 0.717455 0 72.943 0 0.623244 0 76.782 0 0.484925 0 80.621 0 0.33441 0 84.46 0 0.186045 0 88.299 0 0.109697 0 92.138 0 0.0841506 0 95.977 0 0.0726764 0 99.816 0 0.0637969 0 103.655 0 0.0624207 0 107.494 0 0.0651357 0 111.334 0 0.0627178 0 115.173 0 0.0634492 0 119.012 0 0.0651144 0 122.851 0 0.0650617 0 126.69 0 0.0636952 0 130.529 0 0.0620742 0 134.368 0 0.0555182 0 138.207 0 0.0508865 0 142.046 0 0.046201 0 145.885 0 0.0464595 0 149.725 0 0.046104 0 153.564 0 0.0447876 0 157.403 0 0.042816 0 161.242 0 0.043249 0 165.081 0 0.0426918 0 168.92 0 0.0431102 0 172.759 0 0.0440611 0 176.598 0 0.0465236 0 180.437 0 0.0466961 0 184.276 0 0.0473576 0 188.115 0 0.0482582 0 191.955 0 0.0491528 0 195.794 0 0.0483368 0 199.633 0 0.0501933 0 203.472 0 0.0515781 0 207.311 0 0.0560583 0 211.15 0 0.0559256 0 214.989 0 0.0544701 0 218.828 0 0.0546636 0 222.667 0 0.0540459 0 226.506 0 0.0540523 0 230.346 0 0.0567152 0 234.185 0 0.0583739 0 238.024 0 0.0589267 0 241.863 0 0.058371 0 245.702 0 0.0597479 0 249.541 0 0.0615032 0 253.38 0 0.0597276 0 257.219 0 0.0585958 0 261.058 0 0.0583066 0 264.897 0 0.0555681 0 268.737 0 0.0566288 0 272.576 0 0.0600921 0 276.415 0 0.0612357 0 280.254 0 0.0645958 0 284.093 0 0.0669342 0 287.932 0 0.0649984 0 291.771 0 0.0651269 0 295.61 0 0.0656374 0 299.449 0 0.0676706 0 303.288 0 0.0661735 0 307.128 0 0.0694914 0 310.967 0 0.0689067 0 314.806 0 0.0661032 0 318.645 0 0.0635616 0 322.484 0 0.0613225 0 326.323 0 0.0580835 0 330.162 0 0.0558384 0 334.001 0 0.0555782 0 337.84 0 0.0554843 0 341.679 0 0.0555765 0 345.518 0 0.0553311 0 349.358 0 0.0557345 0 353.197 0 0.0537993 0 357.036 0 0.0550078 0 360.875 0 0.056308 0 364.714 0 0.050991 0 368.553 0 0.0511456 0 372.392 0 0.0513588 0 376.231 0 0.0523539 0 380.07 0 0.0500169 0 383.909 0 0.0499977 0 387.749 0 0.0497265 0 391.588 0 0.0503976 0 395.427 0 0.0498487 0 399.266 0 0.0513101 0 403.105 0 0.0503778 0 406.944 0 0.0477146 0 410.783 0 0.0495988 0 414.622 0 0.0485852 0 418.461 0 0.0490688 0 422.3 0 0.0510948 0 426.14 0 0.05199 0 429.979 0 0.0527995 0 433.818 0 0.0541391 0 437.657 0 0.0552376 0 441.496 0 0.0562477 0 445.335 0 0.0559649 0 449.174 0 0.0576771 0 453.013 0 0.0600685 0 456.852 0 0.0620805 0 460.691 0 0.0800851 0 464.531 0 0.0960101 0 468.37 0 0.103421 0 472.209 0 0.110371 0 476.048 0 0.110435 0 479.887 0 0.110142 0 483.726 0 0.109888 0 487.565 0 0.109268 0 491.404 0 0.1065 0 495.243 0 0.106719 0 499.082 0 0.106335 0 502.921 0 0.105394 0 506.761 0 0.1041 0 510.6 0 0.103863 0 514.439 0 0.102285 0 518.278 0 0.0981924 0 522.117 0 0.0937689 0 525.956 0 0.0921229 0 529.795 0 0.0947466 0 533.634 0 0.0941222 0 537.473 0 0.0944024 0 541.312 0 0.100495 0 545.152 0 0.0944395 0 548.991 0 0.0940312 0 552.83 0 0.0953654 0 556.669 0 0.0955088 0 560.508 0 0.0958314 0 564.347 0 0.0950385 0 568.186 0 0.0950293 0 572.025 0 0.0949703 0 575.864 0 0.0953085 0 579.703 0 0.0915556 0 583.543 0 0.0873823 0 587.382 0 0.387012 0 591.221 0 0.398983 0 595.06 0 0.417137 0 598.899 0 0.440002 0 602.738 0 0.475907 0 606.577 0 0.516201 0 610.416 0 0.565675 0 )";
        ss >> masseter;
    }

    {
        std::stringstream ss;
        ss << "(samples_1D. normalityassumed= 0 num_samples= 160 0 0 0.0377121 0 3.839 0 0.0458358 0 7.678 0 0.047502 0 11.517 0 0.0458828 0 15.356 0 0.0484886 0 19.195 0 0.0492785 0 23.034 0 0.0504158 0 26.873 0 0.0588414 0 30.712 0 0.0988049 0 34.552 0 0.211086 0 38.391 0 0.368419 0 42.23 0 0.550288 0 46.069 0 0.682907 0 49.908 0 0.775461 0 53.747 0 0.838908 0 57.586 0 0.874994 0 61.425 0 0.86833 0 65.264 0 0.826871 0 69.103 0 0.758103 0 72.943 0 0.656548 0 76.782 0 0.52773 0 80.621 0 0.368445 0 84.46 0 0.212967 0 88.299 0 0.118276 0 92.138 0 0.0859998 0 95.977 0 0.0782172 0 99.816 0 0.0736424 0 103.655 0 0.0741623 0 107.494 0 0.0711135 0 111.334 0 0.0702698 0 115.173 0 0.0697297 0 119.012 0 0.0695625 0 122.851 0 0.0688184 0 126.69 0 0.0666013 0 130.529 0 0.0635936 0 134.368 0 0.0607 0 138.207 0 0.0603108 0 142.046 0 0.0610561 0 145.885 0 0.0636915 0 149.725 0 0.0641999 0 153.564 0 0.0649581 0 157.403 0 0.0655202 0 161.242 0 0.0630764 0 165.081 0 0.064131 0 168.92 0 0.0639395 0 172.759 0 0.0665879 0 176.598 0 0.0670355 0 180.437 0 0.0654468 0 184.276 0 0.0674919 0 188.115 0 0.069213 0 191.955 0 0.0710492 0 195.794 0 0.0736232 0 199.633 0 0.0774034 0 203.472 0 0.0823709 0 207.311 0 0.0908069 0 211.15 0 0.0909089 0 214.989 0 0.0921903 0 218.828 0 0.0904022 0 222.667 0 0.0899652 0 226.506 0 0.0897819 0 230.346 0 0.0880833 0 234.185 0 0.0874559 0 238.024 0 0.0894015 0 241.863 0 0.0910594 0 245.702 0 0.0889377 0 249.541 0 0.0850975 0 253.38 0 0.0788352 0 257.219 0 0.0774264 0 261.058 0 0.0744838 0 264.897 0 0.0724689 0 268.737 0 0.0736443 0 272.576 0 0.0798839 0 276.415 0 0.0840428 0 280.254 0 0.085373 0 284.093 0 0.089174 0 287.932 0 0.0935498 0 291.771 0 0.0957526 0 295.61 0 0.0954592 0 299.449 0 0.0939907 0 303.288 0 0.0954262 0 307.128 0 0.0943993 0 310.967 0 0.0906123 0 314.806 0 0.088046 0 318.645 0 0.0865937 0 322.484 0 0.0832755 0 326.323 0 0.0815563 0 330.162 0 0.080707 0 334.001 0 0.0822692 0 337.84 0 0.0827854 0 341.679 0 0.0810827 0 345.518 0 0.0792156 0 349.358 0 0.0790914 0 353.197 0 0.078288 0 357.036 0 0.0760742 0 360.875 0 0.0759455 0 364.714 0 0.0723538 0 368.553 0 0.0720689 0 372.392 0 0.0709292 0 376.231 0 0.0714941 0 380.07 0 0.070142 0 383.909 0 0.072446 0 387.749 0 0.0715759 0 391.588 0 0.0741206 0 395.427 0 0.0723848 0 399.266 0 0.0715992 0 403.105 0 0.0695425 0 406.944 0 0.0692299 0 410.783 0 0.0701988 0 414.622 0 0.0687831 0 418.461 0 0.0666017 0 422.3 0 0.0667665 0 426.14 0 0.0683621 0 429.979 0 0.0684988 0 433.818 0 0.0694798 0 437.657 0 0.0707701 0 441.496 0 0.0687432 0 445.335 0 0.070432 0 449.174 0 0.0690008 0 453.013 0 0.0687072 0 456.852 0 0.0671452 0 460.691 0 0.0706095 0 464.531 0 0.0740976 0 468.37 0 0.0743658 0 472.209 0 0.0774453 0 476.048 0 0.0791396 0 479.887 0 0.0845228 0 483.726 0 0.0912632 0 487.565 0 0.0929489 0 491.404 0 0.0900391 0 495.243 0 0.0875683 0 499.082 0 0.0881047 0 502.921 0 0.0869976 0 506.761 0 0.0796258 0 510.6 0 0.07343 0 514.439 0 0.068705 0 518.278 0 0.0639894 0 522.117 0 0.065644 0 525.956 0 0.0679271 0 529.795 0 0.0688172 0 533.634 0 0.0714504 0 537.473 0 0.0714593 0 541.312 0 0.0687999 0 545.152 0 0.0684519 0 548.991 0 0.0681467 0 552.83 0 0.0681153 0 556.669 0 0.0706996 0 560.508 0 0.0736164 0 564.347 0 0.0724479 0 568.186 0 0.0739431 0 572.025 0 0.0742564 0 575.864 0 0.0746973 0 579.703 0 0.0744944 0 583.543 0 0.0738472 0 587.382 0 0.452857 0 591.221 0 0.479425 0 595.06 0 0.51137 0 598.899 0 0.547894 0 602.738 0 0.588812 0 606.577 0 0.645241 0 610.416 0 0.706846 0 )";
        ss >> pharynx;
    }

    parotid.Normalize_wrt_Self_Overlap();
    masseter.Normalize_wrt_Self_Overlap();
    pharynx.Normalize_wrt_Self_Overlap();


    const auto paired_t_test2 = [](const samples_1D<double> &L, const samples_1D<double> &R) -> void {
        std::vector<std::array<double,2>> paired_datum;

        if(L.size() != R.size()) YLOGERR("Differing sizes. The paired t-test requires identical x_i");
        for(auto i = 0; i < L.size(); ++i){
            paired_datum.push_back( { L.samples[i][2], R.samples[i][2] } );
        }
        const double dof_override = static_cast<double>(paired_datum.size() - 1 - 2);
        YLOGINFO("2-tail P-value = " << Stats::P_From_Paired_Ttest_2Tail(paired_datum, dof_override));
        return;
    };

    YLOGINFO("Parotid and masseter");
    paired_t_test2(parotid, masseter);

    YLOGINFO("Parotid and pharynx");
    paired_t_test2(parotid, pharynx);

    YLOGINFO("Masseter and pharynx");
    paired_t_test2(masseter, pharynx);


    const auto working_P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail = [](const samples_1D<double> &L, const samples_1D<double> &R) -> void {
        std::vector<std::array<double,2>> paired_datum;

        if(L.size() != R.size()) YLOGERR("Differing sizes");
        for(auto i = 0; i < L.size(); ++i){
            paired_datum.push_back( { L.samples[i][2], R.samples[i][2] } );
        }
        YLOGINFO("Wilcoxon sign-rank test P-value = " << Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(paired_datum));
        return;
    };

    YLOGINFO("Parotid and masseter");
    working_P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(parotid, masseter);

    YLOGINFO("Parotid and pharynx");
    working_P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(parotid, pharynx);

    YLOGINFO("Masseter and pharynx");
    working_P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(masseter, pharynx);



    //Demonstrate a Wikipedia example for which there is not enough data to generate a z-score.
    {
        YLOGINFO("Wikipedia sample");
        std::vector<std::array<double,2>> data = {
        {125.0, 110.0},
        {115.0, 122.0},
        {130.0, 125.0},
        {140.0, 120.0},
        {140.0, 140.0},
        {115.0, 124.0},
        {140.0, 123.0},
        {125.0, 137.0},
        {140.0, 135.0},
        {135.0, 145.0} };

        const auto ret = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(data);
        YLOGINFO("Wilcoxon sign-rank test P-value = " << ret);
    }

    //Demonstrate a Vassar stats example for which there is enough data to generate z-score and P-value.
    {
        YLOGINFO("Vassar stats sample");
        std::vector<std::array<double,2>> data = {
        {78.0, 78.0},
        {24.0, 24.0},
        {64.0, 62.0},
        {45.0, 48.0},
        {64.0, 68.0},
        {52.0, 56.0},
        {30.0, 25.0},
        {50.0, 44.0},
        {64.0, 56.0},
        {50.0, 40.0},
        {78.0, 68.0},
        {22.0, 36.0},
        {84.0, 68.0},
        {40.0, 20.0},
        {90.0, 58.0},
        {72.0, 32.0} };

        const auto ret = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(data);
        YLOGINFO("Wilcoxon sign-rank test P-value = " << ret);
    }

    //Demonstrate a Vassar stats example for which there is enough data to generate z-score and P-value.
    {
        YLOGINFO("Rosie Shier's Mathematics Learning Support Centre sample");
        std::vector<std::array<double,2>> data = {
        { 2.0,  3.5},
        { 3.6,  5.7},
        { 2.6,  2.9},
        { 2.6,  2.4},
        { 7.3,  9.9},
        { 3.4,  3.3},
        {14.9, 16.7},
        { 6.6,  6.0},
        { 2.3,  3.8},
        { 2.0,  4.0},
        { 6.8,  9.1},
        { 8.5, 20.9} };

        const auto ret = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(data);
        YLOGINFO("Wilcoxon sign-rank test P-value = " << ret);
    }

    //Compute P-values from critical z-scores. The z-scores are critical exactly because they correspond to the
    // z where p = 0.05, 0.025, 0.01, 0.005, and 0.0005.
    {
        double z;
  
        z = 1.645;
        YLOGINFO(" Two-tailed P-value = " << Stats::P_From_Zscore_2Tail(z) << " when z-score = " << z);
        z = 1.960;
        YLOGINFO(" Two-tailed P-value = " << Stats::P_From_Zscore_2Tail(z) << " when z-score = " << z);
        z = 2.326;
        YLOGINFO(" Two-tailed P-value = " << Stats::P_From_Zscore_2Tail(z) << " when z-score = " << z);
        z = 2.576;
        YLOGINFO(" Two-tailed P-value = " << Stats::P_From_Zscore_2Tail(z) << " when z-score = " << z);
        z = 3.291;
        YLOGINFO(" Two-tailed P-value = " << Stats::P_From_Zscore_2Tail(z) << " when z-score = " << z);

        std::cout << std::endl;
        z = 1.645;
        YLOGINFO(" Upper-tailed P-value = " << Stats::P_From_Zscore_Upper_Tail(z) << " when z-score = " << z);
        z = 1.960;
        YLOGINFO(" Upper-tailed P-value = " << Stats::P_From_Zscore_Upper_Tail(z) << " when z-score = " << z);
        z = 2.326;
        YLOGINFO(" Upper-tailed P-value = " << Stats::P_From_Zscore_Upper_Tail(z) << " when z-score = " << z);
        z = 2.576;
        YLOGINFO(" Upper-tailed P-value = " << Stats::P_From_Zscore_Upper_Tail(z) << " when z-score = " << z);
        z = 3.291;
        YLOGINFO(" Upper-tailed P-value = " << Stats::P_From_Zscore_Upper_Tail(z) << " when z-score = " << z);

        std::cout << std::endl;
        z = 1.645;
        YLOGINFO(" Lower-tailed P-value = " << Stats::P_From_Zscore_Lower_Tail(z) << " when z-score = " << z);
        z = 1.960;
        YLOGINFO(" Lower-tailed P-value = " << Stats::P_From_Zscore_Lower_Tail(z) << " when z-score = " << z);
        z = 2.326;
        YLOGINFO(" Lower-tailed P-value = " << Stats::P_From_Zscore_Lower_Tail(z) << " when z-score = " << z);
        z = 2.576;
        YLOGINFO(" Lower-tailed P-value = " << Stats::P_From_Zscore_Lower_Tail(z) << " when z-score = " << z);
        z = 3.291;
        YLOGINFO(" Lower-tailed P-value = " << Stats::P_From_Zscore_Lower_Tail(z) << " when z-score = " << z);
    }

    return 0;
}

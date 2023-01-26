//Test_Math15.cc - Tests of bisection.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"

    //This routine returns a pair of planes that approximately encompass the desired interior volume. The ROIs are not
    // altered. The lower plane is the first element of the pair. This routine can be applied to any contour_collection
    // and the planes can also be applied to any contour_collection.
    const auto bisect_ROIs = [](const contour_collection<double> &ROIs,
                                const vec3<double> &planar_normal,
                                double SelectionLower,
                                double SelectionUpper) ->
                                    std::pair<plane<double>, plane<double>> {


        // Bisection parameters. It usually converges quickly but can get stuck, so cap the max_iters fairly low.
        const double acceptable_deviation = 0.00001; //Deviation from desired_total_area_fraction_above_plane.
        const size_t max_iters = 200; //If the tolerance cannot be reached after this many iters, report the current plane as-is.

        if(ROIs.contours.empty()) throw std::logic_error("Unable to split empty contour collection.");
        size_t iters_taken = 0;
        double final_area_frac = 0.0;

        //Find the lower plane.
        plane<double> lower_plane;
        ROIs.Total_Area_Bisection_Along_Plane(planar_normal,
                                                      SelectionLower,
                                                      acceptable_deviation,
                                                      max_iters,
                                                      &lower_plane,
                                                      &iters_taken,
                                                      &final_area_frac);
        FUNCINFO("Bisection: planar area fraction"
                 << " above LOWER plane with normal: " << planar_normal
                 << " was " << final_area_frac << "."
                 << " Requested: " << SelectionLower << "."
                 << " Iters: " << iters_taken);                                                                                                       
                                                                                                                                                      
        //Find the upper plane.
        plane<double> upper_plane;
        ROIs.Total_Area_Bisection_Along_Plane(planar_normal,
                                                      SelectionUpper,
                                                      acceptable_deviation,
                                                      max_iters,
                                                      &upper_plane,
                                                      &iters_taken,
                                                      &final_area_frac);
        FUNCINFO("Bisection: planar area fraction"
                 << " above UPPER plane with normal: " << planar_normal
                 << " was " << final_area_frac << "."
                 << " Requested: " << SelectionUpper << "."
                 << " Iters: " << iters_taken);

        return std::make_pair(lower_plane, upper_plane);
    };

    const auto subsegment_interior = [](const contour_collection<double> &ROIs,
                                        const std::pair<plane<double>, plane<double>> &planes) ->
                                            contour_collection<double> {
        const plane<double> lower_plane(planes.first);
        const plane<double> upper_plane(planes.second);

        //Implements the sub-segmentation, selecting only the interior portion.
        auto split1 = ROIs.Split_Along_Plane(lower_plane);
        if(split1.size() != 2){
            throw std::logic_error("Expected exactly two groups, above and below plane.");
        }
        auto split2 = split1.back().Split_Along_Plane(upper_plane);
        if(split2.size() != 2){
            throw std::logic_error("Expected exactly two groups, above and below plane.");
        }
        
        if(false) for(auto it = split2.begin(); it != split2.end(); ++it){ it->Plot(); }

        const contour_collection<double> cc_selection( split2.front() );
        if( cc_selection.contours.empty() ){
            FUNCWARN("Selection contains no contours. Try adjusting your criteria.");
        }
        return cc_selection;
    };


    const auto initiate_subseg_planar = [](const contour_collection<double> &ROIs,
                                            const vec3<double> &x_normal,
                                            const vec3<double> &y_normal, 
                                            double XSelectionLower, double XSelectionUpper,
                                            double YSelectionLower, double YSelectionUpper,
                                            double area_expected) -> contour_collection<double> {

        contour_collection<double> cc_final;

        // ---------------------------------- Independent sub-segmentation --------------------------------------
        //Generate all planes using the original contour_collection.
        if(false){
            const auto x_planes_pair = bisect_ROIs(ROIs, x_normal, XSelectionLower, XSelectionUpper);
            const auto y_planes_pair = bisect_ROIs(ROIs, y_normal, YSelectionLower, YSelectionUpper);
            
            //Perform the sub-segmentation.
            contour_collection<double> running(ROIs);
            running = subsegment_interior(running, x_planes_pair);
            running = subsegment_interior(running, y_planes_pair);
            cc_final = running;
        }
        
        // ----------------------------------- Iterative sub-segmentation ---------------------------------------
        // Instead of relying on whole-organ sub-segmentation, attempt to fairly partition the *remaining* volume 
        // at each cleave.
        if(true){
            contour_collection<double> running(ROIs);
            
            const auto x_planes_pair = bisect_ROIs(running, x_normal, XSelectionLower, XSelectionUpper);
            running = subsegment_interior(running, x_planes_pair);
            
            const auto y_planes_pair = bisect_ROIs(running, y_normal, YSelectionLower, YSelectionUpper);
            running = subsegment_interior(running, y_planes_pair);

            cc_final = running;
        }

        const auto area_orig = ROIs.Get_Signed_Area(/*AssumePlanarContours*/ true);
        const auto area_sbsg = cc_final.Get_Signed_Area(/*AssumePlanarContours*/ true);

        std::cout << "The amount of area selected = " << area_sbsg << " / " << area_orig 
                  << " == " << (area_sbsg/area_orig)
                  << std::endl;
        std::cout << "  As a fraction of the expected area: " << (area_sbsg/area_expected)
                  << std::endl;
        return cc_final;
    };

    const auto initiate_subseg_volumetric = [](const contour_collection<double> &ROIs,
                                               const vec3<double> &x_normal,
                                               const vec3<double> &y_normal, 
                                               const vec3<double> &z_normal, 
                                               double XSelectionLower, double XSelectionUpper,
                                               double YSelectionLower, double YSelectionUpper,
                                               double ZSelectionLower, double ZSelectionUpper,
                                               double area_expected) -> contour_collection<double> {

        contour_collection<double> cc_final;

        // ---------------------------------- Independent sub-segmentation --------------------------------------
        //Generate all planes using the original contour_collection.
        if(true){
            const auto x_planes_pair = bisect_ROIs(ROIs, x_normal, XSelectionLower, XSelectionUpper);
            const auto y_planes_pair = bisect_ROIs(ROIs, y_normal, YSelectionLower, YSelectionUpper);
            const auto z_planes_pair = bisect_ROIs(ROIs, z_normal, ZSelectionLower, ZSelectionUpper);
            
            //Perform the sub-segmentation.
            contour_collection<double> running(ROIs);
            running = subsegment_interior(running, x_planes_pair);
            running = subsegment_interior(running, y_planes_pair);
            running = subsegment_interior(running, z_planes_pair);
            cc_final = running;
        }
        
        // ----------------------------------- Iterative sub-segmentation ---------------------------------------
        // Instead of relying on whole-organ sub-segmentation, attempt to fairly partition the *remaining* volume 
        // at each cleave.
        if(false){
            contour_collection<double> running(ROIs);
            
            const auto x_planes_pair = bisect_ROIs(running, x_normal, XSelectionLower, XSelectionUpper);
            running = subsegment_interior(running, x_planes_pair);
            
            const auto y_planes_pair = bisect_ROIs(running, y_normal, YSelectionLower, YSelectionUpper);
            running = subsegment_interior(running, y_planes_pair);

            const auto z_planes_pair = bisect_ROIs(running, z_normal, ZSelectionLower, ZSelectionUpper);
            running = subsegment_interior(running, z_planes_pair);

            cc_final = running;
        }

        const auto area_orig = ROIs.Get_Signed_Area(/*AssumePlanarContours*/ true);
        const auto area_sbsg = cc_final.Get_Signed_Area(/*AssumePlanarContours*/ true);

        std::cout << "The amount of area selected = " << area_sbsg << " / " << area_orig 
                  << " == " << (area_sbsg/area_orig)
                  << std::endl;
        std::cout << "  As a fraction of the expected area: " << (area_sbsg/area_expected)
                  << std::endl;
        return cc_final;
    };



int main(int argc, char **argv){

    const vec3<double> x_normal(1.0, 0.0, 0.0);
    const vec3<double> y_normal(0.0, 1.0, 0.0);
    const vec3<double> z_normal(0.0, 0.0, 1.0);

    //Planar ROI: single circle contour.
    if(true){
        contour_collection<double> ROIs;
        contour_collection<double> subsegs;

        contour_of_points<double> cop;
        const double r = 100.0;
        const long int Npoints = 5000;
        cop.closed = true;
        for(long int i = 0; i < Npoints; ++i){
            const double theta = 0.0 + (2.0*M_PI/Npoints)*i;
            cop.points.emplace_back( r * std::cos(theta), r * std::sin(theta), 0.0 );
        }
        ROIs.contours.emplace_back( cop );

        const auto area_theo_whole = M_PI * r * r;
        const auto area_actual_whole = ROIs.Get_Signed_Area(/*AssumePlanarContours*/ true);

        FUNCINFO("Theoretical and actual whole ROI areas differ by " << 100.0*(area_actual_whole - area_theo_whole)/area_theo_whole << "%");

        const auto area_expected = area_actual_whole / 9.0;


        FUNCINFO("Performing [1/3,0], [1/3,0] sub-seg. (This is a 'corner' sub-seg).");
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (1.0/3.0), (0.0/3.0), // X-selection: lower and upper planes; fractional volume above each plane.
                                   (1.0/3.0), (0.0/3.0), // Y-selection: lower and upper planes; fractional volume above each plane.
                                   area_expected);       // Expected area.
            subsegs.Consume_Contours(c);
        }

        FUNCINFO("Performing [2/3,1/3], [2/3,1/3] sub-seg. (This is the 'centre' sub-seg).");
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (2.0/3.0), (1.0/3.0),
                                   (2.0/3.0), (1.0/3.0),
                                   area_expected);  // Expected area.
            //FUNCWARN("Signed area (true) = "  << c.Get_Signed_Area(/*AssumePlanarContours*/ true));
            //FUNCWARN("Signed area (false) = " << c.Get_Signed_Area(/*AssumePlanarContours*/ false));
            //FUNCWARN("Centroid = " << c.Centroid());
            subsegs.Consume_Contours(c);
        }

        FUNCINFO("Performing [2/3,1/3], [1/3,0] sub-seg. (This is the 'top-central' sub-seg).");
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (2.0/3.0), (1.0/3.0),
                                   (1.0/3.0), (0.0/3.0),
                                   area_expected);  // Expected area.
            subsegs.Consume_Contours(c);
        }

        //The remaining sub-segments.
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (3.0/3.0), (2.0/3.0),
                                   (3.0/3.0), (2.0/3.0), area_expected);
            subsegs.Consume_Contours(c);
        }
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (2.0/3.0), (1.0/3.0),
                                   (3.0/3.0), (2.0/3.0), area_expected);
            subsegs.Consume_Contours(c);
        }
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (3.0/3.0), (2.0/3.0),
                                   (2.0/3.0), (1.0/3.0), area_expected);
            subsegs.Consume_Contours(c);
        }
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (1.0/3.0), (0.0/3.0),
                                   (3.0/3.0), (2.0/3.0), area_expected);
            subsegs.Consume_Contours(c);
        }
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (3.0/3.0), (2.0/3.0),
                                   (1.0/3.0), (0.0/3.0), area_expected);
            subsegs.Consume_Contours(c);
        }
        {
            auto c = initiate_subseg_planar(ROIs, x_normal, y_normal,
                                   (1.0/3.0), (0.0/3.0),
                                   (2.0/3.0), (1.0/3.0), area_expected);
            subsegs.Consume_Contours(c);
        }

        //subsegs.Plot();
        //ROIs.Plot();

        //Print the vertices for post-processing.
        const auto dump_vert = [](long int coll_num, long int contour_num, const vec3<double> &p) -> void {
            std::cout << "Vertex: "
                      << "family " << coll_num << " " 
                      << "contour " << contour_num << " "
                      << p.x << " " << p.y << " " << p.z 
                      << std::endl;
        };
        long int i = 0; //This records which contour each vertex belongs to.
        for(const auto &c : subsegs.contours){
            for(const auto &p : c.points) dump_vert(0, i, p);
            if(!c.points.empty() && c.closed) dump_vert(0, i, c.points.front());
            std::cout << std::endl;
            ++i;
        }

    }

    //Volumetric ROI: sphere defined in terms of stacks of planar circles.
    if(false){
        contour_collection<double> ROIs;
        contour_collection<double> subsegs;

        const long int Ncontours = 200;
        const long int Npoints = 700; // The number of vertices for the contour on the equator.
        const double r = 10.0;
        double area_theo_whole = 0.0;

        for(long int i = 1; i <= Ncontours; ++i){
            const double z = (-r) + 2.0*r*i/(Ncontours+1); //We want equidistant contours, so sample z evenly.
            const double phi = std::acos(z/r);             //Phi will NOT be sampled evenly.
            const double r_eff = r * std::sin(phi);        //Radius of this circle is r*sin(phi) (== r_effective).
            area_theo_whole += M_PI * r_eff * r_eff;
            contour_of_points<double> cop;
            cop.closed = true;

            //Try to maintain the same vertex spacing for all contours. This lets us crank up Npoints and get the best
            // 'bang for our buck' withou needlessly oversampling small contours.  
            long int NpointsThisContour = static_cast<long int>( std::ceil(std::sin(phi)*Npoints) );
            if(NpointsThisContour < 10) NpointsThisContour = 10;

            for(long int j = 0; j < NpointsThisContour; ++j){
                const double theta = 0.0 + (2.0*M_PI/NpointsThisContour)*j;
                cop.points.emplace_back( r * std::cos(theta) * std::sin(phi), 
                                         r * std::sin(theta) * std::sin(phi),
                                         z );
            }
            ROIs.contours.emplace_back( cop );
        }

        const auto area_actual_whole = ROIs.Get_Signed_Area(/*AssumePlanarContours*/ true);

        FUNCINFO("Theoretical and actual whole ROI areas differ by " << 100.0*(area_actual_whole - area_theo_whole)/area_theo_whole << "%");

        const auto area_expected = area_actual_whole / 27.0;


        FUNCINFO("Performing [1/3,0], [1/3,0], [1/3,0] sub-seg. (This is a 'corner' sub-seg).");
        {
            auto c = initiate_subseg_volumetric(ROIs, x_normal, y_normal, z_normal,
                                   (1.0/3.0), (0.0/3.0), // X-selection: lower and upper planes; fractional volume above each plane.
                                   (1.0/3.0), (0.0/3.0), // Y-selection: lower and upper planes; fractional volume above each plane.
                                   (1.0/3.0), (0.0/3.0),
                                   area_expected);       // Expected area.
            subsegs.Consume_Contours(c);
        }

        FUNCINFO("Performing [2/3,1/3], [2/3,1/3], [1/3,0] sub-seg. (This is the 'top-centre-adjacent' sub-seg).");
        {
            auto c = initiate_subseg_volumetric(ROIs, x_normal, y_normal, z_normal,
                                   (2.0/3.0), (1.0/3.0),
                                   (2.0/3.0), (1.0/3.0),
                                   (1.0/3.0), (0.0/3.0),
                                   area_expected);  // Expected area.
            //FUNCWARN("Signed area (true) = "  << c.Get_Signed_Area(/*AssumePlanarContours*/ true));
            //FUNCWARN("Signed area (false) = " << c.Get_Signed_Area(/*AssumePlanarContours*/ false));
            //FUNCWARN("Centroid = " << c.Centroid());
            subsegs.Consume_Contours(c);
        }

        FUNCINFO("Performing [2/3,1/3], [1/3,0], [1/3,0] sub-seg. (This is the 'top-cent' sub-seg).");
        {
            auto c = initiate_subseg_volumetric(ROIs, x_normal, y_normal, z_normal,
                                   (2.0/3.0), (1.0/3.0),
                                   (1.0/3.0), (0.0/3.0),
                                   (1.0/3.0), (0.0/3.0),
                                   area_expected);  // Expected area.
            subsegs.Consume_Contours(c);
        }

        FUNCINFO("Performing [2/3,1/3], [2/3,1/3], [2/3,1/3] sub-seg. (This is the 'centre' sub-seg).");
        {
            auto c = initiate_subseg_volumetric(ROIs, x_normal, y_normal, z_normal,
                                   (2.0/3.0), (1.0/3.0),
                                   (2.0/3.0), (1.0/3.0),
                                   (2.0/3.0), (1.0/3.0),
                                   area_expected);  // Expected area.
            subsegs.Consume_Contours(c);
        }

        //subsegs.Plot();
        //ROIs.Plot();
    }


    return 0;
}


//YgorMath.h

#ifndef YGOR_MATH_H_
#define YGOR_MATH_H_
#include <cmath>
#include <iostream>
#include <fstream>
#include <complex>
#include <list>
#include <string>
#include <functional>
#include <tuple>
#include <vector>

//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------- One-Offs ------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//Moved to YgorStats.
//double Stats::P_From_StudT_1Tail(double tval, double dof);
//double Stats::P_From_StudT_2Tail(double tval, double dof);
//
//double Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(double M1, double S1, double N1, double M2, double S2, double N2);
//
//double Stats::Q_From_ChiSq_Fit(double chi_square, double dof);
//
//double Stats::Unbiased_Var_Est(std::list<double> in);


double Ygor_Sum(std::list<double> in);
double Ygor_Sum_Squares(std::list<double> in);

double Ygor_Median(std::list<double> in);
double Ygor_Mean(std::list<double> in);

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec3: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is the basis of the geometry-heavy classes below. It can happily be used on its own.
template <class T> class vec3 {
    public:
        T x, y, z;
    
        //Constructors.
        vec3();
        vec3(T a, T b, T c);
        vec3( const vec3 & );
    
        //Operators - vec3 typed.
        vec3 & operator=(const vec3 &);
    
        vec3   operator+(const vec3 &) const;
        vec3 & operator+=(const vec3 &);
        vec3   operator-(const vec3 &) const;
        vec3 & operator-=(const vec3 &);
    
        bool operator==(const vec3 &) const;
        bool operator!=(const vec3 &) const;
        bool operator<(const vec3 &) const;   //This is mostly for sorting / placing into a std::map. There is no great 
                                              // way to define '<' for a vector, so be careful with it.
    
        //Operators - T typed.
        //vec3   operator*(const T);
        //vec3 & operator*=(const T);
        //vec3   operator/(const T);
        //vec3 & operator/=(const T);
    
        vec3   operator*(const T &) const;
        vec3 & operator*=(const T &);
        vec3   operator/(const T &) const;
        vec3 & operator/=(const T &);
    
        //Member functions.
        T Dot(const vec3 &) const;        // ---> Dot product.
        vec3 Cross(const vec3 &) const;   // ---> Cross product.
        vec3 Mask(const vec3 &) const;    // ---> Term-by-term product:   (a b c).Outer(1 2 0) = (a 2*b 0)
    
        vec3 unit() const;                // ---> Return a normalized version of this vector.
        T length() const;                 // ---> (pythagorean) length of vector.
        T distance(const vec3 &) const;   // ---> (pythagorean) distance between vectors.
        T sq_dist(const vec3 &) const;    // ---> Square of the (pythagorean) distance. Avoids a sqrt().
        T angle(const vec3 &, bool *OK=nullptr) const;  // ---> The |angle| (in radians, [0:pi]) separating two vectors. 

        //Friends.
//        template<class Y> friend std::ostream & operator << (std::ostream &, const vec3<Y> &); // ---> Overloaded stream operators.
//        template<class Y> friend std::istream & operator >> (std::istream &, vec3<T> &);      
        template<class Y> friend std::ostream & operator << (std::ostream &, const vec3<Y> &); // ---> Overloaded stream operators.
        template<class Y> friend std::istream & operator >> (std::istream &, vec3<Y> &);      
};


//This is a function for rotation unit vectors in some plane. It requires angles to describe the plane of rotation, angle of rotation. 
// It also requires a unit vector with which to rotate the plane about.
vec3<double> rotate_unit_vector_in_plane(const vec3<double> &A, const double &theta, const double &R);

//This function evolves a pair of position and velocity (x(t=0),v(t=0)) to a pair (x(t=T),v(t=T)) using the
// classical expression for a time- and position-dependent force F(x;t). It is highly unstable, so the number
// of iterations must be specified. If this is going to be used for anything important, make sure that the
// number of iterations is chosen sufficiently high so as to produce negligible errors.
std::tuple<vec3<double>,vec3<double>> Evolve_x_v_over_T_via_F(const std::tuple<vec3<double>,vec3<double>> &x_and_v, 
                                                              std::function<vec3<double>(vec3<double> x, double T)> F, 
                                                              double T, long int steps);


//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------- vec2: A two-dimensional vector --------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is the basis of the geometry-heavy classes below. It can happily be used on its own.
template <class T> class vec2 {
    public:
        T x, y;

        //Constructors.
        vec2();
        vec2(T a, T b);
        vec2( const vec2 & );

        //Operators - vec2 typed.
        vec2 & operator=(const vec2 &);

        vec2   operator+(const vec2 &) const;
        vec2 & operator+=(const vec2 &);
        vec2   operator-(const vec2 &) const;
        vec2 & operator-=(const vec2 &);

        bool operator==(const vec2 &) const;
        bool operator!=(const vec2 &) const;
        bool operator<(const vec2 &) const;   //This is mostly for sorting / placing into a std::map. There is no great 
                                              // way to define '<' for a vector, so be careful with it.

        //Operators - T typed.
        //vec2   operator*(const T);
        //vec2 & operator*=(const T);
        //vec2   operator/(const T);
        //vec2 & operator/=(const T);

        vec2   operator*(const T &) const;
        vec2 & operator*=(const T &);
        vec2   operator/(const T &) const;
        vec2 & operator/=(const T &);

        //Member functions.
        T Dot(const vec2 &) const;        // ---> Dot product.
        vec2 Mask(const vec2 &) const;    // ---> Term-by-term product:   (a b).Outer(5 0) = (5*a 0*b)

        vec2 unit() const;                // ---> Return a normalized version of this vector.
        T length() const;                 // ---> (pythagorean) length of vector.
        T distance(const vec2 &) const;   // ---> (pythagorean) distance between vectors.
        T sq_dist(const vec2 &) const;    // ---> Square of the (pythagorean) distance. Avoids a sqrt().

        //Friends.
//        template<class Y> friend std::ostream & operator << (std::ostream &, const vec2<Y> &); // ---> Overloaded stream operator.
//        template<class Y> friend std::istream & operator >> (std::istream &, vec2<T> &);
        template<class Y> friend std::ostream & operator << (std::ostream &, const vec2<Y> &); // ---> Overloaded stream operators.
        template<class Y> friend std::istream & operator >> (std::istream &, vec2<Y> &);           
};


//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------- line: (infinitely-long) lines in 3D space ---------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   class line {
    public:
        vec3<T> R_0;  //A point which the line intersects.
        vec3<T> U_0;  //A unit vector which points along the length of the line.

        //Constructors.
        line();
        line(const vec3<T> &R_A, const vec3<T> &R_B);

        //Member functions.
        T Distance_To_Point( const vec3<T> & ) const;
        bool Intersects_With_Line_Once( const line<T> &, vec3<T> &) const;
        bool Closest_Point_To_Line( const line<T> &, vec3<T> &) const;
};


//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------ line_segment: (finite-length) lines in 3D space --------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
// The difference between a line segment and a line is that the line segment can only be interpolated between two points,
// whereas a line will extend off to infinity.
template <class T>   class line_segment : public line<T> {
    public:
        T t_0;  //These define the endpoints of the line segment in terms of the parameterization R(t) = R_0 + t*U_0.
        T t_1;  // Whenever endpoints are required, they are explicitly computed. (We can usually get around this, though!)

        //Constructors.
        line_segment();
        line_segment(const vec3<T> &R_A, const vec3<T> &R_B);  //Line segment starts at A (at t=t_0), terminates at B (at t=t_1.)

        //Member functions.
        std::list<vec3<T>> Sample_With_Spacing(T spacing, T offset, T & remaining) const; //Samples every <spacing>, beginning at offset. 
                                                                                          // Returns sampled points and remaining space along segment.

        vec3<T> Get_R0(void) const;
        vec3<T> Get_R1(void) const;
};

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------- plane: 2D planes in 3D space -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//A 3D plane can be described completely with the relation:  $\vec{N}_{0} \cdot ( \vec{R} - \vec{R}_{0} ) = 0$ (where $\vec{R}$ 
// is a point on the plane if it satisfies this identity.)

template <class T>   class plane {
    public:
        vec3<T> N_0;  //A unit vector normal to the plane. The orientation is up to the user to choose (and be consistent with!)
        vec3<T> R_0;  //A 3D point the plane intersects.

        //Constructors.
        plane();
        plane(const vec3<T> &N_0_in, const vec3<T> &R_0_in);

        //Member functions.
        T Get_Signed_Distance_To_Point(const vec3<T> &R) const;
        bool Is_Point_Above_Plane(const vec3<T> &R) const;      //Essentially returns the sign. True if pointing in direction of plane's 
                                                                // positive normal, false if in opposite direction. Do not rely on any sign
                                                                // in case the point is in the plane (but a consistent sign IS given in this case.)
        bool Intersects_With_Line_Once(const line<T> &L, vec3<T> &out) const;
};



//---------------------------------------------------------------------------------------------------------------------------
//------------------ contour_of_points: a polygon of line segments in the form of a collection of points --------------------
//---------------------------------------------------------------------------------------------------------------------------
//Simple structure representing a polygon of line segments in the form of a collection of points in 3D space (using vec3<...>'s.) 
// Alternatively, they can be thought of as the perimeter of a (2D) structure.
//
//The points may or may not form a closed surface (ie. the first and last points are connected.) They must be marked as being either.
//
//NOTE: This class holds a *single* contour, represented as a list of points in R^3.
template <class T>   class contour_of_points {
    public:
        std::list<vec3<T>> points; //We use doubly-linked lists because the points can then be re-ordered, inserted or removed, or shuffled around with small cost.
                                   // Additionally, we typically do not random access the points, but instead iterate over them (drawing them, computing areas, etc..
  
        bool closed;               //Are the first and last points connected via a line? For instance, we cannot compute the area of the line segment if they are not.

        //Constructors.
        contour_of_points();
        contour_of_points(const std::list<vec3<T>> &points);

        //Member functions.
//        void append(const vec3<T> &point);
        T Get_Signed_Area(void) const;
        bool Is_Counter_Clockwise(void) const;
        void Reorient_Counter_Clockwise(void);

        vec3<T> Average_Point(void) const;   //This is the average of all the 3D points of the contour. It has little value except simplicity.
        vec3<T> Centroid(void) const;        //This is the centroid of the contour. This is a more mathematically well-define concept. See the source.
        vec3<T> First_N_Point_Avg(const long int N) const; //Useful when dealing with planes for determining if above or below.
        T Perimeter(void) const;             //This is the perimeter (length) of the contour edges. It is computed from vertex to vertex (positive-definite.)

        contour_of_points  Bounding_Box_Along(const vec3<T> &, T margin = (T)(0.0)) const; //Gives a (planar 2D) bounding box with two edges aligned with the given unit vector. 

        std::list<contour_of_points<T>> Split_Along_Plane(const plane<T> &) const; //Splits the contour into individual contours along the given plane. This is a "geometric" splitting.
        std::list<contour_of_points<T>> Split_Against_Ray(const  vec3<T> &) const; //Splits the contour into individual contours along the halfway point of a given ray unit vector. This is a "ray casting" splitting.
        std::list<contour_of_points<T>> Split_Into_Core_Peel_Spherical(const vec3<T> &, T frac_dist) const; //Splits contour into an inner (core) and outer (peel).

        T Integrate_Simple_Scalar_Kernel(std::function<    T    (const vec3<T> &r, const vec3<T> &A, const vec3<T> &B, const vec3<T> &U)> k) const;
        T Integrate_Simple_Vector_Kernel(std::function< vec3<T> (const vec3<T> &r, const vec3<T> &A, const vec3<T> &B, const vec3<T> &U)> k) const;

        contour_of_points Resample_Evenly_Along_Perimeter(const long int N) const; //Walks along the perimeter, creating a new point (using interpolation) every [perimeter]/N .
                                                                                   // Points will only be homogeneously distributed if the shape is circular (or by fluke.)
        contour_of_points Scale_Dist_From_Point(const vec3<T> &, T scale) const;   //Scales distance from each point to given point by factor (scale).

        contour_of_points Resample_LTE_Evenly_Along_Perimeter(const long int N) const; //Performs Resample_Evenly_Along_Perimeter only on contours with more than N.


        long int Avoids_Plane(const plane<T> &P) const; //-1 if contour is below P, 0 if it intersects/crosses it, 1 if it is above.

        //These routines remove points unless/until there are <= 3 of them!
        void Remove_Sequential_Duplicate_Points(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq);
        void Remove_Extraneous_Points(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq); //Removes unnecessary points unnecessary to exactly retain contour.
        void Remove_Needles(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq); //Turns ...-A-B-A-C-D-...  into ...-A-C-D-...

        contour_of_points & operator= (const contour_of_points &);
        bool operator==(const contour_of_points &) const; //Attempts to compare global contour, without disregarding location of first point within the contour.
        bool operator!=(const contour_of_points &) const;
        bool operator< (const contour_of_points &) const; //This is mostly for sorting / placing into a std::map. There is no great 
                                                          // way to define '<' for a contour, so be careful with it. This is designed to be fast, not reliable!

        void Plot(void) const;                            //Spits out a default R^2 plot of the data. Ignores the z-component entirely!

        std::string write_to_string(void) const;
        bool load_from_string(const std::string &in);     //Returns true if it worked/was loaded into the contour, false otherwise.
};


//---------------------------------------------------------------------------------------------------------------------------
//---------------------- contour_collection: a collection of logically-related contour_of_points  ---------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is a sort of utility class for holding logically-grouped contours. For instance, a 3D shape may be represented
// as a stack of contour_of_points. Note, however, that the onus is on the user to ensure the collection is sorted/handled
// in a consistent way.
//
//Essentially, this class is a wrapper around a std::list<contour_of_points> with some extra things like 3D plotting
// and some multi-contour operations (like average centroid, stringification, etc..)
//
//NOTE: This class assumes that ALL contours it knows about are logically linked as a single unit in R^3. For example, the
// vertex operations in this class will typically operate on ALL vertices in ALL contours as if they were part of a single
// (but separated) structure. See the methods for more info on how multi-contour operations are handled.
template <class T>   class contour_collection {
    public:
        std::list<contour_of_points<T>> contours; 

        //Constructors.
        contour_collection();
        contour_collection(const contour_collection<T> &);
        contour_collection(const std::list<contour_of_points<T>> &);

        //Member functions.
        T Get_Signed_Area(void) const;
        bool Is_Counter_Clockwise(void) const;
        void Reorient_Counter_Clockwise(void);

        vec3<T> Average_Point(void) const;   //This is the average of all the 3D points of all contours. It has little value except simplicity.
        vec3<T> Centroid(void) const;        //This is the centroid of all contours. This is a more mathematically well-defined concept, but still 
                                             // somewhat unreliable for stacked, planar contours. Uses SIGNED area, so mind your orientations.
        vec3<T> Generic_Avg_Boundary_Point(const long int N, const long int M) const; //Useful when dealing with planes for determining if above or below.
        T Perimeter(void) const;             //This is the total perimeter (length) of all contour edges. It is computed from vertex to vertex (positive-definite.)
        T Average_Perimeter(void) const;     //This is the total perimeter (length) of all contour edges / number of contours.
        T Longest_Perimeter(void) const;     //This is the perimeter of the largest contour. It is a positive-definite quantity.

        std::list<contour_collection<T>> Split_Along_Plane(const plane<T> &) const; //Splits the contours along the given plane. This is a "geometric" splitting.
        std::list<contour_collection<T>> Split_Against_Ray(const  vec3<T> &) const; //Splits the contours along the halfway point of a given ray unit vector. This is a "ray casting" splitting.
        std::list<contour_collection<T>> Split_Into_Core_Peel_Spherical(T frac_dist) const; //Splits contour into an inner (core) and outer (peel) using the cc centroid.

        contour_collection & operator= (const contour_collection &);
        bool operator==(const contour_collection &) const; //Relies on the contour_of_points operator==. See caveats there.
        bool operator!=(const contour_collection &) const; 
        bool operator< (const contour_collection &) const; //Relies on the contour_of_points operator< . See caveats there. Only used when sorting.

        contour_collection Resample_Evenly_Along_Perimeter(const long int N) const;     //Per-contour resampling. See contour_of_points implementation.
        contour_collection Resample_LTE_Evenly_Along_Perimeter(const long int N) const; //Per-contour resampling. See contour_of_points implementation.

        long int Avoids_Plane(const plane<T> &P) const; //-1 if contour is below P, 0 if it intersects/crosses it, 1 if it is above.

        void Merge_Adjoining_Contours(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq);

        std::vector<vec3<T>> Generate_Basic_Surface(T min_dist) const; //Tries to find a rudimentary surface from the contour data. If points are sep. by > min_dist, they are considered not coupled.
        std::vector<vec3<T>> Generate_Reconstructed_Surface(T qual) const; //Parameter adjusts how many vertices will be produced. 0 is none, 1 is ~# of contour points, >1 is supersampling.

        void Plot(const std::string &title) const;         //Spits out a default R^3 plot of the data.
        void Plot(void) const;

        std::string write_to_string(void) const;
        bool load_from_string(const std::string &in);      //Returns true if it worked/was loaded into the contour, false otherwise.
};



//---------------------------------------------------------------------------------------------------------------------------
//--------------------- samples_1D: a convenient way to collect a sequentially-sampled array of data ------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is mostly useful as a sort of linear array which can easily handle unevenly-spaced data. 
//
//NOTE: Always ensure the samples are ORDERED in the sequential direction of your choice. No sorting will occur within
// because it is not clear which orientation is required. Some routines require a specific order, so you should be aware
// which!
//
//NOTE: The samples should be single-valued, meaning the algorithms herein (generally) assume that there are NOT two distinct
// samples (ie. varying y-values) at the same point (ie. with same x-values.) To handle this, a sort of per-sample averaging
// scheme should be performed by the user. If you decide to do this, be weary of the weird statistics you may find...
//
//NOTE: Unless otherwise specified, we use the notation [x, f_x] or [x, f(x)] to represent the x and y values of samples.
// 
//NOTE: This class can be derived from to accomplish something like a "fat" function which holds data and, say, fit parameters.
//
template <class T> class samples_1D {
    public:
        std::list<vec2<T>> samples;  //The samples are set up like [x,y] == [location of sample, value of sample] == [x_i, f(x_i)].

        //Constructors.
        samples_1D();
        samples_1D(const std::list<vec2<T>> &samps);
        samples_1D(const samples_1D<T> &in);

        //Member functions.
        void push_back(T, T);    // == this->samples.push_back(vec2<T>(x,y))
        bool empty(void) const;  // == this->samples.empty()
        size_t size(void) const; // == this->samples.size()

        samples_1D<T> Select_Those_Within_Inc(T L, T H) const; //Select those within [L,H] (inclusive).

        T Interpolate_Linearly(const T &x) const;

        void Order_Data_Lowest_First(void); //These functions sort (both x and y) into sequential order, sorting on x-axis.
        void Order_Data_Lowest_Last(void);

        template <class Function> T Integrate_Generic(const samples_1D<T> &in, Function F) const;
        T Integrate_Overlap(const samples_1D<T> &in) const; //Computes the overlap integral: \int_{-inf}^{inf}f(x)g(x)dx.

        // Computes an integral over an exponential kernel: \int_{xmin}^{xmax} f(x)*exp(A*(x+x0))dx.
        T Integrate_Over_Kernel_exp(T xmin, T xmax, T A, T x0) const;

        samples_1D<T> Resample_Average_Into_N_Equal_Sized_Bins(long int N, bool explicitbins = false) const; //Equal-dx bins.
        samples_1D<T> Resample_Average_Into_N_Equal_Datum_Bins(long int N, bool explicitbins = false) const; //Equal-datum bins.

        samples_1D<T> Rank_x(void) const; //Replaces x- (y-)values with (casted-integer) rank. N-plicates get
        samples_1D<T> Rank_y(void) const; // an averaged (maybe non-integer) rank.

        samples_1D<T> Swap_x_and_y(void) const;

        T Average_x(void) const; //aka the mean.
        T Average_y(void) const; //aka the mean.

        //Statistical quantities. These may fail (which is legitimate behaviour in most cases).
        std::tuple<T,T,T,T> Spearmans_Rank_Correlation_Coefficient(bool *OK=nullptr) const; //rho, num of samples, z-value, t-value.

        std::tuple<T,T,T> Compute_Sxy_Sxx_Syy(bool *OK=nullptr) const; //Computes Sxy, Sxx, Syy which are used for linear regression.
        std::tuple<T,T> Linear_Least_Squares_Regression(bool *OK=nullptr) const; //slope (A), intercept (B):  y = x*A + B.

        //~Vector math operations.
        samples_1D<T> operator=(const samples_1D<T> &rhs);
        samples_1D<T> Sum_With(const samples_1D<T> &in) const; //i.e. "operator +=".
        samples_1D<T> Subtract(const samples_1D<T> &in) const; //i.e. "operator -=".


        void Normalize_wrt_Self_Overlap(void); //Normalizes data so that \int_{-inf}^{inf} f(x) (times) f(x) dx ~= 1.

        bool Write_To_File(const std::string &filename) const; //Writes data to file as 2 columns. Use it to plot/fit.
        std::string Write_To_String(void) const; //Writes data to file as 2 columns. Use it to plot/fit.
        void Plot(const std::string &Title = "") const; //Spits out a default plot of the data. 
        void Plot_as_PDF(const std::string &Title = "",const std::string &Filename = "/tmp/plot.pdf") const; //May overwrite existing files!


        //Friends.
//        template<class Y> friend std::ostream & operator << (std::ostream &, const samples_1D<Y> &); // ---> Overloaded stream operators.
//        template<class Y> friend std::istream & operator >> (std::istream &, samples_1D<T> &);
        template<class Y> friend std::ostream & operator << (std::ostream &, const samples_1D<Y> &); // ---> Overloaded stream operators.
        template<class Y> friend std::istream & operator >> (std::istream &, samples_1D<Y> &);           
};

template <class T> samples_1D<T> Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<T> &nums, long int N, bool explicitbins = false);



/*
    COMING SOON: A bounding box which inherits from the contour_of_points class.
    //
    (Then we can move is_point_in_poly to the contour_of_points class and get it for free for bounding boxes!)

*/

#endif

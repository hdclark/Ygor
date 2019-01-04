//YgorMath.h

#ifndef YGOR_MATH_H_
#define YGOR_MATH_H_
#include <stddef.h>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <experimental/optional>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

template <class T> class samples_1D;

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec3: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is the basis of the geometry-heavy classes below. It can happily be used on its own.
template <class T> class vec3 {
    public:
        using value_type = T;
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
        bool isfinite(void) const;        //Logical AND of std::isfinite() on each coordinate.

        //Operators - T typed.
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

        vec3 zero(void) const; //Returns a zero-vector.

        vec3<T> rotate_around_x(T angle_rad) const; // Rotate by some angle (in radians, [0:pi]) around a cardinal axis.
        vec3<T> rotate_around_y(T angle_rad) const;
        vec3<T> rotate_around_z(T angle_rad) const;

        bool GramSchmidt_orthogonalize(vec3<T> &, vec3<T> &) const; //Using *this as seed, orthogonalize (n.b. not orthonormalize) the inputs.

        std::string to_string(void) const;
        vec3<T> from_string(const std::string &in); //Sets *this and returns a copy. 
    
        //Friends.
        template<class Y> friend std::ostream & operator << (std::ostream &, const vec3<Y> &); // ---> Overloaded stream operators.
        template<class Y> friend std::istream & operator >> (std::istream &, vec3<Y> &);      
};


//This is a function for rotation unit vectors in some plane. It requires angles to describe the plane of rotation, angle of rotation. 
// It also requires a unit vector with which to rotate the plane about.
//
// Note: Prefer Gram-Schmidt orthogonalization or the cross-product if you can. This routine has some unfortunate poles...
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
        using value_type = T;
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
        bool isfinite(void) const;        //Logical AND of std::isfinite() on each coordinate.

        //Operators - T typed.
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

        vec2 zero(void) const; //Returns a zero-vector.

        vec2<T> rotate_around_z(T angle_rad) const; // Rotate by some angle (in radians, [0:pi]) around a cardinal axis.

        std::string to_string(void) const;
        vec2<T> from_string(const std::string &in); //Sets *this and returns a copy. 

        //Friends.
        template<class Y> friend std::ostream & operator << (std::ostream &, const vec2<Y> &); // ---> Overloaded stream operators.
        template<class Y> friend std::istream & operator >> (std::istream &, vec2<Y> &);           
};


//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------- line: (infinitely-long) lines in 3D space ---------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   class line {
    public:
        using value_type = T;
        vec3<T> R_0;  //A point which the line intersects.
        vec3<T> U_0;  //A unit vector which points along the length of the line.

        //Constructors.
        line();
        line(const vec3<T> &R_A, const vec3<T> &R_B);

        //Member functions.
        T Distance_To_Point( const vec3<T> & ) const;

        bool Intersects_With_Line_Once( const line<T> &, vec3<T> &) const;

        bool Closest_Point_To_Line( const line<T> &, vec3<T> &) const;

        vec3<T> Project_Point_Orthogonally( const vec3<T> & ) const; // Projects the given point onto the nearest point on the line.
};


//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------ line_segment: (finite-length) lines in 3D space --------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
// The difference between a line segment and a line is that the line segment can only be interpolated between two points,
// whereas a line will extend off to infinity.
template <class T>   class line_segment : public line<T> {
    public:
        using value_type = T;
        T t_0;  //These define the endpoints of the line segment in terms of the parameterization R(t) = R_0 + t*U_0.
        T t_1;  // Whenever endpoints are required, they are explicitly computed. (We can usually get around this, though!)

        //Constructors.
        line_segment();
        line_segment(const vec3<T> &R_A, const vec3<T> &R_B);  //Line segment starts at A (at t=t_0), terminates at B (at t=t_1.)

        //Member functions.
        std::list<vec3<T>> Sample_With_Spacing(T spacing, T offset, T & remaining) const; //Samples every <spacing>, beginning at offset. 
                                                                                          // Returns sampled points and remaining space along segment.

        bool Within_Cylindrical_Volume(const vec3<T> &R, T radius) const; // Checks if the point is within a cylinder centred on the line segment.
        bool Within_Pill_Volume(const vec3<T> &R, T radius) const; // Same as Within_Cylindrical_Volume(), but with spherical endcaps.
 
        vec3<T> Get_R0(void) const;
        vec3<T> Get_R1(void) const;
};

//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------ sphere: a convex 2D surface in 3D space ----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   class sphere {
    public:
        using value_type = T;
        vec3<T> C_0; //The centre of the sphere.
        T r_0; //The radius.

        //Constructors.
        sphere();
        sphere(const vec3<T> &C, T r);

        //Member functions.
        T Distance_To_Point( const vec3<T> & ) const;

        std::vector<vec3<T>> Line_Intersections( const line<T> & ) const; // Can have size 0, 1, or 2.
};

// Fit a sphere to a container of vec3<T> using surface-orthogonal least-squares regression.
template <class C> 
sphere<typename C::value_type::value_type>
Sphere_Orthogonal_Regression( C in,
                              long int max_iterations = 100,
                              typename C::value_type::value_type centre_stopping_tol = static_cast<typename C::value_type::value_type>(1E-5),
                              typename C::value_type::value_type radius_stopping_tol = static_cast<typename C::value_type::value_type>(1E-5));

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------- plane: 2D planes in 3D space -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//A 3D plane can be described completely with the relation:  $\vec{N}_{0} \cdot ( \vec{R} - \vec{R}_{0} ) = 0$ (where $\vec{R}$ 
// is a point on the plane if it satisfies this identity.)

template <class T>   class plane {
    public:
        using value_type = T;
        vec3<T> N_0;  //A unit vector normal to the plane. The orientation is up to the user to choose (and be consistent with!)
        vec3<T> R_0;  //A 3D point the plane intersects.

        //Constructors.
        plane();
        plane(const plane<T> &P);
        plane(const vec3<T> &N_0_in, const vec3<T> &R_0_in);

        //Member functions.
        T Get_Signed_Distance_To_Point(const vec3<T> &R) const;
        bool Is_Point_Above_Plane(const vec3<T> &R) const;      //Essentially returns the sign. True if pointing in direction of plane's 
                                                                // positive normal, false if in opposite direction. Do not rely on any sign
                                                                // in case the point is in the plane (but a consistent sign IS given in this case.)
        bool Intersects_With_Line_Once(const line<T> &L, vec3<T> &out) const;

        //Project point (along a normal to this plane) onto a point on this plane.
        vec3<T> Project_Onto_Plane_Orthogonally(const vec3<T> &point) const;

};

// Fit a plane to a container of vec3<T> using plane-orthogonal least-squares regression.
template <class C> 
plane<typename C::value_type::value_type>
Plane_Orthogonal_Regression(C in);


//---------------------------------------------------------------------------------------------------------------------------
//------------------ contour_of_points: a polygon of line segments in the form of a collection of points --------------------
//---------------------------------------------------------------------------------------------------------------------------
//Simple structure representing a polygon of line segments in the form of a collection of points in 3D space (using vec3<...>'s.) 
// Alternatively, they can be thought of as the perimeter of a (2D) structure, though enforcement of this interpretation is
// mostly up to the user.
//
//The points may or may not form a closed surface (ie. the first and last points are connected.) They must be marked as being either.
//
//NOTE: This class holds a *single* contour, represented as a list of points in R^3.
template <class T>   class contour_of_points {
    public:
        std::list<vec3<T>> points; //We use doubly-linked lists because the points can then be re-ordered, inserted or removed, or shuffled around with small cost.
                                   // Additionally, we typically do not random access the points, but instead iterate over them (drawing them, computing areas, etc..
  
        bool closed;               //Are the first and last points connected via a line? For instance, we cannot compute the area of the line segment if they are not.


        std::map<std::string,std::string> metadata; //User-defined metadata.


        //Constructors.
        contour_of_points();
        contour_of_points(std::list<vec3<T>> points);
        contour_of_points(const contour_of_points<T> &in);

        //Member functions.
//        void append(const vec3<T> &point);
        T Get_Signed_Area(bool AssumePlanarContours = false) const; //Generally use the default option! See note in source for details.
        bool Is_Counter_Clockwise(void) const;
        void Reorient_Counter_Clockwise(void);

        vec3<T> Average_Point(void) const;   //This is the average of all the 3D points of the contour. It has little value except simplicity.
        vec3<T> Centroid(void) const;        //This is the centroid of the contour. This is a more mathematically well-define concept. See the source.
        vec3<T> First_N_Point_Avg(const long int N) const; //Useful when dealing with planes for determining if above or below.
        T Perimeter(void) const;             //This is the perimeter (length) of the contour edges. It is computed from vertex to vertex (positive-definite.)

        vec3<T> Get_Point_Within_Contour(void) const; //Get an arbitrary but deterministic point within the contour.
 
        contour_of_points<T>  Bounding_Box_Along(const vec3<T> &, T margin = (T)(0.0)) const; //Gives a (planar 2D) bounding box with two edges aligned with the given unit vector. 

        std::list<contour_of_points<T>> Split_Along_Plane(const plane<T> &) const; //Splits the contour into individual contours along the given plane. This is a "geometric" splitting.
        std::list<contour_of_points<T>> Split_Against_Ray(const  vec3<T> &) const; //Splits the contour into individual contours along the halfway point of a given ray unit vector. This is a "ray casting" splitting.
        std::list<contour_of_points<T>> Split_Into_Core_Peel_Spherical(const vec3<T> &, T frac_dist) const; //Splits contour into an inner (core) and outer (peel).

        T Integrate_Simple_Scalar_Kernel(std::function<    T    (const vec3<T> &r, const vec3<T> &A, const vec3<T> &B, const vec3<T> &U)> k) const;
        T Integrate_Simple_Vector_Kernel(std::function< vec3<T> (const vec3<T> &r, const vec3<T> &A, const vec3<T> &B, const vec3<T> &U)> k) const;

        contour_of_points<T> Resample_Evenly_Along_Perimeter(const T dl) const; //Walks perimeter creating new points (via interpolation) every [dl].
        contour_of_points<T> Resample_Evenly_Along_Perimeter(const long int N) const; //Walks perimeter creating new points (via interpolation) every [perimeter]/N .
                                                                                      // Points will be homogeneously distributed iff the shape is circular.

        contour_of_points<T> Subdivide_Midway(void) const; //Inserts an extra vertex at the midway point between all vertices.

        contour_of_points<T> Remove_Vertices(T area_threshold) const; //Remove adjacent vertices until the cumulative change in (absolute) area exceeds the threshold.
        contour_of_points<T> Collapse_Vertices(T area_threshold) const; //Collapse adjacent vertices into a single vertex until the cumulative change in (absolute) area exceeds the threshold.

        contour_of_points<T> Scale_Dist_From_Point(const vec3<T> &, T scale) const;   //Scales distance from each point to given point by factor (scale).

        contour_of_points<T> Resample_LTE_Evenly_Along_Perimeter(const long int N) const; //Performs Resample_Evenly_Along_Perimeter only on contours with more than N.

        //Compute the least-squares best-fit plane through the contour points with the provided normal. Helpful for later projection.
        plane<T> Least_Squares_Best_Fit_Plane(const vec3<T> &plane_normal) const;

        //Maintain connectivity, but project each point onto the given plane (along a plane normal). Returns empty if impossible.
        contour_of_points<T> Project_Onto_Plane_Orthogonally(const plane<T> &P) const;

        //Estimates the planar normal using a few vertices. 
        vec3<T> Estimate_Planar_Normal() const;

        //Check if the given point lies in a 2D polygon defined by the orthogonal projection (of both contour and point) onto a given plane.
        // This routine entirely ignores the direction orthogonal to the plane, so filter contours at the wrong 'height' beforehand for 3D!
        bool Is_Point_In_Polygon_Projected_Orthogonally(const plane<T> &plane, 
                                                        const vec3<T> &point, 
                                                        bool AlreadyProjected = false,
                                                        T boundary_eps = std::sqrt(std::numeric_limits<T>::epsilon())) const;



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

        bool MetadataKeyPresent(std::string key) const; //Checks if the key is present without inspecting the value.

        //Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
        template <class U> std::experimental::optional<U> GetMetadataValueAs(std::string key) const;

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
//NOTE: This class assumes that ALL contours it knows about are logically linked as a single unit in R^3 (but not necessarily
//      *physically* linked). For example, the vertex operations in this class will typically operate on ALL vertices in ALL 
//      contours as if they were part of a single (but possibly spaitally separated) structure. 
//
//NOTE: Some methods make assumptions about the meaning of the contours. For example, computing a "volume" requires the user 
//      to provide some information about how the contours are connected ("stacked" and "attached"). In lieu of this, we can
//      usually assume the contours have some finite, uniform thickness along \hat{z}. The user must be aware of the 
//      assumptions each method makes. See individual methods for more info on how such multi-contour operations are handled.
template <class T>   class contour_collection {
    public:
        std::list<contour_of_points<T>> contours; 

        //Constructors.
        contour_collection();
        contour_collection(const contour_collection<T> &);
        contour_collection(std::list<contour_of_points<T>> );

        //Member functions.
        void Consume_Contours(contour_collection<T> &);

        T Get_Signed_Area(bool AssumePlanarContours = false) const; //Generally use the default option! See planar_contour version for details.
        bool Is_Counter_Clockwise(void) const;
        void Reorient_Counter_Clockwise(void);

        vec3<T> Average_Point(void) const;   //This is the average of all the 3D points of all contours. It has little value except simplicity.
        vec3<T> Centroid(bool AssumePlanarContours = false) const; //This is the centroid of all contours. This is a mathematically well-defined
                                             // concept, but slightly unintuitive for stacked, planar contours. Uses SIGNED area, so mind
                                             // your orientations.

        vec3<T> Generic_Avg_Boundary_Point(const long int N, const long int M) const; //Useful when dealing with planes for determining if above or below.
        T Perimeter(void) const;             //This is the total perimeter (length) of all contour edges. It is computed from vertex to vertex (positive-definite.)
        T Average_Perimeter(void) const;     //This is the total perimeter (length) of all contour edges / number of contours.
        T Longest_Perimeter(void) const;     //This is the perimeter of the largest contour. It is a positive-definite quantity.

        T Slab_Volume(T thickness, bool IgnoreContourOrientation = false) const; //Computes volume assuming each contour is a slab with the given thickness. Ignores overlap, so carefully figure out the thickness parameter.

        std::list<contour_collection<T>> Split_Along_Plane(const plane<T> &) const; //Splits the contours along the given plane. This is a "geometric" splitting.
        std::list<contour_collection<T>> Split_Against_Ray(const  vec3<T> &) const; //Splits the contours along the halfway point of a given ray unit vector. This is a "ray casting" splitting.
        std::list<contour_collection<T>> Split_Into_Core_Peel_Spherical(T frac_dist) const; //Splits contour into an inner (core) and outer (peel) using the cc centroid.

        //Split contours using a planar splitting approach so that an arbitrary fraction of the total planar area is above
        // the target plane. (Does not do full volumetric slicing!)
        std::list<contour_collection<T>> Total_Area_Bisection_Along_Plane(const vec3<T> &planar_unit_normal,
                                                T desired_total_area_fraction_above_plane,
                                                T acceptable_frac_deviation = static_cast<T>(0.01), // 0.01 = 1% of total area.
                                                size_t max_iters = 100,
                                                plane<T> *final_plane = nullptr, // leave nullptr to ignore.
                                                size_t *iters_taken   = nullptr, // leave nullptr to ignore.
                                                T *final_area_frac    = nullptr, // leave nullptr to ignore.
                                                vec3<T> lower_bound = vec3<T>(std::numeric_limits<T>::quiet_NaN(),
                                                                              std::numeric_limits<T>::quiet_NaN(),
                                                                              std::numeric_limits<T>::quiet_NaN() ), 
                                                vec3<T> upper_bound = vec3<T>(std::numeric_limits<T>::quiet_NaN(),
                                                                              std::numeric_limits<T>::quiet_NaN(),
                                                                              std::numeric_limits<T>::quiet_NaN() ) ) const; 

        contour_collection & operator= (const contour_collection &);
        bool operator==(const contour_collection &) const; //Relies on the contour_of_points operator==. See caveats there.
        bool operator!=(const contour_collection &) const; 
        bool operator< (const contour_collection &) const; //Relies on the contour_of_points operator< . See caveats there. Only used when sorting.

        contour_collection Resample_Evenly_Along_Perimeter(const long int N) const;     //Per-contour resampling. See contour_of_points implementation.
        contour_collection Resample_LTE_Evenly_Along_Perimeter(const long int N) const; //Per-contour resampling. See contour_of_points implementation.

        long int Avoids_Plane(const plane<T> &P) const; //-1 if contour is below P, 0 if it intersects/crosses it, 1 if it is above.

        void Merge_Adjoining_Contours(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq);

        void Purge_Contours_Below_Point_Count_Threshold(size_t N = 3); //Removes contours if they have < N points. Duplicate points not considered.

        void Insert_Metadata(const std::string &key, const std::string &val); //For all contours. Overwrites if existing keys present.

        std::map<std::string,std::string> get_common_metadata(const std::list<std::reference_wrapper<contour_collection<T>>> &ccl,
                                                              const std::list<std::reference_wrapper<contour_of_points<T>>> &copl); //Uses both *this and input.
        std::list<std::string> get_all_values_for_key(const std::string &akey) const;
        std::list<std::string> get_unique_values_for_key(const std::string &akey) const;

        void Plot(const std::string &title) const;         //Spits out a default R^3 plot of the data.
        void Plot(void) const;

        std::string write_to_string(void) const;
        bool load_from_string(const std::string &in);      //Returns true if it worked/was loaded into the contour, false otherwise.
};

template <class T>
vec3<T>
Average_Contour_Normals(const std::list<std::reference_wrapper<contour_collection<T>>> &ccs);

template <class T>
std::list<plane<T>> 
Unique_Contour_Planes(const std::list<std::reference_wrapper<contour_collection<T>>> &ccs,
                      const vec3<T> &N, 
                      T distance_eps = static_cast<T>(1E-3));

template <class T>
T
Estimate_Contour_Separation(const std::list<std::reference_wrapper<contour_collection<T>>> &ccs,
                            const vec3<T> &N,
                            T distance_eps = static_cast<T>(1E-3));


//---------------------------------------------------------------------------------------------------------------------------
//-------------- lin_reg_results: a simple helper class for dealing with output from linear regression routines -------------
//---------------------------------------------------------------------------------------------------------------------------
//This routine is a helper class for holding the results from either weighted or un-weighted linear regression routines.
// It mostly holds data as members, but also has some routines for using the results to interpolate and pretty-printing.
//
// NOTE: All parameters should be inspected for infinity and NaN before using in any calculations. Not all members are 
//       filled by each routine.
//
// NOTE: Not all data is always used for all routines. The weighted regression routines will omit some values (e.g., NaNs)
//       and possibly unweighted (i.e., infinitely surpressed) values. Always check N or dof instead of ...size().
//
// NOTE: For more info the parameters here, look at the regression routine notes. They are detailed in context there.
//
template <class T> class lin_reg_results {
    public:
        // ===================================== Fit parameters ========================================
        T slope             = std::numeric_limits<T>::quiet_NaN(); //m in y=m*x+b.
        T sigma_slope       = std::numeric_limits<T>::quiet_NaN(); //Uncertainty in m.
        T intercept         = std::numeric_limits<T>::quiet_NaN(); //b in y=m*x+b.
        T sigma_intercept   = std::numeric_limits<T>::quiet_NaN(); //Uncertainty in b.
    
        // ================= Quantities related to the fit and/or distribution of data =================
        T N                 = std::numeric_limits<T>::quiet_NaN(); //# of datum used (not the DOF, int casted to a T).
        T dof               = std::numeric_limits<T>::quiet_NaN(); //# of degrees of freedom used.
        T sigma_f           = std::numeric_limits<T>::quiet_NaN(); //Std. dev. of the data.
        T covariance        = std::numeric_limits<T>::quiet_NaN(); //Covariance.
        T lin_corr          = std::numeric_limits<T>::quiet_NaN(); //Linear corr. coeff. (aka "r", "Pearson's coeff").

        // -------------------------------- un-weighted regression only --------------------------------
        T sum_sq_res        = std::numeric_limits<T>::quiet_NaN(); //Sum-of-squared residuals.
        T tvalue            = std::numeric_limits<T>::quiet_NaN(); //t-value for r.
        T pvalue            = std::numeric_limits<T>::quiet_NaN(); //Two-tailed p-value for r.

        // --------------------------------- weighted regression only ----------------------------------
        T chi_square        = std::numeric_limits<T>::quiet_NaN(); //Sum-of-squared residuals weighted by sigma_f_i^{-2}.
        T qvalue            = std::numeric_limits<T>::quiet_NaN(); //q-value for chi-square.
        T cov_params        = std::numeric_limits<T>::quiet_NaN(); //Covariance of the slope and intercept.
        T corr_params       = std::numeric_limits<T>::quiet_NaN(); //Correlation of sigma_slope and sigma_intercept.

        // ========== Quantities which might be of interest for computing other quantities =============

        // -------------------------------- un-weighted regression only --------------------------------
        T mean_x            = std::numeric_limits<T>::quiet_NaN(); //Mean of all x_i for used datum.
        T mean_f            = std::numeric_limits<T>::quiet_NaN(); //Mean of all f_i for used datum.
        T sum_x             = std::numeric_limits<T>::quiet_NaN(); //Sum of x_i.
        T sum_f             = std::numeric_limits<T>::quiet_NaN(); //Sum of f_i.
        T sum_xx            = std::numeric_limits<T>::quiet_NaN(); //Sum of x_i*x_i.
        T sum_xf            = std::numeric_limits<T>::quiet_NaN(); //Sum of f_i*f_i.
        T Sxf               = std::numeric_limits<T>::quiet_NaN(); //Sum of (x_i - mean_x)*(f_i - mean_f).
        T Sxx               = std::numeric_limits<T>::quiet_NaN(); //Sum of (x_i - mean_x)^2.
        T Sff               = std::numeric_limits<T>::quiet_NaN(); //Sum of (f_i - mean_f)^2.

        // --------------------------------- weighted regression only ----------------------------------
        // NOTE: sigma_f'_i is an equivalent sigma_f_i which incorporates sigma_f_i and sigma_x_i using a rough guess
        //       of the slope. 
        T mean_wx           = std::numeric_limits<T>::quiet_NaN(); //Mean of all x_i/(sigma_f'_i^2) for used datum.
        T mean_wf           = std::numeric_limits<T>::quiet_NaN(); //Mean of all f_i/(sigma_f'_i^2) for used datum.

        T sum_w             = std::numeric_limits<T>::quiet_NaN(); //Sum of 1/(sigma_f'_i^2).
        T sum_wx            = std::numeric_limits<T>::quiet_NaN(); //Sum of x_i/(sigma_f'_i^2).
        T sum_wf            = std::numeric_limits<T>::quiet_NaN(); //Sum of f_i/(sigma_f'_i^2).
        T sum_wxx           = std::numeric_limits<T>::quiet_NaN(); //Sum of x_i*x_i/(sigma_f'_i^2).
        T sum_wxf           = std::numeric_limits<T>::quiet_NaN(); //Sum of f_i*f_i/(sigma_f'_i^2).

        // ==================================== Member functions =======================================
        lin_reg_results();

        //Evaluates the model at the given point. Ignores uncertainties.
        T evaluate_simple(T x) const;

        //Sample the line over the given range at 'n' equally-spaced points. Ignores uncertainties.
        samples_1D<T> sample_uniformly_over(T xmin, T xmax, size_t n = 5) const;

        //Aligns into a simple table.
        std::string display_table(void) const;

        //Compare all members which aren't NAN. Aligns into a simple table.
        std::string comparison_table(const lin_reg_results<T> &in) const;
};


//---------------------------------------------------------------------------------------------------------------------------
//--------------------- samples_1D: a convenient way to collect a sequentially-sampled array of data ------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is mostly useful as a sort of linear array which can easily handle unevenly-spaced data like a list of
// [x_i,f_i]. Uncertainties in both x_i and f_i are supported, but uncertainties in f_i are not always taken into account
// (e.g., during interpolation) and uncertainties in x_i are often outright ignored. Error propagation is attempted whereever
// possible, but many concepts do not translate easily to uncertainty propagation (i.e., spline interpolation, combining 
// smoothing uncertainties with true uncertainties). So be careful to inspect and verify uncertainties are being treated 
// the way you think they are.
//
// Uncertainties are propagated using the standard approach where you assume the uncertainties are small and use the
// derivative to scale how much uncertainty is propagated. Covariance is not taken into account, but there is an option to
// forgo any assumptions (other than that they are small, and that the approximation of various computations are approx.
// linear over the scale of the uncertainties). This is described below.
//
// Room for uncertainties is always allocated to simplify the implementation and eliminate ambiguity (i.e., if handed a vec3,
// which is the uncertainty? vec3[1] or vec3[2]? And does it refer to x_i or f_i?). If uncertainties are not explicitly
// provided they default to (T)(0). If you opt to ignore uncertainties in specific routines, it is undefined what they will 
// contain, but it will probably be (T)(0). Uncertainties of (T)(0) are understood to mean no uncertainty, but the routines
// will often treat these numbers as infinitely-certain quantitites. This may have surprising effects if you aren't 
// prepared. If you really don't want to deal with uncertainties, it is best to set them all to (T)(1).
//
// Uncertainties are treated one of two ways, configurable by setting a class-member flag:
//
//   1. As independent and random, normally-distributed errors. In this case we treat them as one-sigma = 68% confidence 
//      intervals and *assume* the normality has been checked. Note that systematic errors are NOT random errors. (So be 
//      careful when figuring out whether your uncertainties are random or systematic!)
//
//      In particular, uncertainties will be propagated using quadrature rules.
//
//   2. As maximal 'uncertainties' which might possibly be correlated and non-random. In this case, no assumptions are made
//      about the nature of the uncertainties. We therefore compute the theoretical maximum uncertainties. This is always 
//      the safest approach when the data is cannot be properly assessed/inspected, but will probably result in excessively
//      large resultant uncertainties. Note that this approach encompasses, to my knowledge, all scenarios: variance, co-
//      variance, non-random, and (I think) even systematic biases.
//
//      In particular, uncertainties will be propagated using ordinary sums.
// 
// Most operations that involve generating a new samples_1D from *this will inherit the uncertainty flavour flag. This 
// doesn't always make sense, so you should be vigilant about your uncertainties if you are invested in them.
//
//
//NOTE: For info on the error propagation used herein, including derivations and rationale for the formulae used, consult 
//      chapter 3 of John R. Taylor's "An Introduction to Error Analysis - The Study of Uncertainties in Physical 
//      Measurements", Second Edition. The mean, average, and weighted mean are covered in chapters 4 and 7. Covariances 
//      and correlation are, at the time of writing, completely ignored. (They could be added in if needed, but it seems
//      difficult to apply to the general case b/c it's sometimes hard to assess whether two objects are the same or 
//      refer to the same thing.) Chapter 8 covers the weighted linear regression I implement and discusses how sigma_x_i
//      are taken into account using a two-stage estimation approach.
//
//NOTE: Samples are always ordered lowest-x_i-first. An internal stable_sort will ensure this is the case, but you can
//      easily fool the implementation by accessing memory behind its back. Don't do this unless you know exactly what 
//      you're doing. It is better for you to explicitly sort because samples with the same x-value are tricky to handle in
//      some situations. The sort is run almost every push_back, so you might want to inhibit it while pushing back lots of 
//      datum.
//
//NOTE: If possible, ensure there are not samples with the same x-value. The algorithms herein (generally) assume that there 
//      are NOT two distinct samples (ie. varying y-values) at the same point (ie. with same x-values.) In some routines this
//      is OK. Combining datum with equal x_i is tricky, because we cannot tell if they truly are measurements of the same
//      quantity or not. Thus we don't have a general way of combining like-x_i datum. But you probably can: Try binning 
//      your samples, or fixing the problematic points. If you decide to proceed with samples with the same x-value,
//      be weary that some assumptions might be broken and buggy results will likely occur.
//
//NOTE: This class can be derived from to accomplish something like a "fat" function which holds data and, say, fit parameters.
//      I'm not against the idea of a user-provided interpolation kernel, or fitting function, or whatever. Go for it.
//
template <class T> class samples_1D {
    public:
        //--------------------------------------------------- Data members -------------------------------------------------
        //Samples are laid out like [x_i, sigma_x_i, f_i, sigma_f_i] sequentially, so if there are three samples data will be
        // laid out in memory like:
        //   [x_0, sigma_x_0, f_0, sigma_f_0, x_1, sigma_x_1, f_1, sigma_f_1, x_2, sigma_x_2, f_2, sigma_f_2 ].
        // and can thus be used as a c-style array if needed. If you can pass stride and offset (e.g., to GNU GSL), use:
        //   - for x_i:        stride = 4,  offset from this->samples.data() to first element = 0. 
        //   - for sigma_x_i:  stride = 4,  offset from this->samples.data() to first element = 1. 
        //   - for f_i:        stride = 4,  offset from this->samples.data() to first element = 2. 
        //   - for sigma_f_i:  stride = 4,  offset from this->samples.data() to first element = 3. 
        std::vector<std::array<T,4>> samples;

        //This flag controls how uncertainties are treated. By default, no assumptions about normality are made. If you are
        // certain that the sample uncertainties are: independent, random, and normally-distributed (i.e., no systematic 
        // biases are thought to be present, and the underlying measurement process follows a normal distribution) then this
        // flag can be set to reduce undertainties in accordance with the knowledge that arithmetical combinations of values 
        // tend to cancel uncertainties. Be CERTAIN to actually inspect this fact!
        //
        // Conversely, setting this flag may under- or over-value uncertainties because we completely ignore covariance.
        // If it is false, you at least know for certain that you are over-valuing uncertainties. Be aware that BOTH 
        // settings imply that the uncertainties are considered small enough that a linear error propagation procedure 
        // remains approximately valid!
        bool uncertainties_known_to_be_independent_and_random = false;        


        //User-defined metadata, useful for packing along extra information about the data. Be aware that the data can
        // become stale if you are not updating it as needed!
        std::map<std::string,std::string> metadata;


        //-------------------------------------------------- Constructors --------------------------------------------------
        samples_1D();
        samples_1D(const samples_1D<T> &in);
        samples_1D(std::vector<std::array<T,4>> samps);

        //Providing [x_i, f_i] data. Assumes sigma_x_i and sigma_f_i uncertainties are (T)(0).
        samples_1D(const std::list<vec2<T>> &samps);

        //------------------------------------------------ Member Functions ------------------------------------------------
        samples_1D<T> operator=(const samples_1D<T> &rhs);

        //If not supplied, sigma_x_i and sigma_f_i uncertainties are assumed to be (T)(0).
        void push_back(T x_i, T sigma_x_i, T f_i, T sigma_f_i, bool inhibit_sort = false);
        void push_back(const std::array<T,4> &samp, bool inhibit_sort = false);
        void push_back(const std::array<T,2> &x_dx, const std::array<T,2> &y_dy, bool inhibit_sort = false);
        void push_back(T x_i, T f_i, bool inhibit_sort = false);
        void push_back(const vec2<T> &x_i_and_f_i, bool inhibit_sort = false);
        void push_back(T x_i, T f_i, T sigma_f_i, bool inhibit_sort = false);

        bool empty(void) const;  // == this->samples.empty()
        size_t size(void) const; // == this->samples.size()

        //An explicit way for the user to sort. Not needed unless you are directly editing this->samples for some reason.
        void stable_sort(void); //Sorts on x-axis. Lowest-first.

        //Get the datum with the minimum and maximum x_i or f_i. If duplicates are found, there is no rule specifying which.
        std::pair<std::array<T,4>,std::array<T,4>> Get_Extreme_Datum_x(void) const;
        std::pair<std::array<T,4>,std::array<T,4>> Get_Extreme_Datum_y(void) const;

        //Normalizes data so that \int_{-inf}^{inf} f(x) (times) f(x) dx = 1 multiplying by constant factor.
        void Normalize_wrt_Self_Overlap(void);

        //Sets uncertainties to zero. Useful in certain situations, such as computing aggregate std dev's.
        samples_1D<T> Strip_Uncertainties_in_x(void) const;
        samples_1D<T> Strip_Uncertainties_in_y(void) const;

        //Ensure there is a single datum with the given x_i (within +-eps), averaging coincident data if necessary.
        void Average_Coincident_Data(T eps);

        //Replaces {x,y}-values with rank. {dup,N}-plicates get an averaged (maybe non-integer) rank.
        samples_1D<T> Rank_x(void) const;
        samples_1D<T> Rank_y(void) const;

        //Swaps x_i with f_i and sigma_x_i with sigma_f_i.
        samples_1D<T> Swap_x_and_y(void) const;
        
        //Sum of all x_i or f_i. Uncertainties are propagated properly.
        std::array<T,2> Sum_x(void) const;
        std::array<T,2> Sum_y(void) const;

        //Computes the [statistical mean (average), std dev of the *data*]. Be careful to choose the MEAN or AVERAGE 
        // carefully -- the uncertainty is different! See source for more info.
        std::array<T,2> Average_x(void) const; 
        std::array<T,2> Average_y(void) const;

        //Computes the [statistical mean (average), std dev of the *mean*]. Be careful to choose the MEAN or AVERAGE 
        // carefully -- the uncertainty is different! See source for more info.
        std::array<T,2> Mean_x(void) const;
        std::array<T,2> Mean_y(void) const;

        //Computes the [statistical weighted mean (average), std dev of the *weighted mean*]. See source. 
        std::array<T,2> Weighted_Mean_x(void) const;
        std::array<T,2> Weighted_Mean_y(void) const;

        //Finds or computes the median datum with linear interpolation halfway between datum if N is odd.
        std::array<T,2> Median_x(void) const;
        std::array<T,2> Median_y(void) const;

        //Select those within [L,H] (inclusive). Ignores sigma_x_i for inclusivity purposes. Keeps uncertainties intact.
        samples_1D<T> Select_Those_Within_Inc(T L, T H) const;

        //Returns [at_x, sigma_at_x (==0), f, sigma_f] interpolated at at_x. If at_x is not within the range of the 
        // samples, expect {at_x,(T)(0),(T)(0),(T)(0)}.
        std::array<T,4> Interpolate_Linearly(const T &at_x) const;

        //Returns linearly interpolated crossing-points.
        samples_1D<T> Crossings(T value) const;

        //Returns the locations linearly-interpolated peaks.
        samples_1D<T> Peaks(void) const;

        //Resamples the data into approximately equally-spaced samples using linear interpolation.
        samples_1D<T> Resample_Equal_Spacing(size_t N) const;

        //Multiply all sample f_i's by a given factor. Uncertainties are appropriately scaled too.
        samples_1D<T> Multiply_With(T factor) const; // i.e., "operator *".
        samples_1D<T> Sum_With(T factor) const; // i.e., "operator +".

        //Apply an absolute value functor to all f_i.
        samples_1D<T> Apply_Abs(void) const;

        //Shift all x_i's by a factor. No change to uncertainties.
        samples_1D<T> Sum_x_With(T dx) const;
        samples_1D<T> Multiply_x_With(T dx) const;
 
        //Distribution operators. Samples are retained, so linear interpolation is used.
        samples_1D<T> Sum_With(const samples_1D<T> &in) const; //i.e. "operator +".
        samples_1D<T> Subtract(const samples_1D<T> &in) const; //i.e. "operator -".

        //Purge non-finite samples.
        samples_1D<T> Purge_Nonfinite_Samples(void) const;

        //Generic driver for numerical integration. See source for more info.
        template <class Function> std::array<T,2> Integrate_Generic(const samples_1D<T> &in, Function F) const;

        //Computes the overlap integral: \int_{-inf}^{inf}f(x)g(x)dx. Also returns uncertainty estimate. See source.
        std::array<T,2> Integrate_Overlap(const samples_1D<T> &in) const;

        //Computes the integral: \int_{-inf}^{+inf} f(x) dx and uncertainty estimate. See source.
        std::array<T,2> Integrate_Over_Kernel_unit() const;

        //Computes the integral: \int_{xmin}^{xmax} f(x) dx and uncertainty estimate. See source.
        std::array<T,2> Integrate_Over_Kernel_unit(T xmin, T xmax) const;

        //Computes integral over exp. kernel: \int_{xmin}^{xmax} f(x)*exp(A*(x+x0))dx and uncertainty estimate. See source.
        std::array<T,2> Integrate_Over_Kernel_exp(T xmin, T xmax, std::array<T,2> A, std::array<T,2> x0) const;

        //Group datum into (N) equal-dx bins, reducing variance. Ignores sigma_x_i. *Not* a histogram! See source.
        samples_1D<T> Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool explicitbins = false) const;

        //Group datum into equal-datum (N datum per bin) bins, reducing variance. *Not* a histogram! See source.
        samples_1D<T> Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const; //Handles all uncertainties.

        //Generate histogram with (N) equal-dx bins. Bin height is sum of f_i in bin. Propagates sigma_f_i, ignores sigma_x_i.
        samples_1D<T> Histogram_Equal_Sized_Bins(long int N, bool explicitbins = false) const;

        //Spearman's Rank Correlation Coefficient. Ignores uncertainties. Returns: {rho, # of datum, z-value, t-value}.
        std::array<T,4> Spearmans_Rank_Correlation_Coefficient(bool *OK=nullptr) const;

        //Computes {Sxy,Sxx,Syy} which are used for linear regression and other procedures.
        std::array<T,3> Compute_Sxy_Sxx_Syy(bool *OK=nullptr) const;

        //Computes a weighted, two-side, moving average or mean of the f_i. Ignores x_i spacing. This is a convolution
        // that effectively acts like a low-pass filter. OK to apply consecutively. This driver can be used directly.
        // Ignores x_i spacing and sigma_x_i.
        samples_1D<T> Moving_Average_Two_Sided_Driver(const std::vector<T> &weights) const;

        //Computes a "Spencer's 15-point weighting, two-sided moving average" or mean of the f_i. Ignores x_i, sigma_x_i. 
        // This is a convolution that effectively acts like a low-pass filter. OK to apply consecutively. Lets polynomials 
        // of order <=3 through ~as-is.
        samples_1D<T> Moving_Average_Two_Sided_Spencers_15_point(void) const;

        //Computes a "Henderson's 23-point weighting, two-sided  moving average" or mean of the f_i. Ignores x_i, sigma_x_i. 
        // This is a convolution that effectively acts like a low-pass filter. OK to apply consecutively. I *think* it lets
        // polynomials of order <=3 through ~as-is, but have not verified.
        samples_1D<T> Moving_Average_Two_Sided_Hendersons_23_point(void) const;

        //Computes a (2N+1)-point, all-points-in-window-are-equally-weighted, two-sided moving average or mean of the f_i. This 
        // is a convolution that effectively acts like a low-pass filter. OK to apply consecutively. Ignores x_i, sigma_x_i.
        samples_1D<T> Moving_Average_Two_Sided_Equal_Weighting(long int N) const;

        //Performs a "Gaussian blur" by convolving a Gaussian kernel over the nearby data points. This just boils down to another
        // weighted, two-sided, moving average. It is a low-pass filter. Applying consecutively is equiv. to applying once with
        // a stronger strength. Ignores x_i, sigma_x_i.
        samples_1D<T> Moving_Average_Two_Sided_Gaussian_Weighting(T datum_sigma) const;

        //Performs a weighted "median filter" over the data; essentially a two-sided moving average with the mean replaced 
        // by the median. It is a low-pass filter. Ignores x_i, sigma_x_i. Analogous to Moving_Average_Two_Sided_Driver().
        samples_1D<T> Moving_Median_Filter_Two_Sided_Driver(const std::vector<uint64_t> &weights) const;

        //Applies a (2N+1)-point, all-points-in-window-are-equally-weighted, two-sided moving median filter over f_i. This 
        // is a low-pass filter. OK to apply consecutively. Ignores x_i, sigma_x_i.
        samples_1D<T> Moving_Median_Filter_Two_Sided_Equal_Weighting(long int N) const;

        //Applies a Gaussian-weighted kernel over the nearby data points. It is a low-pass filter. Ignores x_i, sigma_x_i.
        samples_1D<T> Moving_Median_Filter_Two_Sided_Gaussian_Weighting(T datum_sigma) const;

        //Applies a (2N+1)-point, triangular-weighted, two-sided moving median filter over f_i. This 
        // is a low-pass filter. OK to apply consecutively. Ignores x_i, sigma_x_i.
        samples_1D<T> Moving_Median_Filter_Two_Sided_Triangular_Weighting(long int N) const;


        //Calculates an unbiased estimate of a population's variance over a window of (2N+1) points. Endpoints use fewer 
        // points (min = N) and have higher variance. If treatment of endpoints bothers you, consider doing a discrete 
        // binning instead of windowing. Ignores x_i spacing, sigma_x_i, *and* sigma_f_i! Setting N >= 5 is recommended.
        samples_1D<T> Moving_Variance_Two_Sided(long int N) const;


        //Calculates the discrete derivative using forward finite differences. (The right-side endpoint uses backward 
        // finite differences to handle the boundary.) Data should be reasonably smooth -- no interpolation is used.
        samples_1D<T> Derivative_Forward_Finite_Differences(void) const;

        //Calculates the discrete derivative using backward finite differences. (The right-side endpoint uses forward 
        // finite differences to handle the boundary.) Data should be reasonably smooth -- no interpolation is used.
        samples_1D<T> Derivative_Backward_Finite_Differences(void) const;

        //Calculates the discrete derivative using centered finite differences. (The endpoints use forward and backward 
        // finite differences to handle boundaries.) Data should be reasonably smooth -- no interpolation is used.
        samples_1D<T> Derivative_Centered_Finite_Differences(void) const;

        //Calculates the local signed curvature at x_i using adjacent nearest-neighbour datum. Endpoints are dropped. Both 
        // sigma_x_i and sigma_f_i are propagated. |Curvature| is the tangent circle's inverse radius.
        samples_1D<T> Local_Signed_Curvature_Three_Datum(void) const;


        //Standard linear (y=m*x+b) regression. Ignores all provided uncertainties. Prefer weighted regression. See source.
        lin_reg_results<T> Linear_Least_Squares_Regression(bool *OK=nullptr, bool SkipExtras = false) const;

        //More sophisticated linear regression. Takes into account both sigma_x_i and sigma_f_i uncertainties to compute
        // a weighted linear regression using likelihood-maximizing weighting. See source for more info.
        lin_reg_results<T> Weighted_Linear_Least_Squares_Regression(bool *OK=nullptr, T *slope_guess=nullptr) const;


        //Checks if the key is present without inspecting the value.
        bool MetadataKeyPresent(std::string key) const;

        //Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
        template <class U> std::experimental::optional<U> GetMetadataValueAs(std::string key) const;


        //Various IO routines.
        bool Write_To_File(const std::string &filename) const; //Writes data to file as 4 columns. Use it to plot/fit.
        std::string Write_To_String(void) const; //Writes data to file as 4 columns. Use it to plot/fit.

        bool Read_From_File(const std::string &filename); //Reads data from a file as 4 columns. True iff all went OK.

        void Plot(const std::string &Title = "") const; //Spits out a default plot of the data. 
        void Plot_as_PDF(const std::string &Title = "",const std::string &Filename = "/tmp/plot.pdf") const; //May overwrite existing files!

        //----------------------------------------------------- Friends ----------------------------------------------------
        //Overloaded stream operators for reading, writing, and serializing the samples.
        template<class Y> friend std::ostream & operator << (std::ostream &, const samples_1D<Y> &);
        template<class Y> friend std::istream & operator >> (std::istream &, samples_1D<Y> &);
};

//Standalone function for viewing the distribution of a list of scalar numbers. Great for performing a statistical bootstrap
// or inspecting normality of a measured quantity.
template <class C>
samples_1D<typename C::value_type>
Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const C &nums, long int N, bool explicitbins = false);



#endif

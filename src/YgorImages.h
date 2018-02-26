//YgorImages.h
#pragma once
#ifndef YGOR_IMAGES_HDR_GRD_H
#define YGOR_IMAGES_HDR_GRD_H

#include <experimental/any>
#include <experimental/optional>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorMath.h"


//----------------------------------------------------------------------------------------------------
//---------------------------------------- planar_image ----------------------------------------------
//----------------------------------------------------------------------------------------------------
//This is a class used to handle flat images (ie. 2D buffers) which may be floating in 3D space. This 
// class is useful because it encapsulates external indexing and querying. It can simply dole out 
// image values, or it can precisely tell you the 3D center of a given pixel (or voxel).
//
template <class T, class R> class planar_image {
    //NOTE: template type T --- image data type (probably unsigned int).
    //NOTE: template type R --- real-world coordinates (probably double or float).

    public: //private:
        //This is the actual, raw, 2D data.
//        std::unique_ptr<T []> data;
        std::vector<T> data;

        long int rows;      //This is the number of rows in the 2D image.
        long int columns; 

        long int channels;  //The number of colour channels.

        R pxl_dx;//(row)    //This is the R^3 spacing of nearest-neighbour pixels, which is assumed to 
        R pxl_dy;//(col)    // also be the in-plane dimensions of each pixel (i.e., image pixels are 
                            // watertight).
                            //
                            // Specifically, pxl_dx is the in-plane spacing between adjacent columns,
                            // and pxl_dy is the in-plane spacing between adjacent rows.
                            // in the image plane (where |x| points along the row unit vector - see below).
                            //
                            // pxl_dx could also be called pxl_drow or drow, because you need to translate
                            // by 1.0*pxl_dx along row_unit to get to the adjacent pixel in the same row.
                            //
                            // pxl_dy could also be called pxl_dcol or dcol, because you need to translate
                            // by 1.0*pxl_dy along col_unit to get to the adjacent pixel in the same col.

        R pxl_dz;           //This is the R^3 'thickness' of the image. A thickness of zero is perfectly
                            // fine. (The meaning of the thickness of a plane is mostly up to the user.)
                            // It is treated as the thickness along the normal to row and col unit vectors.
                            // It is considered to be half above and half below plane of image when
                            // internal calculations are needed.
                            //
                            // The volume of a single voxel should be equal to pxl_dx*pxl_dy*pxl_dz.

        vec3<R> anchor;     //An (arbitrary) point in R^3 in which the image is considered to be anchored to.
                            // The edges, corners, and center of the image are given relative to this.
                            // The point is virtual, and should be thought of as the origin of a frame of 
                            // reference (which may not be at the numbers (0,0,0)).
                            //NOTE: This vector is most likely to be (0,0,0) for typical usage.

        vec3<R> offset;     //A vector from the anchor's terminus to the *center* of the pixel with pixel
                            // space coordinates (0,0). It is how we specify the position of the 2D data
                            // in R^3. Do not confuse this with the vector from the anchor's terminus to 
                            // the *corner* of the pixel with pixel space coordinates (0,0).

        vec3<R> row_unit;   //An orientation unit vector denoting the (positive) direction of row indices.
                            // For example, if R denotes the location of pixel(row,col), then
                            // [R' = R + row_unit*pxl_dx] is the location of pixel(row+1,col) (if it exists).

        vec3<R> col_unit;   //An orientation unit vector denoting the (positive) direction of col indices.
                            // For example, if R denotes the location of pixel(row,col), then
                            // [R' = R + col_unit*pxl_dy] is the location of pixel(row,col+1) (if it exists).

        std::map<std::string,std::string> metadata; //User-defined metadata.

        //  ****** If you add data members to this class, be certain that you modify all  *******
        //  ****** routines to accommodata. This includes all serialization and IO        *******
        //  ****** routines as well!                                                      *******

        //------------------------------------ Member functions --------------------------------------------
        //Zero-based indexing (the default, and used internally). These routines return -1 if out-of-bounds.
        long int index(long int r, long int c) const;
        long int index(long int row, long int col, long int chnl) const;
        long int index(const vec3<R> &point, long int chnl) const;

        std::tuple<long int,long int,long int> row_column_channel_from_index(long int index) const;
        std::pair<R, R> fractional_row_column(const vec3<R> &point) const; //throws if out-of-bounds.

    //public:

        //------------------------------------ Member functions --------------------------------------------
        //Constructor/Destructors.
        planar_image();
        planar_image(const planar_image<T,R> &in); //Performs a deep copy (unless copying self).
        ~planar_image();

        //Allocating space and initializing the purely-2D-image members.
        void init_buffer(long int rows, long int columns, long int channels);

        //Initializing the R^3 members. These are less important because they won't cause a segfault.
        // If one wants to simply deal with R^2 image data, they could probably skip these.
        void init_spatial(R pxldx, R pxldy, R pxldz, const vec3<R> &anchr, const vec3<R> &offst);
        void init_orientation(const vec3<R> &rowunit, const vec3<R> &colunit);

        planar_image & operator= (const planar_image &); //Performs a deep copy (unless copying self).
        template <class U> planar_image & cast_from (const planar_image<U,R> &); //Copies other image types using static_cast<T>(v).
        bool operator==(const planar_image &) const; //Deep comparison: image position, dimensions, orientation, metadata, pixel values.
        bool operator!=(const planar_image &) const;
        bool operator< (const planar_image &) const; //This is mostly for sorting / placing into a std::map. There is no great 
                                                     // way to define '<' for images, so be careful with it. This is designed to be fast, not reliable!

        //Spatial comparison operators. Recovers natural ordering when possible. Handles different anchors.
        bool Spatially_eq(const planar_image &) const;
        bool Spatially_lt(const planar_image &) const;
        bool Spatially_lte(const planar_image &) const;

        //Get the value of a channel. These throw if out-of-bounds.
        T value(long int row, long int col, long int chnl) const;
        T value(const vec3<R> &point, long int chnl) const;
        T value(long int index) const;

        //Get a reference to the value of a channel. These throw if out-of-bounds.
        T& reference(long int row, long int col, long int chnl);
        T& reference(const vec3<R> &point, long int chnl);
        T& reference(long int index);

        //Interpolate within the plane of the image, in pixel number coordinates (e.g, permitting fractional pixel row and numbers).
        // Only nearest neighbour pixels are used, and mirror boundary conditions are assumed. Pixel shape is ignored.
        T bilinearly_interpolate_in_pixel_number_space(R row, R col, long int chnl) const; //Fails on out-of-bounds input.

        //Compute centered finite-difference approximations of derivatives (in pixel coordinate space) along the row and column axes.
        // First derivatives. Only nearest neighbour pixels are used, and mirror boundary conditions are assumed. Pixel shape is ignored.
        R row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        //Compute centered finite-difference approximations of derivatives (in pixel coordinate space) along the row and column axes.
        // Second derivatives. Only nearest neighbour pixels are used, and mirror boundary conditions are assumed. Pixel shape is ignored.
        R row_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R cross_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        //Interpolate within the plane of the image, in pixel number coordinates (e.g, permitting fractional pixel row and numbers).
        // Only nearest neighbour pixels are used, but derivatives use NN-NN pixels. Mirror boundary conditions are assumed. Pixel shape is ignored.
        T bicubically_interpolate_in_pixel_number_space(R row, R col, long int chnl) const; //Fails on out-of-bounds input.

        //Average a block of pixels. Boundaries are inclusive. Out-of-bounds parts are ignored. Negatives OK (they are just ignored).
        T block_average(long int row_min, long int row_max, long int col_min, long int col_max, long int chnl) const; 
        T block_median(long int row_min, long int row_max, long int col_min, long int col_max, long int chnl) const;

        //Minimum and maximum pixel values.
        std::pair<T,T> minmax(void) const; //The min/maximum pixel values of all channels.

        //Set all pixel data to the given value.
        void fill_pixels(long int chnl, T val);
        void fill_pixels(T val); //All channels.

        //Fill pixels above a given plane. Returns the number of affected pixels. Provide empty set for all channels.
        long int set_voxels_above_plane(const plane<R> &, T val, std::set<long int> chnls);

        //Apply a functor to individual pixels.
        void apply_to_pixels(std::function<void(long int row, long int col, long int chnl, T &val)> func);

        //Replace non-finite numbers.
        void replace_nonfinite_pixels_with(long int chnl, T val);
        void replace_nonfinite_pixels_with(T val); //All channels.

        //Get an R^3 position of the *center* of the pixel/voxel.
        vec3<R> position(long int row, long int col) const;

        //Determine if a given point in R^3 is encompassed by the 3D volume of the image (using 'thickness' pxl_dz).
        bool encompasses_point(const vec3<R> &in) const; //Note: does not consider points on the outermost surface encompassed.

        //Determine if a given point is in the infinite, thick plane defined by the 'top' and 'bottom' of image.
        bool sandwiches_point_within_top_bottom_planes(const vec3<R> &in) const;

        //Determine if a contour is contained within the 3D volume of the image.
        bool encompasses_contour_of_points(const contour_of_points<R> &in) const;

        //Determine if at least one contour is fully contained within the 3D volume of the image.
        bool encompasses_any_contour_in_collection(const contour_collection<R> &in) const;

        //Determine if a contour is partially contained within the 3D volume of the image.
        bool encompasses_any_of_contour_of_points(const contour_of_points<R> &in) const;

        //Determine if at least one contour is partially contained within the 3D volume of the image.
        bool encompasses_any_part_of_contour_in_collection(const contour_collection<R> &in) const;

        //Computes the R^3 center of the image. Nothing fancy.
        vec3<R> center(void) const;

        //Returns the volume occupied by the image.
        R volume(void) const;

        //Returns an ordered list of the corners of the 2D image. Does NOT use thickness!
        std::list<vec3<R>> corners2D(void) const; 

        //Returns the plane that the image resides in. Useful for is_point_in_poly routines.
        plane<R> image_plane(void) const;

        //Returns true if the 3D volume of this image encompasses the 2D image of the given planar image.
        bool encloses_2D_planar_image(const planar_image<T,R> &in) const; //Note: considers images on the boundary enclosed.

        //Returns (R)(1.0) for perfect spatial overlap, (R)(0.0) for no spatial overlap.
        R Spatial_Overlap_Dice_Sorensen_Coefficient(const planar_image<T,R> &in) const;

        //Blur pixels isotropically, completely ignoring pixel shape and real-space coordinates. Leave chnls empty for all channels.
        bool Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);

        //Checks if the key is present without inspecting the value.
        bool MetadataKeyPresent(std::string key) const;

        //Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
        template <class U> std::experimental::optional<U> GetMetadataValueAs(std::string key) const;

};





//---------------------------------------------------------------------------------------------------------------------------
//-------------------------- image_collection: a collection of logically-related planar_images  -----------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is a sort of utility class for holding logically-grouped images. For instance, a 3D array may be represented
// as a stack of planar_images. Note, however, that the onus is on the user to ensure the collection is sorted/handled
// in a consistent way.
//
//Essentially, this class is a thin wrapper around a std::list<planar_image> with some extra things like 3D plotting,
// some multi-image operations (like channel-volume groupings, etc.), and logical ordering tools.
//
//NOTE: This class assumes that all contained images are part of a logically cohesive unit. It is the user's responsibility
// to correctly interpret/handle their particular meaning of the words 'logically related'. Some operations may violate
// this notion in some scenarios. It is important not to enforce or require any particular arrangement so as to remain 
// as general as possible, whereever possible.
template <class T,class R>   class planar_image_collection {
    public:
        std::list<planar_image<T,R>> images;   //This is not a list of pointers because the planar_images contain a 
                                               // pointer to the data. It is cheap to move around planar_images!
                                               // Copying is another story. We usually want a local copy of image
                                               // data, so deep copying will be performed.

        typedef typename std::list<planar_image<T,R>>::iterator images_list_it_t;


        //  ****** Do not add data members to this class unless it is absolutely required *******
        //  ****** It will greatly complicate image collation, the operators, and the     *******
        //  ****** logical purpose for this routine. If you think you need to add data    *******
        //  ****** members, consider making a wrapping class or inheriting.               *******




        //Constructors.
        planar_image_collection();
        planar_image_collection(const planar_image_collection<T,R> &);
        planar_image_collection(std::list<planar_image<T,R>> ); 

        //Member functions.
        planar_image_collection & operator=(const planar_image_collection &); //Performs a deep copy (unless copying self).
        bool operator==(const planar_image_collection &) const; //Relies on the planar_image operator==. See caveats there.
        bool operator!=(const planar_image_collection &) const;
        bool operator< (const planar_image_collection &) const; //Relies on the planar_image operator< . See caveats there. Only use for sorting.

        void Swap(planar_image_collection &); //Swaps images.

        //Stable ordering operations. Useful for sorting on several keys (one at a time -- stable!).
        void Stable_Sort(std::function<bool(const planar_image<T,R> &lhs, const planar_image<T,R> &rhs)> lt_func);

        template <class P> void Stable_Sort_on_Metadata_Keys_Value_Numeric(const std::string &key); //Specialized for long int and double.
        void Stable_Sort_on_Metadata_Keys_Value_Lexicographic(const std::string &key);

        //Generate a stable-ordered list of iterators to images. Be careful not to invalidate the data after calling these!
        //std::list<typename std::list<planar_image<T,R>>::iterator> get_all_images(void);
        //std::list<typename std::list<planar_image<T,R>>::iterator> get_images_satisfying(std::function<bool(const planar_image<T,R> &animg)> select_pred);
        //std::list<typename std::list<planar_image<T,R>>::iterator> get_images_which_encompass_point(const vec3<R> &in);
        //std::list<typename std::list<planar_image<T,R>>::iterator> get_images_which_encompass_all_points(const std::list<vec3<R>> &in);
        //std::list<typename std::list<planar_image<T,R>>::iterator> get_images_which_sandwich_point_within_top_bottom_planes(const vec3<R> &in);

        //Various selectors and other non-member getters.
        std::list<images_list_it_t> get_all_images(void);
        std::list<images_list_it_t> get_images_satisfying(std::function<bool(const planar_image<T,R> &animg)> select_pred);
        std::list<images_list_it_t> get_images_which_encompass_point(const vec3<R> &in);
        std::list<images_list_it_t> get_images_which_encompass_all_points(const std::list<vec3<R>> &in);
        std::list<images_list_it_t> get_images_which_sandwich_point_within_top_bottom_planes(const vec3<R> &in);
        std::pair< std::list<images_list_it_t>, 
                   std::list<images_list_it_t> > get_nearest_images_above_below_not_encompassing_image(const planar_image<T,R> &animg);

        std::map<std::string,std::string> get_common_metadata(const std::list<images_list_it_t> &in) const; //Uses both *this and input.
        std::list<std::string> get_all_values_for_key(const std::string &akey) const;
        std::list<std::string> get_unique_values_for_key(const std::string &akey) const;


        //Image pruning/partitioning routine. Returns 'pruned' images; retains the rest in *this. If pruning predicate is true, 
        // image is pruned. The output from these functions can be ignored if you don't care about it!
        planar_image_collection<T,R> Prune_Images_Satisfying(std::function<bool(const planar_image<T,R> &animg)> prune_pred);
        planar_image_collection<T,R> Retain_Images_Satisfying(std::function<bool(const planar_image<T,R> &animg)> retain_pred);

        //Generic routine for processing/combining groups of images into single images with a user-defined operation. This is useful 
        // for spatial averaging, blurring, etc.. Note that this routine modifies *this, so deep-copy beforehand if needed. See 
        // in-source default functors for descriptions/examples. The user has full read-write access to the external_imgs.
        //
        // Note that this routine converts groups of images into single images and then *erases* all but one of the old images.
        // This is a feature; many algorithms condense groups of images into a single image.
        bool Process_Images( std::function<typename std::list<images_list_it_t> (images_list_it_t,
                                      std::reference_wrapper<planar_image_collection<T,R>> )>         image_grouper,
                             std::function<bool (images_list_it_t, 
                                      std::list<images_list_it_t>,
                                      std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::experimental::any )>                                       operation_functor, 
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>          external_imgs,
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::experimental::any                                                   user_data = std::experimental::any() );

        bool Process_Images_Parallel( std::function<typename std::list<images_list_it_t> (images_list_it_t,
                                      std::reference_wrapper<planar_image_collection<T,R>> )>         image_grouper,
                             std::function<bool (images_list_it_t, 
                                      std::list<images_list_it_t>,
                                      std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::experimental::any )>                                       operation_functor, 
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>          external_imgs,
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::experimental::any                                                   user_data = std::experimental::any() );


        //Generic routine for performing an operation on images which may depend on external images (such as pixel maps).
        // No images are deleted, though the user has full read-write access to the image data of all images. This routine is complementary
        // to the Process_Images() routine, and has different use-cases and approaches.
        bool Transform_Images( std::function<bool (images_list_it_t, 
                                        std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                        std::list<std::reference_wrapper<contour_collection<R>>>,
                                        std::experimental::any )>                                          op_func,
                               std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
                               std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
                               std::experimental::any                                                      user_data = std::experimental::any() );

        bool Transform_Images_Parallel( std::function<bool (images_list_it_t, 
                                        std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                        std::list<std::reference_wrapper<contour_collection<R>>>,
                                        std::experimental::any )>                                          op_func,
                               std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
                               std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
                               std::experimental::any                                                      user_data = std::experimental::any() );


        //Generic routine for altering images as a whole, or computing histograms, time courses, or any sort of distribution using 
        // the entire set of images. No images are deleted, though the user has full read-write access to the image data of all images.
        // This routine is very simple, and merely provides a consistent interface for doing operations on the image collection.
        // (Note that the functor gets called only once by this routine.)
        bool Compute_Images( std::function<bool (planar_image_collection<T,R> &,    //<-- gets populated with *this.
                                      std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::experimental::any )>                                          op_func,
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
                             std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
                             std::experimental::any                                                      user_data = std::experimental::any() );
        

        //Condense groups of images to a single image by averaging at the pixel level, channel by channel. Only the final,
        // averaged image remains.
        bool Condense_Average_Images( std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                                               std::reference_wrapper<planar_image_collection<T,R>>)>    image_grouper );

        //Blur pixels isotropically, independently in their image plane, completely ignoring pixel shape and real-space coordinates.
        // Leave chnls empty for all channels. For more selectivity, run on each image separately.
        bool Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);

        //Fill pixels above a given plane. Returns the number of affected pixels. Provide empty set for all channels.
        long int set_voxels_above_plane(const plane<R> &, T val, std::set<long int> chnls);

        //Apply a functor to individual pixels.
        void apply_to_pixels(std::function<void(long int row, long int col, long int chnl, T &val)> func);

        //Computes the R^3 center of the images.
        vec3<R> center(void) const;

        //Computes the volume occupied by the images.
        // Note: may or may not account for spatially overlapping images.
        R volume(void) const;

        //Compare the geometrical (non-pixel/voxel) aspects of the images to one another.
        bool Spatially_eq(const planar_image_collection<T,R> &in) const;

        //Collates images together, taking ownership of input image data on success. Behaviour can be specified.
        // Note this routine does not *combine* image data -- just collections of images.
        // Should always work if either image collection is empty.
        bool Collate_Images(planar_image_collection<T,R> &in, bool GeometricalOverlapOK = true);

        //Interpolate in R^3.
        T trilinearly_interpolate(const vec3<R> &position, long int chnl, R out_of_bounds = std::numeric_limits<T>::quiet_NaN());

};


//Produce an image by cutting through the image collection and copying intersecting pixels.
template <class T,class R>
long int Intersection_Copy(planar_image<T,R> &in, 
                           const std::list<typename planar_image_collection<T,R>::images_list_it_t> &imgs);


//Produce an image collection that contiguously covers the volume containing the provided contours.
template <class T,class R>
planar_image_collection<T,R> 
Contiguously_Grid_Volume(const std::list<std::reference_wrapper<contour_collection<R>>> &ccs,
                         const R x_margin = static_cast<R>(1.0),
                         const R y_margin = static_cast<R>(1.0),
                         const R z_margin = static_cast<R>(1.0),
                         const long int number_of_rows = 256,
                         const long int number_of_columns = 256,
                         const long int number_of_channels = 1,
                         const long int number_of_images = 25,
                         const vec3<R> &x_orientation = vec3<R>(static_cast<R>(1), static_cast<R>(0), static_cast<R>(0)),
                         const vec3<R> &y_orientation = vec3<R>(static_cast<R>(0), static_cast<R>(1), static_cast<R>(0)),
                         const vec3<R> &z_orientation = vec3<R>(static_cast<R>(0), static_cast<R>(0), static_cast<R>(1)),
                         const R pixel_fill = std::numeric_limits<R>::quiet_NaN(),
                         bool only_top_and_bottom = false); //Only create top and bottom (i.e., extremal) images.


//Produce an image collection that contiguously covers the volume containing the provided contours, but produces images
//  that are centred along a given line (symmetric in the use of margins).
template <class T,class R>
planar_image_collection<T,R> 
Symmetrically_Contiguously_Grid_Volume(const std::list<std::reference_wrapper<contour_collection<R>>> &ccs,
                                       const R x_margin = static_cast<R>(1.0),
                                       const R y_margin = static_cast<R>(1.0),
                                       const R z_margin = static_cast<R>(1.0),
                                       const long int number_of_rows = 256,
                                       const long int number_of_columns = 256,
                                       const long int number_of_channels = 1,
                                       const long int number_of_images = 25,
                                       const line<R> &symm_line = line<double>( vec3<R>(static_cast<R>(0), static_cast<R>(0), static_cast<R>(0)),
                                                                                vec3<R>(static_cast<R>(0), static_cast<R>(0), static_cast<R>(1)) ),
                                       const vec3<R> &x_orientation = vec3<R>(static_cast<R>(1), static_cast<R>(0), static_cast<R>(0)),
                                       const vec3<R> &y_orientation = vec3<R>(static_cast<R>(0), static_cast<R>(1), static_cast<R>(0)),
                                       const R pixel_fill = std::numeric_limits<R>::quiet_NaN(),
                                       bool only_top_and_bottom = false); //Only create top and bottom (i.e., extremal) images.

//A "parameter object" for the Encircle_Images_with_Contours() function.
struct Encircle_Images_with_Contours_Opts {
    enum class
    Inclusivity {  // Controls how the contour vertices lie in relation to the image corner voxels.
                   // (These options reflect how voxels will be bounded by the contours.)
        Centre,    // Corner voxel centres will be bounded by the contours.
        Inclusive, // Corner voxel innermost corners will be bounded by the contours.
        Exclusive, // Corner voxels will be fully bounded by the contours.
    } inclusivity;

    //       If the following depicts             The 'Inclusive' option
    //         a 3x3 image's voxels:                gives contours like:
    //
    //       o.......o.......o.......o           o.......o.......o.......o
    //       :       :       :       :           :       :       :       :
    //       :   o   :   o   :   o   :           :   o   :   o   :   o   :
    //       :       :       :       :           :       _________       :
    //       o.......o.......o.......o           o......|o.......o|......o
    //       :       :       :       :           :      |:       :|      :
    //       :   o   :   o   :   o   :           :   o  |:   o   :|  o   :
    //       :       :       :       :           :      |:       :|      :
    //       o.......o.......o.......:           o......|o.......o|......:
    //       :       :       :       :           :      `---------~      :
    //       :   o   :   o   :   o   :           :   o   :   o   :   o   :
    //       :       :       :       :           :       :       :       :
    //       o.......o.......o.......o           o.......o.......o.......o
    //
    //
    //          The 'centre' option                 And the 'Exclusive' 
    //         gives contours like:             option gives contours like:
    //                                           _________________________
    //       o.......o.......o.......o          |o.......o.......o.......o|
    //       :   _________________   :          |:       :       :       :|
    //       :  |o   :   o   :   o|  :          |:   o   :   o   :   o   :|
    //       :  |    :       :    |  :          |:       :       :       :|
    //       o..|....o.......o....|..o          |o.......o.......o.......o|
    //       :  |    :       :    |  :          |:       :       :       :|
    //       :  |o   :   o   :   o|  :          |:   o   :   o   :   o   :|
    //       :  |    :       :    |  :          |:       :       :       :|
    //       o..|....o.......o....|..:          |o.......o.......o.......:|
    //       :  |    :       :    |  :          |:       :       :       :|
    //       :  |o   :   o   :   o|  :          |:   o   :   o   :   o   :|
    //       :  `----------------~   :          |:       :       :       :|
    //       o.......o.......o.......o          |o.......o.......o.......o|
    //                                          `------------------------~

    enum class
    ContourOverlap {  // Controls whether ROIs can overlap (e.g., due to duplicate images).
        Allow,        // Allow all overlap, even duplicates.
        Disallow,     // Do not allow any overlap, even partial overlap.
    } contouroverlap;
};

//Generate contours that fully encircle/encapsulate the provided images.
template <class T,class R>
contour_collection<R>
Encircle_Images_with_Contours(const std::list<std::reference_wrapper<planar_image<T,R>>> &imgs,
                              Encircle_Images_with_Contours_Opts options,
                              const std::map<std::string,std::string> &metadata);


//A "parameter object" for the Mutate_Voxels() function.
struct Mutate_Voxels_Opts {
    enum class 
    EditStyle {    // Controls how the new value is inserted into the outgoing/edited image.
        InPlace,   // Edits the image in-place. Requires the Adjacency::SingleVoxel option.
        Surrogate, // Provides a separate working image for edits and updates the original afterward.
    } editstyle;

    enum class
    Inclusivity {  // Controls how the voxel (which has a non-zero volume) is tested to be 'within' contours.
        Centre,    // Only check that the centre of the voxel is 'within' contours.
        Inclusive, // In addition to the centre, also use the corners. Any bounded corner implies the voxel is bounded.
        Exclusive, // In addition to the centre, also use the corners. All corners are required to be bounded.
    } inclusivity;

    enum class
    ContourOverlap { // Controls how overlapping contours are handled.
        Ignore,      // Treat overlapping contours as a single contour, regardless of contour orientation.
        HonourOppositeOrientations, // Overlapping contours with opposite orientation cancel. Otherwise, orientation is ignored.
        ImplicitOrientations, // Assume all overlapping contours have opposite orientations and cancel.
                              // In other words, this is a 'toggle' or 'XOR' option.
    } contouroverlap;

    enum class
    Aggregate {    // Controls how the existing voxel values (i.e., from selected_img_its) are aggregated into a scalar.
        Mean,      // Take the mean of all coincident voxels in the selected images.
        Median,    // Take the median of all coincident voxels in the selected images.
        Sum,       // Take the sum of all coincident voxels in the selected images.
        First,     // Take only the voxel value from the first selected image.
    } aggregate;

    enum class
    Adjacency {      // Controls how nearby voxel values are used when computing existing voxel values.
        SingleVoxel, // Only consider the individual bounded voxel, ignoring neighbours.
        NearestNeighbours, // Also use the four nearest neighbours (in the image plane).                       ...Boundary voxels? TODO
    } adjacency;

    enum class
    MaskMod {      // Controls how the masks denoting whether voxels are bounded are modified (post-processed).
        Noop,      // Perform no post-processing on the mask.
      //Grow,      // Grow the mask to include all voxels that are nearest neighbours to the bounded voxels.
      //RemoveIsolated, // Remove voxels that are isolated from other bounded voxels (ala Game of Life).
    } maskmod;

// - RunningMinMax::AllVoxels    // Include all voxels in the edited/output image.
// - RunningMinMax::OnlyBounded  // Only the voxels whose value has been changed.  (Useful?)

};


// This routine applies a user-provided function on voxels that are bounded within one or more contours. The
// user-provided function only gets called to update voxels that are bounded by one or more contours (depending on the
// specific options selected). The internal behaviour is parameterized. Several input images can be handled: the voxel
// values are aggregated.
//
// Note: the list 'selected_img_its' contains the images from which voxel values will be aggregated. The list may
//       itself contain the 'img_to_edit' which will be overwritten. If neighbouring voxels are to be taken into
//       account, you should NOT use in-place editing.
//
// Note: the provided functors are called once per voxel, no matter how many contours encompass them. Only one of the
//       bounded/unbounded functors is called for each voxel. The observer functor is called for every voxel.
//
// Note: the observer functor is always provided post-modification voxel values (if they are modified).
//
template <class T,class R>
void Mutate_Voxels(
    std::reference_wrapper<planar_image<T,R>> img_to_edit,
    std::list<std::reference_wrapper<planar_image<T,R>>> selected_imgs,
    std::list<std::reference_wrapper<contour_collection<R>>> ccsl,
    Mutate_Voxels_Opts options,
    std::function<void(long int row, long int col, long int channel, T &voxel_val)> f_bounded
        = std::function<void(long int row, long int col, long int channel, T &voxel_val)>(),
    std::function<void(long int row, long int col, long int channel, T &voxel_val)> f_unbounded 
        = std::function<void(long int row, long int col, long int channel, T &voxel_val)>(),
    std::function<void(long int row, long int col, long int channel, T &voxel_val)> f_observer
        = std::function<void(long int row, long int col, long int channel, T &voxel_val)>() );

#endif

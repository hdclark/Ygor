//YgorImages.h
#pragma once
#ifndef YGOR_IMAGES_HDR_GRD_H
#define YGOR_IMAGES_HDR_GRD_H

#include <any>
#include <optional>
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

#include "YgorDefinitions.h"
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

        //Add and remove channels.
        void add_channel(T channel_value);
        void remove_channel(long int channel_number); // Zero-based.
        void remove_all_channels_except(long int channel_number); // Zero-based.

        //Interpolate within the plane of the image, in pixel number coordinates (e.g, permitting fractional pixel row and numbers).
        // Only nearest neighbour pixels are used, and mirror boundary conditions are assumed. Pixel shape is ignored.
        T bilinearly_interpolate_in_pixel_number_space(R row, R col, long int chnl) const; //Fails on out-of-bounds input.

        //Compute centered finite-difference approximations of derivatives (in pixel coordinate space) along the row and column axes.
        // First derivatives. Only nearest neighbour pixels are used, and mirror boundary conditions are assumed. Pixel shape is ignored.
        R row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        R prow_pcol_aligned_Roberts_cross_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R nrow_pcol_aligned_Roberts_cross_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        R row_aligned_Prewitt_derivative_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_Prewitt_derivative_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        R row_aligned_Sobel_derivative_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_Sobel_derivative_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        R row_aligned_Sobel_derivative_5x5(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_Sobel_derivative_5x5(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        R row_aligned_Scharr_derivative_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_Scharr_derivative_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        R row_aligned_Scharr_derivative_5x5(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        R column_aligned_Scharr_derivative_5x5(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

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

        //Approximate pixel-coordinate blurs using precomputed convolution kernel estimators.
        T fixed_gaussian_blur_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        T fixed_gaussian_blur_5x5(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        T fixed_box_blur_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        T fixed_box_blur_5x5(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        //Approximate pixel-coordinate sharpening using precomputed convolution kernel estimators.
        T fixed_sharpen_3x3(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        T fixed_unsharp_mask_5x5(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.

        //Minimum and maximum pixel values.
        std::pair<T,T> minmax(void) const; //The min/maximum pixel values of all channels.

        //Set all pixel data to the given value.
        void fill_pixels(long int chnl, T val);
        void fill_pixels(T val); //All channels.

        //Fill pixels above a given plane. Returns the number of affected pixels. Provide empty set for all channels.
        long int set_voxels_above_plane(const plane<R> &, T val, std::set<long int> chnls);

        //Apply a functor to individual pixels.
        void apply_to_pixels(std::function<void(long int row, long int col, long int chnl, T &val)> func);
        void apply_to_pixels(std::function<void(long int row, long int col, long int chnl, T val)> func) const;

        //Replace non-finite numbers.
        void replace_nonfinite_pixels_with(long int chnl, T val);
        void replace_nonfinite_pixels_with(T val); //All channels.

        //Get an R^3 position of the *center* of the pixel/voxel.
        vec3<R> position(long int row, long int col) const;
        vec3<R> position(long int index) const;

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

        //Clip the provided contours to the (six) image boundaries.
        contour_collection<R> clip_to_volume(contour_collection<R> in) const;

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
        template <class U> std::optional<U> GetMetadataValueAs(std::string key) const;

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
        std::list<std::string> get_distinct_values_for_key(const std::string &akey) const;


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
                                      std::any )>                                                     operation_functor, 
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>          external_imgs,
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::any                                                                 user_data = std::any() );

        bool Process_Images_Parallel( std::function<typename std::list<images_list_it_t> (images_list_it_t,
                                      std::reference_wrapper<planar_image_collection<T,R>> )>         image_grouper,
                             std::function<bool (images_list_it_t, 
                                      std::list<images_list_it_t>,
                                      std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::any )>                                                     operation_functor, 
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>          external_imgs,
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::any                                                                 user_data = std::any() );


        //Generic routine for performing an operation on images which may depend on external images (such as pixel maps).
        // No images are deleted, though the user has full read-write access to the image data of all images. This routine is complementary
        // to the Process_Images() routine, and has different use-cases and approaches.
        bool Transform_Images( std::function<bool (images_list_it_t, 
                                        std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                        std::list<std::reference_wrapper<contour_collection<R>>>,
                                        std::any )>                                                        op_func,
                               std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
                               std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
                               std::any                                                                    user_data = std::any() );

        bool Transform_Images_Parallel( std::function<bool (images_list_it_t, 
                                        std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                        std::list<std::reference_wrapper<contour_collection<R>>>,
                                        std::any )>                                                        op_func,
                               std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
                               std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
                               std::any                                                                    user_data = std::any() );


        //Generic routine for altering images as a whole, or computing histograms, time courses, or any sort of distribution using 
        // the entire set of images. No images are deleted, though the user has full read-write access to the image data of all images.
        // This routine is very simple, and merely provides a consistent interface for doing operations on the image collection.
        // (Note that the functor gets called only once by this routine.)
        bool Compute_Images( std::function<bool (planar_image_collection<T,R> &,    //<-- gets populated with *this.
                                      std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::any )>                                                        op_func,
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
                             std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
                             std::any                                                                    user_data = std::any() );
        

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
        void apply_to_pixels(std::function<void(long int row, long int col, long int chnl, T val)> func) const;

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
    } inclusivity = Inclusivity::Centre;

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
    } contouroverlap = ContourOverlap::Allow;
};

//Generate contours that fully encircle/encapsulate the provided images.
template <class T,class R>
contour_collection<R>
Encircle_Images_with_Contours(const std::list<std::reference_wrapper<planar_image<T,R>>> &imgs,
                              Encircle_Images_with_Contours_Opts options,
                              const std::map<std::string,std::string> &metadata,
                              vec3<R> dRowLH = vec3<R>().zero(),   // Extra offset applied (by +ing) at row 0 vertices.
                              vec3<R> dRowHL = vec3<R>().zero(),   // Extra offset applied (by +ing) at max row vertices.
                              vec3<R> dColLH = vec3<R>().zero(),   // Extra offset applied (by +ing) at column 0 vertices.
                              vec3<R> dColHL = vec3<R>().zero() ); // Extra offset applied (by +ing) at max column vertices.


//A "parameter object" for the Mutate_Voxels() function.
struct Mutate_Voxels_Opts {
    enum class 
    EditStyle {    // Controls how the new value is inserted into the outgoing/edited image.
        InPlace,   // Edits the image in-place. Requires the Adjacency::SingleVoxel option.
        Surrogate, // Provides a separate working image for edits and updates the original afterward.
    } editstyle = EditStyle::Surrogate;

    enum class
    Inclusivity {  // Controls how the voxel (which has a non-zero volume) is tested to be 'within' contours.
        Centre,    // Only check that the centre of the voxel is 'within' contours.
        Inclusive, // In addition to the centre, also use the corners. Any bounded corner implies the voxel is bounded.
        Exclusive, // In addition to the centre, also use the corners. All corners are required to be bounded.
    } inclusivity = Inclusivity::Centre;

    enum class
    ContourOverlap { // Controls how overlapping contours are handled.
        Ignore,      // Treat overlapping contours as a single contour, regardless of contour orientation.
        HonourOppositeOrientations, // Overlapping contours with opposite orientation cancel. Otherwise, orientation is ignored.
        ImplicitOrientations, // Assume all overlapping contours have opposite orientations and cancel.
                              // In other words, this is a 'toggle' or 'XOR' option.
    } contouroverlap = ContourOverlap::Ignore;

    enum class
    Aggregate {    // Controls how the existing voxel values (i.e., from selected_img_its) are aggregated into a scalar.
        Mean,      // Take the mean of all coincident voxels in the selected images.
        Median,    // Take the median of all coincident voxels in the selected images.
        Sum,       // Take the sum of all coincident voxels in the selected images.
        First,     // Take only the voxel value from the first selected image (nb. from the voxel centre).
    } aggregate = Aggregate::Mean;

    enum class
    Adjacency {      // Controls how nearby voxel values are used when computing existing voxel values.
        SingleVoxel, // Only consider the individual bounded voxel (for every selected image), ignoring neighbours.
        NearestNeighbours, // Also use the four nearest neighbours (for every selected image; only in the image plane; iff the neighbour exists).
    } adjacency = Adjacency::SingleVoxel;

    enum class
    MaskStyle {     // Controls where the mask is created.
        AddChannel, // Add a channel to the image to use as an inclusivity mask. The mask is retained for post-processing.
                    // Note that the mask channel will not be visited by user functors within the Mutate_Voxels routine.
        Surrogate,  // Use a surrogate image mask which is discarded afterward.
    } maskstyle = MaskStyle::Surrogate;

    enum class
    MaskMod {      // Controls how the masks denoting whether voxels are bounded are modified (post-processed).
        Noop,      // Perform no post-processing on the mask.
      //Grow,      // Grow the mask to include all voxels that are nearest neighbours to the bounded voxels.
      //RemoveIsolated, // Remove voxels that are isolated from other bounded voxels (ala Game of Life).
    } maskmod = MaskMod::Noop;

// - RunningMinMax::AllVoxels    // Include all voxels in the edited/output image.
// - RunningMinMax::OnlyBounded  // Only the voxels whose value has been changed.  (Useful?)

};


// This routine applies a user-provided function on voxels that are bounded within one or more contours. The
// user-provided function only gets called to update voxels that are bounded by one or more contours (depending on the
// specific options selected). The internal behaviour is parameterized. Several input images can be handled: the voxel
// values are aggregated.
//
// Note: The list 'selected_img_its' contains the images from which voxel values will be aggregated. The list may
//       itself contain the 'img_to_edit' which will be overwritten. The voxels from 'img_to_edit' will NOT be sampled
//       if 'img_to_edit' does not appear in 'selected_img_its'. If ONLY voxels from 'img_to_edit' need to be sampled,
//       then 'selected_img_its' should contain ONLY 'img_to_edit'. If neighbouring voxels are to be taken into
//       account, you should NOT use in-place editing.
//
// Note: All selected_imgs are sampled from -- no adjacency tracking is currently performed. In other words, the
//       selected images are always projected onto the 'img_to_edit' (or vice-versa), discarding adjacency information
//       completely. So ensure only those images which should be sampled (e.g., the self image, nearest-neighbour
//       adjacent images, etc) are passed in.
//
// Note: The provided functors are called once per voxel, no matter how many contours encompass them. Only one of the
//       bounded/unbounded functors is called for each voxel. The observer functor is called for every voxel.
//
// Note: The observer functor is always provided post-modification voxel values (if they are modified).
//
template <class T,class R>
void Mutate_Voxels(
    std::reference_wrapper<planar_image<T,R>> img_to_edit,
    std::list<std::reference_wrapper<planar_image<T,R>>> selected_imgs,
    std::list<std::reference_wrapper<contour_collection<R>>> ccsl,
    Mutate_Voxels_Opts options,
    std::function<void(long int row, long int col, long int channel, std::reference_wrapper<planar_image<T,R>> img_refw, T &voxel_val)> f_bounded
        = std::function<void(long int row, long int col, long int channel, std::reference_wrapper<planar_image<T,R>> img_refw, T &voxel_val)>(),
    std::function<void(long int row, long int col, long int channel, std::reference_wrapper<planar_image<T,R>> img_refw, T &voxel_val)> f_unbounded 
        = std::function<void(long int row, long int col, long int channel, std::reference_wrapper<planar_image<T,R>> img_refw, T &voxel_val)>(),
    std::function<void(long int row, long int col, long int channel, std::reference_wrapper<planar_image<T,R>> img_refw, T &voxel_val)> f_observer
        = std::function<void(long int row, long int col, long int channel, std::reference_wrapper<planar_image<T,R>> img_Refw, T &voxel_val)>() );


// Test whether the images collectively form a regular grid.
//
// In a regular grid the voxels can be unambiguously addressed by three indices (e.g., i,j,k) and the position of voxel
// satisfies $p(i,j,k) = p(0,0,0) + i*(p(1,0,0)-p(0,0,0)) + j*(p(0,1,0)-p(0,0,0)) + k*(p(0,0,1)-p(0,0,0))$.
// Another way of describing the property of regularity is if the planes abutting voxel faces are either orthogonal or
// congruent and also evenly spaced in 3 distinct directions.
//
// Rectinear grids are like regular grids in that the planes abutting voxel faces are either orthogonal or congruent,
// but the planes need **not** be evenly spaced in 3 dimensions. For the case of planar images, in which in-image-plane
// regularity is enforced, images that form a rectilinear grid might be separated by a variable amount of distance.
// Rectilinear grids can still be indexed by three indices (e.g., i,j,k), and abutting voxels still will have a
// separation of exactly one coordinate number (e.g., i and i+1, or i and i-1, but never i and i+2)..
//
// Verifying either regularity or rectilinearity holds for an arbitrary collection of images often permit the use of
// techniques that afford a considerable reduction in computational complexity. Both regular and rectilinear grids can
// be traversed easily by enumerating (i,j,k) over a set of positive integers. Both can be important for interpolation.
//
// Note: Returns true IFF the images form a (non-empty) regular (or rectilinear) grid.
//
// Note: The parameter 'eps' controls the spatial precision required. 
//
// Note: The rectilinear variant currently does not verify that image pxl_dz is valid. (This consistency is often
//       irrelevant.)

template <class T,class R>
bool Images_Form_Regular_Grid(
    std::list<std::reference_wrapper<planar_image<T,R>>> img_refws,
    R eps = std::sqrt( static_cast<R>(10) * std::numeric_limits<R>::epsilon()) );

template <class T,class R>
bool Images_Form_Rectilinear_Grid(
    std::list<std::reference_wrapper<planar_image<T,R>>> img_refws,
    R eps = std::sqrt( static_cast<R>(10) * std::numeric_limits<R>::epsilon()) );


//---------------------------------------------------------------------------------------------------------------------------
//------------------------------- planar_image_adjacency: a class for 3D adjacency indexing ---------------------------------
//---------------------------------------------------------------------------------------------------------------------------
// Class for determining and querying adjacency information for a collection of images.
// This class is designed to make working with 3D grids composed of 2D planar slices more easy.
//
// This routine is most useful after ascertaining that the images form a regular or rectilinear grid.
// It allows to query for the nearest image, determine which images are adjacent (above and below the image plane), and
// compute a given image's integer index in the grid.
//
// Note: Pointers to planar_images were used in a predecessor routine since operator< for reference wrappers degrades to
//       the underlying object (planar_image in this case), which is costly compared to operator< for a pointer.
//       The external interface has been made to use reference_wrappers, but images are still indexed internally by
//       pointers address to the underlying images. BEWARE: MOVING IMAGES OR REALLOCATION BY CONTAINERS STORING THEM
//       COULD LEAD TO MEMORY ERRORS!
//
// Note: This routine is complementary to the planar_image_collection class. Not all logically-related planar_images
//       need to be indexed, nor are they necessarily rectilinear. Additionally, adjacency may be needed between a sub-
//       or superset of planar_images, rather than for all images within a planar_image_collection.
//

template <class T,class R>   class planar_image_adjacency {
    public:

        using img_refw_t      = std::reference_wrapper< planar_image<T,R> >;
        using img_coll_refw_t = std::reference_wrapper< planar_image_collection<T,R> >;
        using img_ptr_t       = planar_image<T,R>*;
        using img_planes_t    = std::pair< plane<R>, img_ptr_t >;

        // Normal used to sort images for adjacency determination.
        vec3<R> orientation_normal;

        // Store image planes to determine which image is nearest to a user-provided point.
        std::vector<img_planes_t> img_plane_to_img;

        // Index-to-image and vice versa.
        std::vector<img_ptr_t> int_to_img;
        std::map<img_ptr_t, long int> img_to_int;

        // Planes that define a bounding volume for faster interpolation outside the useful volume.
        //
        // Note: These are only used during interpolation.
        //
        // Note: This bounding volume is a hyperrectangular volume. If images are not rectilinear or regular, then the
        //       bounding volume will be larger than the volume occupied by the images. Interpolation in these regions
        //       will require a more costly inclusivity check.
        std::vector<plane<R>> bounding_volume_planes;

    //------------------------------------------------------------------------

        planar_image_adjacency( 
            const std::list< img_refw_t > &imgs,
            const std::list< img_coll_refw_t > &img_colls, 
            const vec3<R> &normal );

        // Find the nearest image plane; not necessarily overlapping!
        img_refw_t
        position_to_image(const vec3<R> &p) const;

        // Query for whole images that overlap.
        // This is useful when working with two overlapping rectilinear grids.
        // 
        // Note: If the result is empty, images may still partially overlap.
        //       Use the complementary position_to_image() for individual voxels.
        std::list< img_refw_t >
        get_wholly_overlapping_images(const img_refw_t &) const;
            
        // Query whether the index or image are known by this class.
        bool index_present(long int) const;
        bool image_present(const img_refw_t &) const;

        // Convert an index to an image reference, and vice versa.
        img_refw_t index_to_image(long int) const;
        long int image_to_index(const img_refw_t &) const;
        
        // Interpolate the image at the given point, if it is possible.
        T trilinearly_interpolate(const vec3<R> &pos,
                                  long int chnl, 
                                  R out_of_bounds = std::numeric_limits<T>::quiet_NaN()) const;
};


#endif

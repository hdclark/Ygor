//YgorImages.h
#pragma once
#ifndef YGOR_IMAGES_HDR_GRD_H
#define YGOR_IMAGES_HDR_GRD_H

#include <memory>
#include <string>
#include <list>
#include <set>
#include <map>
#include <utility>
#include <functional>
#include <experimental/optional>
#include <experimental/any>

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
        std::unique_ptr<T []> data;

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

        //------------------------------------ Member functions --------------------------------------------
        //Zero-based indexing (the default, and used internally).
        long int index(long int r, long int c) const;    //Note: For external access, prefer this->value().
        long int index(long int row, long int col, long int chnl) const;

        long int index(const vec3<R> &point, long int chnl) const; //Returns -1 or throws on failure/out of bounds. Verify.

        std::tuple<long int,long int,long int> row_column_channel_from_index(long int index) const;

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
        bool operator==(const planar_image &) const; //Deep comparison: image position, dimensions, orientation, metadata, pixel values.
        bool operator!=(const planar_image &) const;
        bool operator< (const planar_image &) const; //This is mostly for sorting / placing into a std::map. There is no great 
                                                     // way to define '<' for images, so be careful with it. This is designed to be fast, not reliable!

        //Spatial comparison operators. Recovers natural ordering when possible. Handles different anchors.
        bool Spatially_eq(const planar_image &) const;
        bool Spatially_lt(const planar_image &) const;
        bool Spatially_lte(const planar_image &) const;

        //Get the value of a channel.
        T value(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        T value(const vec3<R> &point, long int chnl) const; //Returns the value of the voxel which contains the point, or zero.

        //Get a reference to the value of a channel. This can be used to set it.
        T& reference(long int row, long int col, long int chnl);

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


        //Minimum and maximum pixel values.
        std::pair<T,T> minmax(void) const; //The min/maximum pixel values of all channels.


        //Set all pixel data to the given value.
        void fill_pixels(long int chnl, T val);
        void fill_pixels(T val); //All channels.

        //Get an R^3 position of the *center* of the pixel/voxel.
        vec3<R> position(long int row, long int col) const;

        //Determine if a given point in R^3 is encompassed by the 3D volume of the image (using 'thickness' pxl_dz).
        bool encompasses_point(const vec3<R> &in) const; //Note: does not consider points on the boundary encompassed.

        //Determine if a given point is in the infinite, thick plane defined by the 'top' and 'bottom' of image.
        bool sandwiches_point_within_top_bottom_planes(const vec3<R> &in) const;

        //Determine if a contour is contained within the 3D volume of the image.
        bool encompasses_contour_of_points(const contour_of_points<R> &in) const;

        //Determine if at least one contour is fully contained within the 3D volume of the image.
        bool encompasses_any_contour_in_collection(const contour_collection<R> &in) const;

        //Computes the R^3 center of the image. Nothing fancy.
        vec3<R> center(void) const;

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

/*
        //Plot an outline of the image. Useful for alignment testing.
        void Plot_Outline(void) const;

        //Plot the pixels, disregarding spatial information.
        void Plot_Pixels(long int chnl) const;
        void Plot_Pixels_RGB(long int Rchnl = 0, long int Gchnl = 1, long int Bchnl = 2) const;
*/

        //Dump pixel data in various formats to a file. No metadata included!
        bool Dump_Pixels(const std::string &filename) const;
        bool Dump_d64_Pixels(const std::string &filename) const;  //As 64bit doubles.
        bool Dump_u16_scale_Pixels(const std::string &filename, bool AutoScaleToFillRange = true) const;  //Autoscales the range to span the u16 range.

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
        planar_image_collection(const std::list<planar_image<T,R>> &);

        //Member functions.
        planar_image_collection & operator=(const planar_image_collection &); //Performs a deep copy (unless copying self).
        bool operator==(const planar_image_collection &) const; //Relies on the planar_image operator==. See caveats there.
        bool operator!=(const planar_image_collection &) const;
        bool operator< (const planar_image_collection &) const; //Relies on the planar_image operator< . See caveats there. Only use for sorting.

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

        std::list<images_list_it_t> get_all_images(void);
        std::list<images_list_it_t> get_images_satisfying(std::function<bool(const planar_image<T,R> &animg)> select_pred);
        std::list<images_list_it_t> get_images_which_encompass_point(const vec3<R> &in);
        std::list<images_list_it_t> get_images_which_encompass_all_points(const std::list<vec3<R>> &in);
        std::list<images_list_it_t> get_images_which_sandwich_point_within_top_bottom_planes(const vec3<R> &in);

        //Image pruning/partitioning routine. Returns 'pruned' images; retains the rest. If pruning predicate is true, image is pruned.
        planar_image_collection<T,R> Prune_Images_Satisfying(std::function<bool(const planar_image<T,R> &animg)> prune_pred);

        //Generic routine for processing/combining groups of images into single images with a user-defined operation. This is useful 
        // for spatial averaging, blurring, etc.. Note that this routine modifies *this, so deep-copy beforehand if needed. See 
        // in-source default functors for descriptions/examples.
        //
        // Note that this routine converts groups of images into single images and then *erases* all but one of the old images.
        // This is a feature; many algorithms condense groups of images into a single image.
        bool Process_Images( std::function<typename std::list<images_list_it_t> (images_list_it_t,
                                      std::reference_wrapper<planar_image_collection<T,R>> )>         image_grouper,
                             std::function<bool (images_list_it_t, 
                                      std::list<images_list_it_t>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::experimental::any )>                                       operation_functor, 
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::experimental::any                                                   user_data = std::experimental::any());


        //Generic routine for performing an operation on images which may depend on external images (such as pixel maps).
        // No images are deleted, though the user has full access to the image data of all images. This routine is complementary
        // to the Process_Images() routine, and has different use-cases and approaches.
        bool Transform_Images( std::function<bool (images_list_it_t, 
                                        std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                        std::list<std::reference_wrapper<contour_collection<R>>>,
                                        std::experimental::any )>                                          op_func,
                               std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
                               std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
                               std::experimental::any                                                      user_data = std::experimental::any());

/*
        //Generic routine for computing histograms, time courses, or any sort of distribution from a grouping of images.
        // No images are deleted, though the user has full access to the image data of all images.
        bool Compute_Images( std::function<typename std::list<images_list_it_t> (images_list_it_t,
                                      std::reference_wrapper<planar_image_collection<T,R>> )>         image_grouper,
                             std::function<bool (images_list_it_t,
                                      std::list<images_list_it_t>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::map<    > )>                                               operation_functor,
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::map<   >                                                            time_courses );
*/


        //Condense groups of images to a single image by averaging at the pixel level, channel by channel. Only the final,
        // averaged image remains.
        bool Condense_Average_Images( std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                                               std::reference_wrapper<planar_image_collection<T,R>>)>    image_grouper );

        //Blur pixels isotropically, independently in their image plane, completely ignoring pixel shape and real-space coordinates.
        // Leave chnls empty for all channels. For more selectivity, run on each image separately.
        bool Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);

        //Computes the R^3 center of the images.
        vec3<R> center(void) const;

        //Compare the geometrical (non-pixel/voxel) aspects of the images to one another.
        bool Spatially_eq(const planar_image_collection<T,R> &in) const;

        //Collates images together, taking ownership of input image data on success. Behaviour can be specified.
        // Note this routine does not *combine* image data -- just collections of images.
        // Should always work if either image collection is empty.
        bool Collate_Images(planar_image_collection<T,R> &in, bool GeometricalOverlapOK = true);
/*
        //Spits out a default R^3 plot of the image outlines.
        void Plot_Outlines(const std::string &title) const;
        void Plot_Outlines(void) const;
*/
};

#endif

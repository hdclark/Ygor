//YgorImages.h
#ifndef YGOR_IMAGES_HDR_GRD_H
#define YGOR_IMAGES_HDR_GRD_H

#include <memory>
#include <string>
#include <list>

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

        R pxl_dx;//(row)    //This is the R^3 spacing of adjacent pixels. It is nearest-neighbour distance
        R pxl_dy;//(col)    // in the image plane (where |x| points along the row unit vector - see below).
        R pxl_dz;           //This is the R^3 'thickness' of the image. A thickness of zero is perfectly
                            // fine. (The meaning of the thickness of a plane is mostly up to the user.)
                            // It is treated as the thickness along the normal to row and col unit vectors.
                            // It is considered to be half above and half below plane of image when
                            // internal calculations are needed.
                            //
                            //The volume of a single voxel should be equal to pxl_dx*pxl_dy*pxl_dz.

        vec3<R> anchor;     //An (arbitrary) point in R^3 in which the image is considered to be anchored to.
                            // The edges, corners, and center of the image are given relative to this.
                            // The point is virtual, and should be thought of as the origin of a frame of 
                            // reference (which may not be at the numbers (0,0,0)).
                            //NOTE: This vector is most likely to be (0,0,0) for typical usage.

        vec3<R> offset;     //A vector from the anchor's terminus to the *center* of the pixel with image 
                            // coordinates (0,0). It is how we specify the position of the 2D data in R^3.

        vec3<R> row_unit;   //An orientation unit vector denoting the (positive) direction of row indices.
                            // For example, if R denotes the location of pixel(i,j), then
                            // [R' = R + row_unit*pxl_dx] is the location of pixel(i+1,j) (if it exists).

        vec3<R> col_unit;   //An orientation unit vector denoting the (positive) direction of col indices.
                            // For example, if R denotes the location of pixel(i,j), then
                            // [R' = R + col_unit*pxl_dy] is the location of pixel(i,j+1) (if it exists).

        R start_time;       //Temporal beginning and end of the image. Useful for keyframing. There is no
        R end_time;         // specified unit here! Interpretation is up to the user.

        int64_t sort_key_A; //Explicit ordering key for overriding the default sort order for collections
        int64_t sort_key_B; // of images. (This key is merely available in case you want to use it by 
                            // passing in a custom sorting routine.) Interpretation is ultimately up to the
                            // user, but this is the default primary sorting key.

        //------------------------------------ Member functions --------------------------------------------
        //Zero-based indexing (the default, and used internally).
        long int index(long int r, long int c) const;    //Note: For external access, prefer this->value().
        long int index(long int row, long int col, long int chnl) const;

        long int index(const vec3<R> &point, long int chnl) const; //Returns -1 on failure/out of bounds.

    //public:

        //------------------------------------ Member functions --------------------------------------------
        //Constructor/Destructors.
        planar_image();
        planar_image(const planar_image<T,R> &in);
        ~planar_image();

        //Allocating space and initializing the purely-2D-image members.
        void init_buffer(long int rows, long int columns, long int channels);

        //Initializing the R^3 members. These are less important because they won't cause a segfault.
        // If one wants to simply deal with R^2 image data, they could probably skip these.
        void init_spatial(R pxldx, R pxldy, R pxldz, const vec3<R> &anchr, const vec3<R> &offst);
        void init_temporal(R start, R end);
        void init_orientation(const vec3<R> &rowunit, const vec3<R> &colunit);

        planar_image & operator= (const planar_image &); //Performs a deep copy (unless copying self).
        bool operator==(const planar_image &) const; //Compares image position, dimensions, orientation, and finally image contents.
        bool operator!=(const planar_image &) const;
        bool operator< (const planar_image &) const; //This is mostly for sorting / placing into a std::map. There is no great 
                                                     // way to define '<' for images, so be careful with it. This is designed to be fast, not reliable!

        //Spatial comparison operators. Recovers natural ordering when possible. Handles different anchors.
        bool Spatially_eq(const planar_image &) const;
        bool Spatially_lt(const planar_image &) const;
        bool Spatially_lte(const planar_image &) const;

        //Temporal comparison operators. Recovers natural ordering when possible.
        bool Temporally_eq(const planar_image &) const;
        bool Temporally_lt(const planar_image &) const;
        bool Temporally_lte(const planar_image &) const;

        //Get the value of a channel.
        T value(long int row, long int col, long int chnl) const; //Fails on out-of-bounds input.
        T value(const vec3<R> &point, long int chnl) const; //Returns the value of the voxel which contains the point, or zero.

        //Get a reference to the value of a channel. This can be used to set it.
        T& reference(long int row, long int col, long int chnl);

        //Get an R^3 position of the *center* of the pixel/voxel.
        vec3<R> position(long int row, long int col) const;

        //Determine if a given point in R^3 is encompassed by the 3D volume of the image (using 'thickness' pxl_dz).
        bool encompasses_point(const vec3<R> &in) const; //Note: does not consider points on the boundary encompassed.

        //Determine if a given point is in the infinite, thick plane defined by the 'top' and 'bottom' of image.
        bool sandwiches_point_within_top_bottom_planes(const vec3<R> &in) const;

        //Determine if a given time is (inclusively) encompassed by the image's temporal extent.
        bool encompasses_time(R t) const;

        //Computes the R^3 center of the image. Nothing fancy.
        vec3<R> center(void) const;

        //The central time of the image's temporal extent.
        R temporal_center(void) const;

        //Returns an ordered list of the corners of the 2D image. Does NOT use thickness!
        std::list<vec3<R>> corners2D(void) const; 

        //Returns true if the 3D volume of this image encompasses the 2D image of the given planar image.
        bool encloses_2D_planar_image(const planar_image<T,R> &in) const; //Note: considers images on the boundary enclosed.

//DEPRECATED MEMBER:        //Compare the geometrical (non-pixel/voxel) aspects of the image to another.
//                          bool Compare_Geom(const planar_image<T,R> &in) const;
//

        //Temporal overlap of the (start --> end) ranges.
        bool Temporally_Overlap(const planar_image<T,R> &in) const;

        //Plot an outline of the image. Useful for alignment testing.
        void Plot_Outline(void) const;

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

        //Ordering operations, useful for easier iteration afterward. Feel free to add whatever here...
        void Stable_Sort_on_Sort_Keys_Temporal(void);
        void Stable_Sort_on_Temporal_Sort_Keys(void);
        void Stable_Sort_on_Temporal_Spatial(void);
        void Stable_Sort_on_Spatial_Temporal(void);
        void Stable_Sort_on_Sort_Keys_Spatial(void);
        void Stable_Sort_on_Spatial_Sort_Keys(void);


        //Returns a list of list::const_iterators to images which encompass a given point.
        // Be careful not to invalidate the data after calling these!
        std::list<typename std::list<planar_image<T,R>>::const_iterator> get_images_which_encompass_point(const vec3<R> &in) const;

        std::list<typename std::list<planar_image<T,R>>::const_iterator> get_images_which_sandwich_point_within_top_bottom_planes(const vec3<R> &in) const;

        //Returns a list of list::const_iterators to images which temporally overlap a given timepoint.
        // Be careful not to invalidate the data after calling these!
        std::list<typename std::list<planar_image<T,R>>::const_iterator> get_images_which_encompasses_time(R t) const;
       


        //Computes the R^3 center of the images.
        vec3<R> center(void) const;

        //Compare the geometrical (non-pixel/voxel) aspects of the images to one another.
        bool Spatially_eq(const planar_image_collection<T,R> &in) const;

        //Collates images together, taking ownership of input image data on success. Behaviour can be specified.
        // Note this routine does not *combine* image data -- just collections of images.
        // Should always work if either image collection is empty.
        bool Collate_Images(planar_image_collection<T,R> &in, bool SortKeyAOverlapOK = true, 
                                                              bool SortKeyBOverlapOK = true,
                                                              bool GeometricalOverlapOK = true, 
                                                              bool TemporalOverlapOK = true);

        //Spits out a default R^3 plot of the image outlines.
        void Plot_Outlines(const std::string &title) const;
        void Plot_Outlines(void) const;

};


//---------------------------------------------------------------------------------------------------------------------------
//--------------------------- postscriptinator: a thin class for generating Postscript images -------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class generates Postscript files from simple data. It was developed to convert the workflow 
// R^3 -> OpenGL Bitmap -> R^3 (using potrace or similar) into just R^3.

template <class R> class postscriptinator {
    R xmin, xmax, ymin, ymax;  //Drawing coordinates.
    R PageW, PageH;            //Page geometry (in [cm]).
    bool Enable_Auto_Sizing;   //Default is yes. Will be flipped to no if size explicitly set by user.
    std::string Definitions;
    std::string Header; //Note: Does NOT hold the '%!PS' at the top!
    std::list<std::string> Stack; //Holds postscript commands. May morph into something more easily parseable (true stack)...
    std::string Footer;

    //Constructor.
    postscriptinator();

    //Methods. 
    void Import_Contour(const contour_of_points<R> &C, const vec3<R> &Proj2);


    std::string Generate_Page_Geom(void) const;
    std::string Assemble(void) const; //Assembles all the pieces into a single string.
};


#endif

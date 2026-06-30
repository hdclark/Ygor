# Boost Replacement Needs

Ygor previously depended on Boost in two areas: XML-style named serialization adapters and Chebyshev exponential approximation support.

## Serialization

Source locations:

- `src/YgorMathIOBoostSerialization.h`
- `src/YgorImagesIOBoostSerialization.h`
- `src/YgorMathChebyshevIOBoostSerialization.h`
- `tests/Test_Math_11.cc`
- `tests/Test_Images_02.cc`
- `tests/compile.sh`

Boost components previously used:

- `boost::archive::xml_oarchive` and `boost::archive::xml_iarchive`
- `boost::serialization::make_nvp`
- Boost.Serialization adapters for `std::string`, `std::array`, `std::list`, `std::vector`, and `std::map`
- `boost::serialization::base_object` for serializing `line_segment<T>` through its `line<T>` base
- `boost::serialization::split_free` for separate save/load handling of `cheby_approx<T>`

Required replacement behavior:

- Header-only named serialization API usable from tests and downstream headers.
- Output and input archive types with `operator&`, `operator<<`, and `operator>>` for named values.
- Serialization support for arithmetic values, `std::string`, `std::array`, `std::list`, `std::vector`, and `std::map`.
- Adapters for the Ygor math/image types that had Boost.Serialization adapters: `vec2<T>`, `vec3<T>`, `line<T>`, `line_segment<T>`, `plane<T>`, `contour_of_points<T>`, `contour_collection<T>`, `fv_surface_mesh<T,I>`, `point_set<T>`, `lin_reg_results<T>`, `samples_1D<T>`, `planar_image<T,R>`, `planar_image_collection<T,R>`, and `cheby_approx<T>`.

Replacement implementation:

- `src/YgorIOXMLSerialization.h` provides a lightweight C++17 named text archive API under `ygor::serialization`.
- `src/YgorMathIOSerialization.h`, `src/YgorImagesIOSerialization.h`, and `src/YgorMathChebyshevIOSerialization.h` provide Ygor type adapters.
- The old compatibility header filenames include the new adapters without requiring Boost.

## Chebyshev Bessel Function

Source locations:

- `src/YgorMathChebyshevFunctions.h`
- `src/YgorMathChebyshevFunctions.cc`

Boost component previously used:

- `boost::math::cyl_bessel_i` from `<boost/math/special_functions/bessel.hpp>`

Required replacement behavior:

- Compute the modified cylindrical Bessel function of the first kind for the analytic Chebyshev exponential approximation.
- Remain available in C++17 builds without a feature macro or optional dependency.

Replacement implementation:

- Use the C++17 standard library special math function `std::cyl_bessel_i` from `<cmath>`.

## Build And Packaging

Locations updated:

- `CMakeLists.txt`
- `cmake/YgorConfig.cmake.in`
- `src/YgorDefinitions.h.in`
- `tests/compile.sh`
- `.gitlab-ci.yml`
- `PKGBUILD`
- `nix/ygor_derivation.nix`
- `README.md`

Required replacement behavior:

- No `WITH_BOOST` option, no generated `YGOR_USE_BOOST` definition, no `find_package(Boost)`, no exported `find_dependency(Boost)`, and no Boost package/link dependencies.

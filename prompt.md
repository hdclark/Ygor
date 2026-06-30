You are a senior C++ software engineer. 

Ygor is a C++17 library that provides support to several downstream projects. One major limitation is that Ygor depends on external package Boost for some critical functionality.

Your task is to remove the external dependency on Boost by providing substitute functionality where needed. For all source locations currently depending on Boost, identify the specific components and functionality needed, document the needs in a markdown file `needs.md`, build replacements in Ygor, remove the existing Boost invocations, and then validate that the code still builds correctly and all unit tests continue to pass. Don't forget to update the build system.

The Boost replacements should work cross-platform, but only need to target C++17. Adhere to local styles and conventions, and strictly adhere to the C++17 standard for all C++ code. Be meticulous and do not introduce any bugs.


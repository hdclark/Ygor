# Component 02 validation failure

apply outcome: success
GCC outcome: failure
Clang outcome: skipped

```text
-- The CXX compiler identification is GNU 13.3.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Looking for __arm__
-- Looking for __arm__ - not found
-- Looking for __aarch64__
-- Looking for __aarch64__ - not found
Assuming libstdc++fs library '/usr/lib/gcc/x86_64-linux-gnu/12/libstdc++fs.a;/usr/lib/gcc/x86_64-linux-gnu/13/libstdc++fs.a;/usr/lib/gcc/x86_64-linux-gnu/14/libstdc++fs.a' is needed.
-- Assuming Linux-specific sys interfaces are available.
-- Configuring done (316.9s)
-- Generating done (0.0s)
-- Build files have been written to: /home/runner/work/Ygor/Ygor/build-gcc
[  0%] Building CXX object src/YgorMeshesBooleanBounded/CMakeFiles/ygor_exact_float_expansion_core.dir/__/YgorMeshesExactFloatExpansionCore.cc.o
[  1%] Building CXX object src/YgorMeshesBooleanBounded/CMakeFiles/ygor_mesh_boolean_bounded_facade.dir/Cancellation.cc.o
[01m[Kcc1plus:[m[K [01;35m[Kwarning: [m[K‘[01m[K-fstack-check=[m[K’ and ‘[01m[K-fstack-clash-protection[m[K’ are mutually exclusive; disabling ‘[01m[K-fstack-check=[m[K’
[01m[Kcc1plus:[m[K [01;35m[Kwarning: [m[K‘[01m[K-fstack-check=[m[K’ and ‘[01m[K-fstack-clash-protection[m[K’ are mutually exclusive; disabling ‘[01m[K-fstack-check=[m[K’
[  2%] Building CXX object src/YgorMeshesBooleanBounded/CMakeFiles/ygor_exact_float_expansion_core.dir/__/YgorMeshesAdaptivePredicates.cc.o
[01m[Kcc1plus:[m[K [01;35m[Kwarning: [m[K‘[01m[K-fstack-check=[m[K’ and ‘[01m[K-fstack-clash-protection[m[K’ are mutually exclusive; disabling ‘[01m[K-fstack-check=[m[K’
[  4%] Linking CXX shared library ../../lib/libygor_exact_float_expansion_core.so
[  4%] Built target ygor_exact_float_expansion_core
[  5%] Building CXX object src/YgorMeshesBooleanBounded/CMakeFiles/ygor_mesh_boolean_bounded_facade.dir/YgorMeshesBooleanBounded.cc.o
[01m[Kcc1plus:[m[K [01;35m[Kwarning: [m[K‘[01m[K-fstack-check=[m[K’ and ‘[01m[K-fstack-clash-protection[m[K’ are mutually exclusive; disabling ‘[01m[K-fstack-check=[m[K’
[  5%] Building CXX object src/YgorMeshesBooleanBounded/CMakeFiles/ygor_mesh_boolean_bounded_strict.dir/BoundedOperations.cc.o
[01m[Kcc1plus:[m[K [01;35m[Kwarning: [m[K‘[01m[K-fstack-check=[m[K’ and ‘[01m[K-fstack-clash-protection[m[K’ are mutually exclusive; disabling ‘[01m[K-fstack-check=[m[K’
In file included from [01m[K/home/runner/work/Ygor/Ygor/src/YgorMeshesBooleanBounded/../YgorMeshesBooleanBounded.h:3[m[K,
                 from [01m[K/home/runner/work/Ygor/Ygor/src/YgorMeshesBooleanBounded/Outcome.h:3[m[K,
                 from [01m[K/home/runner/work/Ygor/Ygor/src/YgorMeshesBooleanBounded/BoundedOperations.h:5[m[K,
                 from [01m[K/home/runner/work/Ygor/Ygor/src/YgorMeshesBooleanBounded/BoundedOperations.cc:1[m[K:
[01m[K/home/runner/work/Ygor/Ygor/src/YgorMeshesBooleanBounded/../YgorMath.h:23:10:[m[K [01;31m[Kfatal error: [m[KYgorDefinitions.h: No such file or directory
   23 | #include [01;31m[K"YgorDefinitions.h"[m[K
      |          [01;31m[K^~~~~~~~~~~~~~~~~~~[m[K
compilation terminated.
gmake[3]: *** [src/YgorMeshesBooleanBounded/CMakeFiles/ygor_mesh_boolean_bounded_strict.dir/build.make:79: src/YgorMeshesBooleanBounded/CMakeFiles/ygor_mesh_boolean_bounded_strict.dir/BoundedOperations.cc.o] Error 1
gmake[2]: *** [CMakeFiles/Makefile2:1241: src/YgorMeshesBooleanBounded/CMakeFiles/ygor_mesh_boolean_bounded_strict.dir/all] Error 2
gmake[2]: *** Waiting for unfinished jobs....
[  5%] Built target ygor_mesh_boolean_bounded_facade
gmake[1]: *** [CMakeFiles/Makefile2:1348: tests/mesh_boolean_bounded/CMakeFiles/ygor_mesh_boolean_input_validation_tests.dir/rule] Error 2
gmake: *** [Makefile:628: ygor_mesh_boolean_input_validation_tests] Error 2
```

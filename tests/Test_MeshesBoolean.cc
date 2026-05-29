
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <YgorLog.h>
#include <YgorMathIOOBJ.h>
#include <YgorMeshesBoolean.h>


int main(int argc, char **argv){

    // Read two meshes from file.
    fv_surface_mesh<double, uint64_t> mesh_A;
    {
        std::fstream f("/tmp/dcma_mesh_A_extruded.obj", std::ifstream::in);
        ReadFVSMeshFromOBJ(mesh_A, f);
        f.close();
    }

    fv_surface_mesh<double, uint64_t> mesh_B;
    {
        std::fstream f("/tmp/dcma_mesh_B_parent.obj", std::ifstream::in);
        ReadFVSMeshFromOBJ(mesh_B, f);
        f.close();
    }

    // Perform a Boolean operation.
    auto res = BooleanSubtraction(mesh_B, mesh_A, /*max_depth=*/5, /*boundary_scale=*/0.0);

    // Attempt to write the resulting mesh to file for debugging/inspection purposes.
    {
        std::fstream f("/tmp/dcma_mesh_C_output.obj", std::ifstream::out);
        WriteFVSMeshToOBJ(res, f);
        f.close();
    }

    return 0;
}


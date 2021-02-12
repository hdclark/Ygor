// Test of convex hull routine.

#include <limits>
#include <utility>
#include <iostream>
#include <random>

#include <YgorMath.h>
#include <YgorMathIOPLY.h>
#include <YgorMisc.h>


int main(int argc, char **argv){

    long int random_seed = 123456;
    std::mt19937 re( random_seed );
    std::uniform_real_distribution<> rd(0.0, 1.0); //Random distribution.

    std::vector<vec3<double>> all_verts;
    for(size_t i = 0; i < 50000; ++i){
        all_verts.emplace_back(vec3<double>(rd(re), rd(re), rd(re)));
    }

    // Shuffle order, so first vertices are unlikely to be present in the final hull.
    std::shuffle(std::begin(all_verts), std::end(all_verts), re);

    std::vector< std::reference_wrapper<vec3<double>> > all_vert_refs;
    for(auto &v : all_verts){
        all_vert_refs.emplace_back( std::ref(v) );
    }

    auto faces = Convex_Hull<double,uint32_t>(all_vert_refs);

    // Dump the mesh for inspection.
    fv_surface_mesh<double, uint32_t> mesh;
    mesh.vertices = all_verts;
    mesh.faces = faces;
    std::ofstream of("convex_hull.ply");
    if(!WriteFVSMeshToPLY(mesh, of)){
        throw std::runtime_error("Unable to write file.");
    }

    return 0;
}



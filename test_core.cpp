#include "src/dune3d_core_api.h"
#include <iostream>
#include <cassert>
#include <cstring>

int main() {
    std::cout << "Initializing Dune3D Core..." << std::endl;
    dune3d_init();

    std::cout << "Creating new document..." << std::endl;
    void* doc = dune3d_new_document();
    assert(doc != nullptr);

    std::cout << "Converting document to JSON..." << std::endl;
    char* json_str = dune3d_document_to_json(doc);
    assert(json_str != nullptr);
    std::cout << "JSON: " << json_str << std::endl;
    dune3d_free_string(json_str);

    std::cout << "Solving document..." << std::endl;
    int res = dune3d_solve_document(doc);
    std::cout << "Solve result: " << res << std::endl;

    std::cout << "Deleting document..." << std::endl;
    dune3d_delete_document(doc);

    std::cout << "Test passed!" << std::endl;
    return 0;
}

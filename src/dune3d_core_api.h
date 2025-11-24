#ifndef DUNE3D_CORE_API_H
#define DUNE3D_CORE_API_H

#ifdef WIN32
    #ifdef BUILDING_DUNE3D_CORE
        #define DUNE3D_API __declspec(dllexport)
    #else
        #define DUNE3D_API __declspec(dllimport)
    #endif
#else
    #define DUNE3D_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // Initialize the library
    DUNE3D_API void dune3d_init();

    // Create a new document
    DUNE3D_API void* dune3d_new_document();

    // Destroy a document
    DUNE3D_API void dune3d_delete_document(void* doc_ptr);

    // Get document JSON representation (for initial testing)
    // Returns a malloc-ed string that must be freed by the caller
    DUNE3D_API char* dune3d_document_to_json(void* doc_ptr);

    // Example of solving a system (placeholder)
    DUNE3D_API int dune3d_solve_document(void* doc_ptr);

    // Free string memory
    DUNE3D_API void dune3d_free_string(char* str);

#ifdef __cplusplus
}
#endif

#endif // DUNE3D_CORE_API_H

#include "dune3d_core_api.h"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "system/system.hpp"
#include "util/json_util.hpp"
#include "nlohmann/json.hpp"
#include <cstring>
#include <cstdlib>
#include <iostream>

using namespace dune3d;

void dune3d_init() {
    // Initialization code if needed
}

void* dune3d_new_document() {
    return new Document();
}

void dune3d_delete_document(void* doc_ptr) {
    if (doc_ptr) {
        delete static_cast<Document*>(doc_ptr);
    }
}

char* dune3d_document_to_json(void* doc_ptr) {
    if (!doc_ptr) return nullptr;
    Document* doc = static_cast<Document*>(doc_ptr);
    nlohmann::json j = doc->serialize();
    std::string s = j.dump();
    char* result = (char*)malloc(s.length() + 1);
    strcpy(result, s.c_str());
    return result;
}

int dune3d_solve_document(void* doc_ptr) {
    if (!doc_ptr) return -1;
    Document* doc = static_cast<Document*>(doc_ptr);

    // Solve all solve-able groups (sketches, etc.)
    int result_agg = 0; // 0 = OK

    for (auto group : doc->get_groups_sorted()) {
        // Check if group is solvable (e.g. Sketch) or just needs update
        // For simplicity, we rely on Document::update_pending mechanism which triggers solve
        // However, update_pending usually runs on idle or when requested.
        // Let's try to force an update on the document.

        // Alternatively, we can explicitly construct a System for specific groups.
        // But Document::update_pending() is the high-level way.
    }

    // Force an update of all pending items.
    doc->update_pending();

    // Check results of groups
    for (auto group : doc->get_groups_sorted()) {
        if (group->m_solve_result != SolveResult::OKAY && group->m_solve_result != SolveResult::REDUNDANT_OKAY) {
            result_agg = -1;
        }
    }

    return result_agg;
}

void dune3d_free_string(char* str) {
    if (str) {
        free(str);
    }
}

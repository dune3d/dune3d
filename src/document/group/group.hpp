#pragma once
#include "util/uuid.hpp"
#include "util/badge.hpp"
#include "nlohmann/json_fwd.hpp"
#include "system/solve_result.hpp"
#include <memory>
#include <optional>
#include <list>
#include <set>

namespace dune3d {
using json = nlohmann::json;

class Document;

class Body {
public:
    explicit Body(const json &j);
    Body();

    std::string m_name = "Body";

    json serialize() const;
};

struct GroupStatusMessage {
    enum class Status { NONE, INFO, WARN, ERR };
    Status status;
    std::string message;

    static GroupStatusMessage::Status summarize(const std::list<GroupStatusMessage> &msgs);
};

enum class GroupType {
    REFERENCE,
    SKETCH,
    EXTRUDE,
    LATHE,
    FILLET,
    CHAMFER,
    LINEAR_ARRAY,
    POLAR_ARRAY,
};

class Group {
public:
    UUID m_uuid;
    using Type = GroupType;
    virtual Type get_type() const = 0;
    static std::string get_type_name(Type type);
    std::string get_type_name() const;

    virtual ~Group();
    virtual json serialize() const;
    virtual json serialize(const Document &doc) const;

    static std::unique_ptr<Group> new_from_json(const UUID &uu, const json &j);
    virtual std::unique_ptr<Group> clone() const = 0;

    std::string m_name;
    int m_dof = -1;
    SolveResult m_solve_result = SolveResult::OKAY;
    std::optional<std::vector<UUID>> m_bad_constraints;

    std::set<UUID> find_redundant_constraints(Document &doc);

    auto get_index() const
    {
        return m_index;
    }
    void set_index(Badge<Document>, int index)
    {
        m_index = index;
    }

    std::optional<Body> m_body;
    UUID m_active_wrkpl;

    struct BodyAndGroup {
        const Body &body;
        const Group &group;
    };
    BodyAndGroup find_body(const Document &doc) const;

    std::list<GroupStatusMessage> m_solve_messages;

    virtual std::list<GroupStatusMessage> get_messages() const
    {
        return m_solve_messages;
    }

    // entities referenced by entities, constraints or the group itself
    virtual std::set<UUID> get_referenced_entities(const Document &doc) const;

    // groups for the above entities and other groups directly referenced
    virtual std::set<UUID> get_referenced_groups(const Document &doc) const;

    virtual std::set<UUID> get_required_entities(const Document &doc) const;
    virtual std::set<UUID> get_required_groups(const Document &doc) const;

protected:
    explicit Group(const UUID &uu, const json &j);
    explicit Group(const UUID &uu);

private:
    int m_index = 0;
};

} // namespace dune3d

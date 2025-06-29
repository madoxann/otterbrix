#pragma once

#include "schema.hpp"
#include "table_id.hpp"

namespace catalog {
    struct table_metadata {
        // Partition specs - no partitioning in otterbrix
        //        std::vector<PartitionSpec> partition_specs;
        //        SpecId default_spec_id;
        //        FieldId last_partition_id;

        // Sort orders - no such feature in otterbrix
        //        std::vector<SortOrder> sort_orders;
        //        int default_sort_order_id;

        // Snapshots - no snapshots + logical plan does NOT include disk info (actors?)
        //        std::optional<SnapshotId> current_snapshot_id;
        //        std::vector<Snapshot> snapshots;
        //        std::vector<SnapshotLogEntry> snapshot_log;
        //        std::vector<MetadataLogEntry> metadata_log;

        // Helper methods
        schema currentSchema() const;

        nlohmann::json toJson() const;
        static table_metadata fromJson(const nlohmann::json& j);

    private:
        table_uuid table_uuid;
        schema schema;
        iceberg::timestamp last_updated_ms; // ?
        iceberg::field_id last_column_id;   // increment with new version
        // may be extended with new data as needed
    };
} // namespace catalog
#pragma once

#include "schema.hpp"
#include "table_id.hpp"

namespace catalog {
    struct table_metadata {
        table_metadata(std::pmr::memory_resource* resource);

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
        schema current_schema() const;

        std::pmr::string to_json() const;
        static table_metadata from_json(const std::string& j, std::pmr::memory_resource* resource);

    private:
        table_uuid table_uuid;
        schema schema;
        iceberg::timestamp last_updated_ms; // ?
        iceberg::field_id last_column_id;   // increment with new version
        // may be extended with new data as needed
    };
} // namespace catalog
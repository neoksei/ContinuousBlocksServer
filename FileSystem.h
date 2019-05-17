//
// Created by hivemind on 16/05/19.
//

#ifndef CONTINUOUSBLOCKS_FILESYSTEM_H
#define CONTINUOUSBLOCKS_FILESYSTEM_H

#include <climits>
#include <cstdio>
#include <exception>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <fstream>

#define CLUSTER_FREE SIZE_MAX
#define CLUSTER_BUSY 0
#define RECORD_FREE SIZE_MAX
#define FS_FAIL SIZE_MAX


struct Record {
    char name[16];
    size_t index;
    size_t size;
};


class FileSystem {
private:
    size_t n_clusters;
    size_t cluster_size;
    size_t total_clusters_busy;
    size_t *allocation_table;
    size_t table_index;
    char *storage;
    Record root_file[512];

    size_t calculate_nclusters(size_t data_size);

    size_t allocate_clusters(size_t amount);

    size_t save_data(size_t init_cluster, const char *data, size_t data_size);

    size_t show_available_space();

public:

    FileSystem(const char* dump_file, size_t _cluster_size);

    FileSystem(size_t _cluster_size, size_t _n_clusters);

    ~FileSystem();

    bool file_exists(const char* name);

    size_t write(const char *name, const char *file_data, size_t file_size);

    size_t delete_file(const char *name);

    char *read(const char *name);

    size_t get_file_size(const char *name);

    void dump();

};


#endif //CONTINUOUSBLOCKS_FILESYSTEM_H

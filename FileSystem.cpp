//
// Created by hivemind on 16/05/19.
//

#include "FileSystem.h"

FileSystem::FileSystem(const char *dump_file, size_t _cluster_size) {
    std::ifstream dump_stream(dump_file);
    dump_stream.seekg(0, std::ifstream::end);
    size_t length = dump_stream.tellg();
    dump_stream.seekg(0, std::ifstream::beg);
    cluster_size = _cluster_size;
    n_clusters = length / cluster_size;
    storage = new char[n_clusters * cluster_size];
    total_clusters_busy = 0;

    dump_stream.read(storage, length);
    dump_stream.close();

    allocation_table = new size_t[n_clusters]; // инициализация таблицы

    for (size_t i = 0; i < n_clusters; i++) {
        allocation_table[i] = *((size_t*)storage + i);
    }

    table_index = 0;

    for (size_t i = sizeof(size_t) * n_clusters; i < sizeof(Record)* 512 + sizeof(size_t) * n_clusters; i++) {
        *((char*)root_file + i - sizeof(size_t) * n_clusters) = storage[i];
    }

    for (size_t i = 0; i < n_clusters; i++) {
        if (allocation_table[i] == CLUSTER_BUSY) {
            ++total_clusters_busy;
        }
    }
}

FileSystem::FileSystem(size_t _cluster_size, size_t _n_clusters) {
    cluster_size = _cluster_size;
    n_clusters = _n_clusters;
    total_clusters_busy = 0;
    storage = new char[n_clusters * cluster_size]();

    allocation_table = new size_t[n_clusters]; // инициализация таблицы
    std::fill(allocation_table, allocation_table + n_clusters, CLUSTER_FREE);
    table_index = allocate_clusters(calculate_nclusters(sizeof(size_t) * n_clusters));
    save_data(table_index, (char *) allocation_table, sizeof(size_t) * n_clusters);

    for (size_t i = 1; i < 512; i++) {
        root_file[i].index = SIZE_MAX;
        root_file[i].size = 0;
        std::fill(root_file[i].name, root_file[i].name + sizeof(Record::name), 0);
    }

    strcpy(root_file[0].name, "/");
    root_file[0].size = sizeof(root_file);
    root_file[0].index = allocate_clusters(calculate_nclusters(sizeof(root_file)));
}

FileSystem::~FileSystem() {
    delete[] storage;
}

size_t FileSystem::calculate_nclusters(size_t data_size) {
    return (size_t) ceil((double) data_size / (double) cluster_size);
}

size_t FileSystem::allocate_clusters(size_t amount) {
    size_t skip_clusters = 0;

    for (size_t i = 0; i < n_clusters; i += skip_clusters + 1) {
        skip_clusters = 0;
        if (allocation_table[i] == CLUSTER_FREE) {
            skip_clusters = 1;
            for (size_t j = i + 1; j < i + amount && allocation_table[j] == CLUSTER_FREE; j++) {
                ++skip_clusters;
            }

            if (skip_clusters == amount) {

                for (size_t j = i; j < i + amount; j++) {
                    allocation_table[j] = CLUSTER_BUSY;
                }

                total_clusters_busy += amount;
                return i;
            }
        }
    }

}

size_t FileSystem::save_data(size_t init_cluster, const char *data, size_t data_size) {

    for (size_t i = 0; i < data_size; i++) {
        storage[init_cluster * cluster_size + i] = data[i];
    }

    return data_size;
}

size_t FileSystem::write(const char *name, const char *file_data, size_t file_size) {
    char *buffer = nullptr;
    size_t buffer_size;

    if (strlen(name) >= 16) { // неподходящее имя
        return  FS_FAIL;
    }


    for (Record x: root_file) { // если файл уже существует
        if (!strcmp(name, x.name)) { // стоит попробовать его перезаписать
            buffer = read(name);
            buffer_size = x.size;
            delete_file(name);
        }
    }


    if (calculate_nclusters(file_size) > show_available_space()) { // закончились кластеры

        if (buffer != nullptr) { // файл не удаляется, просто не изменяется
            write(name, buffer, buffer_size);
            delete[] buffer;
        }

        return FS_FAIL;
    }

    size_t file_index = SIZE_MAX;

    for (size_t i = 0; i < sizeof(root_file) / sizeof(Record); i++) { // поиск свободной записи в root_file

        if (root_file[i].index == RECORD_FREE) {
            file_index = i;
            break;
        }
    }

    if (file_index == SIZE_MAX) { // достигнут предел количества файлов
        return FS_FAIL;
    } else {
        strcpy(root_file[file_index].name, name); // инициализация записи в root_file
        root_file[file_index].size = file_size;
        root_file[file_index].index = allocate_clusters(calculate_nclusters(file_size));
        save_data(root_file[file_index].index, file_data, file_size);
        delete[] buffer; //очищаем буфер
        return file_index;
    }
}

char* FileSystem::read(const char *name) {
    size_t current_index = SIZE_MAX;
    Record record;

    for (Record x: root_file) { // поиск записи в root_file
        if (!strcmp(name, x.name)) {
            record = x;
            current_index = record.index;
            break;
        }
    }

    if (current_index == SIZE_MAX) { // файл не найден
        return nullptr;
    }

    char* buffer = new char[record.size];

    for (size_t i = 0; i < record.size; i++) {
        buffer[i] = storage[record.index * cluster_size + i];
    }

    return buffer;
}

size_t FileSystem::delete_file(const char *name) {
    size_t init_index = SIZE_MAX;
    size_t deleted_size;

    for (Record& x: root_file) { // ищем файл
        if (!strcmp(name, x.name)) {
            init_index = x.index; // сохраняем индекс
            x.index = RECORD_FREE; // файл удален в root_file
            deleted_size = x.size;
            break;
        }
    }

    if (init_index == SIZE_MAX) { // запись не найдена
        return FS_FAIL;
    }

    for (size_t i = 0; i < deleted_size; i++) {
        storage[init_index * cluster_size + i] = 0;
    }

    for (size_t i = init_index; i < init_index + calculate_nclusters(deleted_size); i++) {
        allocation_table[i] = CLUSTER_FREE;
    }

    return deleted_size;
}

size_t FileSystem::get_file_size(const char *name) {
    size_t file_size = SIZE_MAX;

    for (Record x: root_file) { // поиск записи в root_file
        if (!strcmp(name, x.name)) {
            file_size = x.size;
            break;
        }
    }

    return file_size;
}

size_t FileSystem::show_available_space() {
    size_t free_clusters = 0;
    size_t max_available_space = 0;

    for (size_t i = 0; i < n_clusters; i += free_clusters + 1) {
        free_clusters = 0;
        if (allocation_table[i] == CLUSTER_FREE) {
            free_clusters = 1;
            for (size_t j = i + 1; j < n_clusters && allocation_table[j] == CLUSTER_FREE; j++) {
                ++free_clusters;
            }

            if (free_clusters > max_available_space) {
                max_available_space = free_clusters;
            }
        }
    }

    return max_available_space;
}

void FileSystem::dump() {
    save_data(root_file[0].index, (char*) root_file, root_file[0].size);
    save_data(table_index, (char*) allocation_table, sizeof(size_t) * n_clusters);
    std::ofstream f = std::ofstream("../ContinuousDrive");
    f.write(storage, n_clusters * cluster_size);
    f.close();
}

bool FileSystem::file_exists(const char *name) {

    for (Record x: root_file) {
        if (!strcmp(name, x.name)) {
            return true;
        }
    }

    return false;
}

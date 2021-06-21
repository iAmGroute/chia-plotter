/*
 * Table.h
 *
 *  Created on: May 29, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_DISKTABLE_H_
#define INCLUDE_CHIA_DISKTABLE_H_

#include <chia/buffer.h>
#include <chia/ThreadPool.h>

#include <cstdio>


template<typename T>
class DiskTable {
private:
    struct local_t {
        FILE*       file   = nullptr;
        std::string fname  = "";
        uint8_t*    buffer = nullptr;
        ~local_t() {
            if (file) {
                std::cout << "c " << fname << std::endl;
                fclose(file);
            }
            delete [] buffer;
        }
    };

public:
    DiskTable(std::string file_name, size_t num_entries = 0)
        :    file_name(file_name),
            num_entries(num_entries)
    {
        if (!num_entries) {
            std::cout << "w " << file_name << std::endl;
            file_out = fopen(file_name.c_str(), "wb");
            if (!file_out) {
                throw std::runtime_error("fopen() failed " + file_name);
            }
        }
    }

    DiskTable(const table_t& info)
        :    DiskTable(info.file_name, info.num_entries)
    {
    }

    ~DiskTable() {
        close();
    }

    DiskTable(DiskTable&) = delete;
    DiskTable& operator=(DiskTable&) = delete;

    table_t get_info() const {
        table_t out;
        out.file_name = file_name;
        out.num_entries = num_entries;
        return out;
    }

    void read(
        Processor<std::pair<std::vector<T>, size_t>>* output,
        int                                           num_threads_read,
        const size_t                                  block_size
    ) const {
        if (num_threads_read > 256) throw std::logic_error("DT: too many read threads");
        if (block_size < 1024)      throw std::logic_error("DT: too small read size");

        ThreadPool<std::pair<size_t, size_t>, std::pair<std::vector<T>, size_t>, local_t>
        pool (
            std::bind(
                &DiskTable::read_block,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3
            ),
            output, num_threads_read, "Table/read"
        );

        for (size_t i = 0; i < pool.num_threads(); ++i)
        {
            std::cout << "r " << file_name << std::endl;
            FILE* file = fopen(file_name.c_str(), "rb");
            if (!file) {
                throw std::runtime_error("fopen() failed " + file_name);
            }
            auto& local  = pool.get_local(i);
            local.file   = file;
            local.fname  = file_name;
            local.buffer = new uint8_t[block_size * T::disk_size];
        }
        const size_t num_blocks = num_entries / block_size;
        const size_t left_over = num_entries % block_size;

        size_t offset = 0;
        for (size_t i = 0; i < num_blocks; ++i) {
            pool.take_copy(std::make_pair(offset, block_size));
            offset += block_size;
        }
        if (left_over) {
            pool.take_copy(std::make_pair(offset, left_over));
        }
        pool.close();
    }

    // NOT thread-safe
    void write(const T& entry) {
        if (cache.count >= cache.capacity) {
            flush();
        }
        entry.write(cache.entry_at(cache.count));
        cache.count++;
    }

    void flush() {
        if (fwrite(cache.data, cache.entry_size, cache.count, file_out) != cache.count) {
            throw std::runtime_error("fwrite() failed");
        }
        num_entries += cache.count;
        cache.count = 0;
    }

    void close() {
        if (file_out) {
            flush();
            std::cout << "c " << file_name << std::endl;
            fclose(file_out);
            file_out = nullptr;
        }
    }

private:
    void read_block(std::pair<size_t, size_t>& param,
                    std::pair<std::vector<T>, size_t>& out,
                    local_t& local) const
    {
        if (int err = fseek(local.file, param.first * T::disk_size, SEEK_SET)) {
            throw std::runtime_error("fseek() failed");
        }
        if (fread(local.buffer, T::disk_size, param.second, local.file) != param.second) {
            throw std::runtime_error("fread() failed");
        }
        auto& entries = out.first;
        entries.resize(param.second);
        for (size_t k = 0; k < param.second; ++k) {
            entries[k].read(local.buffer + k * T::disk_size);
        }
        out.second = param.first;
    }

private:
    std::string file_name;
    size_t num_entries;

    write_buffer_t<T> cache;
    FILE* file_out = nullptr;

};


#endif /* INCLUDE_CHIA_DISKTABLE_H_ */

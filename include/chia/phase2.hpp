/*
 * phase2.hpp
 *
 *  Created on: May 29, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_PHASE2_HPP_
#define INCLUDE_CHIA_PHASE2_HPP_

#include <chia/phase2.h>
#include <chia/DiskTable.h>
#include <chia/ThreadPool.h>

#include <chia/bitfield_index.hpp>


namespace phase2 {

template<typename T, typename S, typename DS>
void compute_table(
    int             R_index,
    DS*             R_sort,
    DiskTable<S>*   R_file,
    const table_t&  R_table,
    bitfield*       L_used,
    const bitfield* R_used
) {
    DiskTable<T> R_input(R_table);
    {
        const auto begin = get_wall_time_micros();

        ThreadPool<std::pair<std::vector<T>, size_t>, size_t>
        pool (
            [L_used, R_used](std::pair<std::vector<T>, size_t>& input, size_t&, size_t&)
            {
                uint64_t offset = 0;
                for (const auto& entry : input.first) {
                    if (R_used && !R_used->get(input.second + (offset++))) {
                        continue;    // drop it
                    }
                    L_used->set(entry.pos);
                    L_used->set(uint64_t(entry.pos) + entry.off);
                }
            },
            nullptr, G_P2_MARK_THREADS, "phase2/mark"
        );

        L_used->clear();
        R_input.read(&pool, G_P2_P1F_READ_THREADS, G_P2_P1F_READ_SIZE);
        pool.close();

        std::cout << "[P2] Table " << R_index << " scan took "
                << (get_wall_time_micros() - begin) / 1e6 << " sec" << std::endl;
    }
    const auto begin = get_wall_time_micros();

    uint64_t num_written = 0;
    const bitfield_index index(*L_used);

    typedef typename DS::WriteCache WriteCache;

    Thread<std::vector<S>> R_write (
        [R_file](std::vector<S>& input)
        {
            for (auto& entry : input) {
                R_file->write(entry);
            }
        },
        "phase2/write"
    );

    ThreadPool<std::vector<S>, size_t, std::shared_ptr<WriteCache>>
    R_add (
        [R_sort](std::vector<S>& input, size_t&, std::shared_ptr<WriteCache>& cache)
        {
            if (!cache) {
                cache = R_sort->add_cache();
            }
            for (auto& entry : input) {
                cache->add(entry);
            }
        },
        nullptr, G_P2_P2_WRITE_THREADS, "phase2/add"
    );

    Processor<std::vector<S>>* R_out = R_file ? &R_write : &R_add;

    Thread<std::vector<S>> R_count (
        [R_out, &num_written](std::vector<S>& input)
        {
            for (auto& entry : input) {
                set_sort_key<S>{}(entry, num_written++);
            }
            R_out->take(input);
        },
        "phase2/count"
    );

    ThreadPool<std::pair<std::vector<T>, size_t>, std::vector<S>>
    map_pool (
        [&index, R_used](std::pair<std::vector<T>, size_t>& input, std::vector<S>& out, size_t&)
        {
            out.reserve(input.first.size());
            uint64_t offset = 0;
            for (const auto& entry : input.first) {
                if (R_used && !R_used->get(input.second + (offset++))) {
                    continue;    // drop it
                }
                S tmp;
                tmp.assign(entry);
                const auto pos_off = index.lookup(entry.pos, entry.off);
                tmp.pos = pos_off.first;
                tmp.off = pos_off.second;
                out.push_back(tmp);
            }
        },
        &R_count, G_P2_REMAP_THREADS, "phase2/remap"
    );

    R_input.read(&map_pool, G_P2_P1F_READ_THREADS, G_P2_P1F_READ_SIZE);

    map_pool.close();
    R_count.close();
    R_write.close();
    R_add.close();

    if (R_file) R_file->close();

    std::cout << "[P2] Table " << R_index << " rewrite took "
                << (get_wall_time_micros() - begin) / 1e6 << " sec"
                << ", dropped " << R_table.num_entries - num_written << " entries"
                << " (" << 100 * (1 - double(num_written) / R_table.num_entries) << " %)" << std::endl;
}

inline void compute(
    const phase1::output_t& input,
                  output_t& out,
    const std::string       plot_name,
    const std::string       tree_dir
) {
    const auto total_begin = get_wall_time_micros();

    const std::string path   = tree_dir  + "p2/";
    const std::string prefix = plot_name + "_p2_";

    out.params = input.params;
    out.table_1 = input.table[0];

    size_t max_table_size = 0;
    for (const auto& table : input.table) {
        max_table_size = std::max(max_table_size, table.num_entries);
    }
    std::cout << "[P2] max_table_size = " << max_table_size << std::endl;

    auto curr_bitfield = std::make_shared<bitfield>(max_table_size);
    auto next_bitfield = std::make_shared<bitfield>(max_table_size);

    {
        DiskTable<entry_7> table_7(path+"t7f/"+prefix+"t7f.tmp");
        compute_table<entry_7, entry_7, DiskSort7>(7, nullptr, &table_7, input.table[6], next_bitfield.get(), nullptr);
        table_7.close();
        out.table_7 = table_7.get_info();
    }

    remove(input.table[6].file_name);

    for (int i = 5; i >= 1; i -= 1)
    {
        std::swap(curr_bitfield, next_bitfield);
        auto t_string = "t" + std::to_string(i+1);
        out.sort[i] = std::make_shared<DiskSortT>(32, G_LOG_NUM_BUCKETS, path+t_string+"/", prefix+t_string+"_");

        compute_table<phase1::tmp_entry_x, entry_x, DiskSortT>(
            i + 1, out.sort[i].get(), nullptr, input.table[i], next_bitfield.get(), curr_bitfield.get());

        remove(input.table[i].file_name);
    }

    out.bitfield_1 = next_bitfield;

    std::cout << "Phase 2 took " << (get_wall_time_micros() - total_begin) / 1e6 << " sec" << std::endl;
}


} // phase2

#endif /* INCLUDE_CHIA_PHASE2_HPP_ */

/*
 * buffer.h
 *
 *  Created on: Jun 7, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_BUFFER_H_
#define INCLUDE_CHIA_BUFFER_H_

#include <chia/settings.h>


template<typename T, size_t _capacity>
struct byte_buffer_t
{
    static constexpr size_t capacity   = _capacity;
    static constexpr size_t entry_size = T::disk_size;
    uint8_t data[capacity * entry_size];
    size_t  count = 0;

    uint8_t* entry_at(const size_t i) {
        return data + i * entry_size;
    }
};

template<typename T>
using  read_buffer_t = byte_buffer_t<T,  g_read_chunk_size>;

template<typename T>
using write_buffer_t = byte_buffer_t<T, g_write_chunk_size>;


#endif /* INCLUDE_CHIA_BUFFER_H_ */

/*
 * buffer.h
 *
 *  Created on: Jun 7, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_BUFFER_H_
#define INCLUDE_CHIA_BUFFER_H_

#include <chia/settings.h>


template<typename T, size_t capacity>
struct byte_buffer_t
{
    uint8_t data[capacity * entry_size];
    size_t  count = 0;

    uint8_t* entry_at(const size_t i) {
        return data + i * entry_size;
    }
};

template<typename T>
typedef byte_buffer_t<T,  g_read_chunk_size> read_buffer_t;

template<typename T>
typedef byte_buffer_t<T, g_write_chunk_size> write_buffer_t;


#endif /* INCLUDE_CHIA_BUFFER_H_ */

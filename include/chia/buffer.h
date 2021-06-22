/*
 * buffer.h
 *
 *  Created on: Jun 7, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_BUFFER_H_
#define INCLUDE_CHIA_BUFFER_H_

#include <chia/settings.h>


template<typename T, size_t _size>
struct byte_buffer_t
{
    static constexpr size_t entry_size = T::disk_size;
    static constexpr size_t capacity   = _size / entry_size;
    size_t  count = 0;
    uint8_t data[capacity * entry_size];

    uint8_t* entry_at(const size_t i) {
        return data + i * entry_size;
    }
};

template<typename T>
using  read_buffer_t = byte_buffer_t<T,  G_READ_CHUNK_SIZE>;

template<typename T>
using write_buffer_t = byte_buffer_t<T, G_WRITE_CHUNK_SIZE>;


#endif /* INCLUDE_CHIA_BUFFER_H_ */

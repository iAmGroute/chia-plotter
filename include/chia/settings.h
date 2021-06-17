/*
 * settings.h
 *
 *  Created on: Jun 7, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_SETTINGS_H_
#define INCLUDE_CHIA_SETTINGS_H_

#include <cstdint>
#include <cstddef>

constexpr size_t  g_read_chunk_size = 65536; // Number of table entries to read at once
constexpr size_t g_write_chunk_size = 32768; // Number of table entries to buffer before writing to disk

#endif /* INCLUDE_CHIA_SETTINGS_H_ */

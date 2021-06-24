// Included by settings.h only

#ifdef INCLUDE_CHIA_SETTINGS_H_

#define SETTINGS_FILE "settings_0"

// SIZE is in bytes

constexpr size_t  G_READ_CHUNK_SIZE = 1 * 1024 * 1024; // Bytes of table entries to read at once (per read thread)
constexpr size_t G_WRITE_CHUNK_SIZE =      256 * 1024; // Bytes of table entries to buffer before write (per bucket per thread)

constexpr size_t G_DT_WRITE_CACHE_SIZE = 16 * 1024 * 1024; // DiskTable's write cache size (bytes)

//

constexpr int G_LOG_NUM_BUCKETS         =  7; // log2 of number of buckets in DiskSort
constexpr int G_TOTAL_THREADS           = 24;

//                                              Current | Original

constexpr int G_P1T1_CALC_THREADS       = 24; // tot    | tot
constexpr int G_P1T1_P1T1_WRITE_THREADS =  8; //        | tot /2

constexpr int G_P1_P1_SORT_THREADS      = 24; // tot    | tot /2
constexpr int G_P1_P1_READ_THREADS      = 12; // sort/2 | sort/2
constexpr int G_P1_MATCH_THREADS        = 24; // tot    | tot
constexpr int G_P1_EVAL_THREADS         = 24; // tot    | tot
constexpr int G_P1_P1_WRITE_THREADS     =  8; //        | tot /2

constexpr int G_P2_P1F_READ_THREADS     =  4; //        | tot /4
constexpr int G_P2_P1F_READ_SIZE        = 2 * 1024 * 1024;
constexpr int G_P2_MARK_THREADS         = 24; // tot    | tot
constexpr int G_P2_REMAP_THREADS        = 24; // tot    | tot
constexpr int G_P2_P2_WRITE_THREADS     =  8; //        | tot /2

constexpr int G_P3S1_P1T1F_READ_THREADS =  2; //        | tot /4
constexpr int G_P3S1_P1T1F_READ_SIZE    = 2 * 1024 * 1024;
constexpr int G_P3S1_P2T7F_READ_THREADS =  2; //        | tot /4
constexpr int G_P3S1_P2T7F_READ_SIZE    = 2 * 1024 * 1024;
constexpr int G_P3S1_P3S1_WRITE_THREADS =  8; //        | tot /2
constexpr int G_P3S1_P2_SORT_THREADS    = 24; // tot    | tot /2 except t1f where tot
constexpr int G_P3S1_P2_READ_THREADS    = 10; //        | sort/2
constexpr int G_P3S1_P3S2_SORT_THREADS  = 24; // tot    | tot /2 except t7f where tot
constexpr int G_P3S1_P3S2_READ_THREADS  = 10; //        | sort/2
constexpr int G_P3S1_FILTER_THREADS     =  6; // tot /4 | tot /4
constexpr int G_P3S1_MERGE_THREADS      =  6; // tot /4 | tot /4

constexpr int G_P3S2_P3S1_SORT_THREADS  = 24; // tot    | tot
constexpr int G_P3S2_P3S1_READ_THREADS  =  8; //        | sort/2
constexpr int G_P3S2_P3S2_WRITE_THREADS =  8; // tot /2 | tot /2
constexpr int G_P3S2_F_PARK_THREADS     = 12; // tot /2 | tot /2

constexpr int G_P4_P3S2_SORT_THREADS    = 24; // tot    | tot
constexpr int G_P4_P3S2_READ_THREADS    =  8; //        | sort/2
constexpr int G_P4_F_PARK_THREADS       = 12; // tot /2 | tot /2
constexpr int G_P4_F_P7_THREADS         = 12; // tot /2 | tot /2

#endif

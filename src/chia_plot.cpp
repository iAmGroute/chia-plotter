/*
 * chia_plot.cpp
 *
 *  Created on: Jun 5, 2021
 *      Author: mad
 */

#include <chia/phase1.hpp>
#include <chia/phase2.hpp>
#include <chia/phase3.hpp>
#include <chia/phase4.hpp>
#include <chia/util.hpp>

#include <bls.hpp>
#include <sodium.h>
#include <cxxopts.hpp>
#include <libbech32.h>

#include <string>

#include <unistd.h>
#include <sys/resource.h>

std::vector<uint8_t> bech32_address_decode(const std::string& addr) {
    const auto res = bech32::decode(addr);
    if(res.encoding != bech32::Bech32m) {
        throw std::logic_error("invalid contract address (!Bech32m): " + addr);
    }
    if(res.hrp != "xch" && res.hrp != "txch") {
        throw std::logic_error("invalid contract address (" + res.hrp + " != xch or txch): " + addr);
    }
    if(res.dp.size() != 52) {
        throw std::logic_error("invalid contract address (size != 52): " + addr);
    }
    Bits bits;
    for(int i = 0; i < 51; ++i) {
        bits.AppendValue(res.dp[i], 5);
    }
    bits.AppendValue(res.dp[51] >> 4, 1);
    if(bits.GetSize() != 32 * 8) {
        throw std::logic_error("invalid contract address (bits != 256): " + addr);
    }
    std::vector<uint8_t> hash(32);
    bits.ToBytes(hash.data());
    return hash;
}

bls::G1Element calc_plot_key(bool have_puzzle, const bls::G1Element local_key, const bls::G1Element farmer_key) {
    if (have_puzzle) {
        vector<uint8_t> bytes = (local_key + farmer_key).Serialize();
        {
            const auto more_bytes = local_key.Serialize();
            bytes.insert(bytes.end(), more_bytes.begin(), more_bytes.end());
        }
        {
            const auto more_bytes = farmer_key.Serialize();
            bytes.insert(bytes.end(), more_bytes.begin(), more_bytes.end());
        }
        std::vector<uint8_t> hash(32);
        bls::Util::Hash256(hash.data(), bytes.data(), bytes.size());

        const auto taproot_sk = MPL.KeyGen(hash);
        return local_key + farmer_key + taproot_sk.GetG1Element();
    }
    else {
        return local_key + farmer_key;
    }
}

inline phase4::output_t create_plot(
    const std::string&     tree_dir,
    const vector<uint8_t>& pool_key_bytes,
    const vector<uint8_t>& puzzle_hash_bytes,
    const vector<uint8_t>& farmer_key_bytes
) {
    const auto total_begin = get_wall_time_micros();
    const bool have_puzzle = !puzzle_hash_bytes.empty();

    std::cout << "Settings file: " << SETTINGS_FILE << std::endl;
    std::cout << "Process ID: " << getpid() << std::endl;
    std::cout << "Number of Threads: " << G_TOTAL_THREADS << std::endl;
    std::cout << "Number of Buckets: 2^" << G_LOG_NUM_BUCKETS << " (" << (1 << G_LOG_NUM_BUCKETS) << ")" << std::endl;

    bls::G1Element pool_key;
    bls::G1Element farmer_key;
    try {
        if (not have_puzzle) pool_key = bls::G1Element::FromByteVector(pool_key_bytes);
    } catch(std::exception& ex) {
        std::cout << "Invalid pool_key: " << bls::Util::HexStr(pool_key_bytes) << std::endl;
        throw;
    }
    try {
        farmer_key = bls::G1Element::FromByteVector(farmer_key_bytes);
    } catch(std::exception& ex) {
        std::cout << "Invalid farmer_key: " << bls::Util::HexStr(farmer_key_bytes) << std::endl;
        throw;
    }
    if (have_puzzle) std::cout << "Pool Puzzle Hash:  " << bls::Util::HexStr(puzzle_hash_bytes)      << std::endl;
    else             std::cout << "Pool Public Key:   " << bls::Util::HexStr(pool_key.Serialize())   << std::endl;
    std::cout                  << "Farmer Public Key: " << bls::Util::HexStr(farmer_key.Serialize()) << std::endl;

    vector<uint8_t> seed(32);
    randombytes_buf(seed.data(), seed.size());

    bls::AugSchemeMPL MPL;
    const bls::PrivateKey master_sk = MPL.KeyGen(seed);

    bls::PrivateKey local_sk = master_sk;
    for (uint32_t i : {12381, 8444, 3, 0}) {
        local_sk = MPL.DeriveChildSk(local_sk, i);
    }
    const bls::G1Element local_key = local_sk.GetG1Element();
    const bls::G1Element plot_key  = calc_plot_key(have_puzzle, local_key, farmer_key);

    phase1::input_t params;
    {
        vector<uint8_t> bytes = have_puzzle ? puzzle_hash_bytes : pool_key.Serialize();
        {
            const auto plot_bytes = plot_key.Serialize();
            bytes.insert(bytes.end(), plot_bytes.begin(), plot_bytes.end());
        }
        bls::Util::Hash256(params.id.data(), bytes.data(), bytes.size());
    }
    const std::string plot_name = "plot-k32-" + get_date_string_ex("%Y-%m-%d-%H-%M")
            + "-" + bls::Util::HexStr(params.id.data(), params.id.size());

    std::cout << "Working Directory: " << tree_dir << std::endl;
    std::cout << "Plot Name: " << plot_name << std::endl;

    // memo = bytes(pool_public_key) || puzzle_hash_bytes + bytes(farmer_public_key) + bytes(local_master_sk)
    if (have_puzzle) {
        params.memo.insert(params.memo.end(), puzzle_hash_bytes.begin(), puzzle_hash_bytes.end());
    } else {
        params.memo.insert(params.memo.end(), pool_key_bytes.begin(), pool_key_bytes.end());
    }
    params.memo.insert(params.memo.end(), farmer_key_bytes.begin(), farmer_key_bytes.end());
    {
        const auto bytes = master_sk.Serialize();
        params.memo.insert(params.memo.end(), bytes.begin(), bytes.end());
    }
    params.plot_name = plot_name;

    phase1::output_t out_1; phase1::compute(params, out_1, plot_name, tree_dir);
    phase2::output_t out_2; phase2::compute(out_1,  out_2, plot_name, tree_dir);
    phase3::output_t out_3; phase3::compute(out_2,  out_3, plot_name, tree_dir);
    phase4::output_t out_4; phase4::compute(out_3,  out_4, plot_name, tree_dir);

    const auto time_secs = (get_wall_time_micros() - total_begin) / 1e6;
    std::cout << "Total plot creation time was "
            << time_secs << " sec (" << time_secs / 60. << " min)" << std::endl;
    return out_4;
}


int main(int argc, char** argv)
{

    cxxopts::Options options("chia_plot",
        "Multi-threaded pipelined Chia k32 plotter"
        #ifdef GIT_COMMIT_HASH
            " - " GIT_COMMIT_HASH
        #endif
        "\n\n"
        "For <pool_key> and <farmer_key> see output of `chia keys show`.\n"
    );

    std::string tree_dir;
    std::string pool_key_str;
    std::string contract_addr_str;
    std::string farmer_key_str;

    options.add_options() \
        ("t, tree_dir",      "Work tree directory, created by special script", cxxopts::value<std::string>(tree_dir)) \
        ("p, pool_key",      "Pool Public Key (48 bytes)",                     cxxopts::value<std::string>(pool_key_str)) \
        ("c, contract_addr", "NFT Contract Address (62 chars)",                cxxopts::value<std::string>(contract_addr_str)) \
        ("f, farmer_key",    "Farmer Public Key (48 bytes)",                   cxxopts::value<std::string>(farmer_key_str)) \
        ("help",             "Print help") \
    ;

    if (argc <= 1) {
        std::cout << options.help({""}) << std::endl;
        return 0;
    }
    const auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help({""}) << std::endl;
        return 0;
    }
    if (tree_dir.empty()) {
        std::cout << "tree_dir needs to be specified via -t path/" << std::endl;
        return -2;
    }
    if (not (pool_key_str.empty() xor contract_addr_str.empty())) {
        std::cout << "--- EITHER:" << std::endl;
        std::cout << "Pool Public Key (48 bytes) needs to be specified via -p <hex>, see `chia keys show`." << std::endl;
        std::cout << "--- OR:" << std::endl;
        std::cout << "NFT Contract Address (62 chars) needs to be specified via -c, see `chia plotnft show`." << std::endl;
        return -2;
    }
    if (farmer_key_str.empty()) {
        std::cout << "Farmer Public Key (48 bytes) needs to be specified via -f <hex>, see `chia keys show`." << std::endl;
        return -2;
    }

    const bool have_puzzle = !contract_addr_str.empty();

    const auto pool_key    = have_puzzle ? (std::vector<uint8_t>){} : hex_to_bytes(pool_key_str);
    const auto puzzle_hash = have_puzzle ? bech32_address_decode(contract_addr_str) : (std::vector<uint8_t>){};
    const auto farmer_key  = hex_to_bytes(farmer_key_str);

    if (tree_dir.find_last_of("/") != tree_dir.size() - 1) {
        std::cout << "Invalid tree_dir: " << tree_dir << " (needs trailing '/')" << std::endl;
        return -2;
    }
    if (not have_puzzle and (pool_key.size() != bls::G1Element::SIZE)) {
        std::cout << "Invalid pool_key: " << bls::Util::HexStr(pool_key) << ", '" << pool_key_str
            << "' (needs to be " << bls::G1Element::SIZE << " bytes, see `chia keys show`)" << std::endl;
        return -2;
    }
    if (have_puzzle and (puzzle_hash.size() != 32)) {
        std::cout << "Invalid puzzle_hash: " << bls::Util::HexStr(puzzle_hash)
            << ", from contract address '" << contract_addr_str
            << "' (see `chia plotnft show`)" << std::endl;
        return -2;
    }
    if (farmer_key.size() != bls::G1Element::SIZE) {
        std::cout << "Invalid farmer_key: " << bls::Util::HexStr(farmer_key) << ", '" << farmer_key_str
            << "' (needs to be " << bls::G1Element::SIZE << " bytes, see `chia keys show`)" << std::endl;
        return -2;
    }

    {
        struct rlimit the_limit = {
            .rlim_cur = 5000,
            .rlim_max = 5000
        };
        if (setrlimit(RLIMIT_NOFILE, &the_limit)) {
            std::cout << "Warning: setrlimit() failed!" << std::endl;
        }
    }

    std::cout << "Multi-threaded pipelined Chia k32 plotter"
    #ifdef GIT_COMMIT_HASH
        " - " GIT_COMMIT_HASH
    #endif
    << std::endl;

    create_plot(tree_dir, pool_key, puzzle_hash, farmer_key);

    return 0;
}

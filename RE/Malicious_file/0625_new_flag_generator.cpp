#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

int main() {
    std::string flag = "FLAG{Sh@hab_D3bug_M3m0ry_P4tch_1s_C00l}";

    std::string static_key = "PATCH";

    std::string dynamic_key = "THIS_IS_MASTER_KEY_TO_DECRYPT";

    std::vector<unsigned char> temp_data1;
    for (size_t i = 0; i < flag.length(); ++i) {
        temp_data1.push_back(flag[i] ^ static_key[i % static_key.length()]);
    }

    std::reverse(temp_data1.begin(), temp_data1.end());

    std::vector<unsigned char> final_payload;
    for (size_t i = 0; i < temp_data1.size(); ++i) {
        final_payload.push_back(temp_data1[i] ^ dynamic_key[i % dynamic_key.length()]);
    }

    std::cout << "unsigned char encrypted_flag_payload[] = {" << std::endl;
    for (size_t i = 0; i < final_payload.size(); ++i) {
        if (i % 12 == 0) std::cout << "    ";
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)final_payload[i];
        if (i < final_payload.size() - 1) std::cout << ", ";
        if ((i + 1) % 12 == 0) std::cout << std::endl;
    }
    std::cout << std::endl << "};" << std::endl;
    std::cout << "const size_t payload_size = sizeof(encrypted_flag_payload);" << std::endl;

    return 0;
}
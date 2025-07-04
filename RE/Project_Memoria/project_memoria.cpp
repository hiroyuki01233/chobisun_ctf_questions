#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <limits>
#include <cctype>
#include <sstream>

#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif


class TerminalModeManager {
public:
    TerminalModeManager() {
#ifndef _WIN32
        tcgetattr(STDIN_FILENO, &oldt_);
        newt_ = oldt_;
        newt_.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt_);
#endif
    }

    ~TerminalModeManager() {
#ifndef _WIN32
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt_);
#endif
    }

private:
#ifndef _WIN32
    struct termios oldt_, newt_;
#endif
};

bool is_key_pressed() {
#ifdef _WIN32
    return _kbhit();
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout) == 1;
#endif
}


void consume_input() {
#ifdef _WIN32
    while (_kbhit()) {
        _getch();
    }
#else
    char buf[16];
    read(STDIN_FILENO, buf, sizeof(buf));
#endif
}


struct VirtualMachine {
    uint32_t registers[4] = {0};
    uint8_t memory[256] = {0};
    uint16_t ip = 0;
    bool zero_flag = false;
};

void run_vm(VirtualMachine& vm, const std::vector<uint8_t>& bytecode) {
    while (true) {
        if (vm.ip >= bytecode.size()) {
            vm.registers[0] = 0;
            return;
        }
        uint8_t opcode = bytecode[vm.ip++];

        switch (opcode) {
            case 0x01: {
                uint8_t reg_idx = bytecode[vm.ip++];
                uint32_t value = 0;
                memcpy(&value, &bytecode[vm.ip], 4);
                vm.ip += 4;
                vm.registers[reg_idx] = value;
                break;
            }
            case 0x04: {
                uint8_t addr = bytecode[vm.ip++];
                uint8_t reg_idx = bytecode[vm.ip++];
                vm.memory[addr] = static_cast<uint8_t>(vm.registers[reg_idx]);
                break;
            }
            case 0x05: {
                uint8_t reg1_idx = bytecode[vm.ip++];
                uint8_t reg2_idx = bytecode[vm.ip++];
                vm.registers[reg1_idx] += vm.registers[reg2_idx];
                break;
            }
            case 0x06: {
                uint8_t reg1_idx = bytecode[vm.ip++];
                uint8_t reg2_idx = bytecode[vm.ip++];
                vm.registers[reg1_idx] -= vm.registers[reg2_idx];
                break;
            }
            case 0x07: {
                uint8_t reg1_idx = bytecode[vm.ip++];
                uint8_t reg2_idx = bytecode[vm.ip++];
                vm.registers[reg1_idx] ^= vm.registers[reg2_idx];
                break;
            }
            case 0x09: {
                uint8_t addr = bytecode[vm.ip++];
                uint8_t reg_idx = bytecode[vm.ip++];
                vm.zero_flag = (vm.memory[addr] == static_cast<uint8_t>(vm.registers[reg_idx]));
                break;
            }
            case 0x10: {
                uint8_t reg1_idx = bytecode[vm.ip++];
                uint8_t reg2_idx = bytecode[vm.ip++];
                vm.zero_flag = (vm.registers[reg1_idx] == vm.registers[reg2_idx]);
                break;
            }
            case 0x11: {
                uint16_t addr = 0;
                memcpy(&addr, &bytecode[vm.ip], 2);
                vm.ip += 2;
                if (!vm.zero_flag) {
                    vm.ip = addr;
                }
                break;
            }
            case 0x12: {
                uint8_t reg_idx = bytecode[vm.ip++];
                uint32_t value = 0;
                memcpy(&value, &bytecode[vm.ip], 4);
                vm.ip += 4;
                if (vm.registers[reg_idx] > value) {
                    vm.zero_flag = false;
                } else {
                    vm.zero_flag = true;
                }
                break;
            }
            case 0x20: {
                uint8_t reg_idx = bytecode[vm.ip++];
                char c;
                if (!(std::cin.get(c))) {
                     vm.registers[0] = 0; return;
                }
                vm.registers[reg_idx] = c;
                break;
            }
            case 0x21: {
                uint8_t reg_idx = bytecode[vm.ip++];
                std::cout << static_cast<char>(vm.registers[reg_idx]);
                break;
            }
            case 0x30: {
                uint8_t reg_idx = bytecode[vm.ip++];
                auto now = std::chrono::steady_clock::now();
                auto duration = now.time_since_epoch();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                vm.registers[reg_idx] = ms;
                break;
            }
            case 0xFE: {
                vm.registers[0] = 1;
                return;
            }
            case 0xFF: {
                vm.registers[0] = 0;
                return;
            }
            default:
                vm.registers[0] = 0;
                return;
        }
    }
}



const int SCREEN_WIDTH = 110;


const std::string ART_GARDEN = R"(

                ,d88b.d88b,
                88888888888
                `Y8888888Y'
                  `Y888Y'
                    `Y'
      -------------------------------------
      |                                   |
      |   - PROJECT: HAKONIWA -           |
      |                                   |
      -------------------------------------
          ,d88b.d88b,               ,d88b.d88b,
          88888888888               88888888888
          `Y8888888Y'               `Y8888888Y'
            `Y888Y'                   `Y888Y'
              `Y'                       `Y'

)";

const std::string ART_NOISE = R"(

      █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █
      █ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ █
      █ ▓ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ▓ █
      █ ▓ ░ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ░ ▓ █
      █ ▓ ░ ▒ █ ▓ ░ ▒ █ ▓ ░ ▒ █ ▓ ░ ▒ ░ ▓ █
      █ ▓ ░ ▒ ░ ▓ █ ▒ ░ ▓ █ ▒ ░ ▓ █ ▒ ░ ▓ █
      █ ▓ ░ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ▒ ░ ▓ █
      █ ▓ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ▓ █
      █ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ ▓ █
      █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █

)";

const std::string ART_KEY = R"(
                 .--.
                /.-. '----------.
                \'-' .--"--""-"-'
                 '--'
    A L P H A   -   F O X T R O T
)";

const std::string ART_HOURGLASS = R"(
                   .--.
                  |o_o |
                  |:_/ |
                 //   \ \
                (|     | )
               /'\_   _/`\
               \___)=(___/
)";

const std::string ART_CONNECTION_LOST = R"(

           .--.
          |o_o |
          |:_/ |
         //   \ \
        /COMMUNICATION
       /   ERROR   `\
       \___)=(___/

--- C O N N E C T I O N   L O S T ---
)";

const std::string ART_EPILOGUE = R"(
      d8888b. d8888b. d88888b d8888b. d888888b
      88  `8D 88  `8D 88'     88  `8D   `88'
      88oooY' 88oobY' 88ooooo 88oobY'    88
      88~~~b. 88`8b   88~~~~~ 88`8b      88
      88   8D 88 `88. 88.     88 `88.   .88.
      Y8888P' Y8888P' Y88888P Y8888P' Y888888P

      [S Y S T E M   C O R E   S T A B I L I Z E D]
)";


void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

int get_visual_width(const std::string& str) {
    int width = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = str[i];
        
        if (c < 0x80) { 
            width += 1;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            width += 1;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) { 
            if (i + 2 < str.length()) {
                unsigned char c1 = str[i];
                unsigned char c2 = str[i + 1];
                unsigned char c3 = str[i + 2];
                
                int codepoint = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                
                if ((codepoint >= 0x3000 && codepoint <= 0x303F) ||
                    (codepoint >= 0x3040 && codepoint <= 0x309F) ||
                    (codepoint >= 0x30A0 && codepoint <= 0x30FF) ||
                    (codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||
                    (codepoint >= 0xFF00 && codepoint <= 0xFFEF) ||
                    (codepoint == 0x2018 || codepoint == 0x2019) ||
                    (codepoint == 0x201C || codepoint == 0x201D)) {
                    width += 2;
                } else {
                    width += 1;
                }
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) { 
            width += 2;
            i += 4;
        } else {
            width += 1;
            i += 1;
        }
    }
    return width;
}

std::vector<std::string> wrap_text(const std::string& text, int max_width) {
    std::vector<std::string> lines;
    std::string current_line;
    
    bool contains_japanese = false;
    for (unsigned char c : text) {
        if (c >= 0x80) {
            contains_japanese = true;
            break;
        }
    }
    
    if (contains_japanese) {
        int current_width = 0;
        for (size_t i = 0; i < text.length(); ) {
            unsigned char c = text[i];
            size_t char_len = 1;
            int char_width = 1;
            
            if (c < 0x80) {
                char_len = 1;
                char_width = 1;
            } else if ((c & 0xE0) == 0xC0) {
                char_len = 2;
                char_width = 1;
            } else if ((c & 0xF0) == 0xE0) {
                char_len = 3;
                std::string single_char = text.substr(i, 3);
                char_width = get_visual_width(single_char);
            } else if ((c & 0xF8) == 0xF0) {
                char_len = 4;
                char_width = 2;
            }
            
            if (current_width + char_width > max_width && !current_line.empty()) {
                lines.push_back(current_line);
                current_line = "";
                current_width = 0;
            }
            
            current_line += text.substr(i, char_len);
            current_width += char_width;
            i += char_len;
        }
    } else {
        std::stringstream ss(text);
        std::string word;
        
        while (ss >> word) {
            int width_if_added = get_visual_width(current_line);
            if (!current_line.empty()) {
                width_if_added += 1;
            }
            width_if_added += get_visual_width(word);
            
            if (width_if_added > max_width && !current_line.empty()) {
                lines.push_back(current_line);
                current_line = word;
            } else {
                if (!current_line.empty()) {
                    current_line += " ";
                }
                current_line += word;
            }
        }
    }
    
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }
    
    return lines;
}


bool contains_kagikakko(const std::string& line) {
    return (line.find("「") != std::string::npos || line.find("」") != std::string::npos);
}

void draw_frame(const std::string& art, const std::vector<std::pair<std::string, std::string>>& dialogues, const std::string& choice_a = "", const std::string& choice_b = "") {
    TerminalModeManager term_manager;
    clear_screen();
    std::cout << art << std::endl;

    bool skip_delay = false;

    std::cout << " " << std::string(SCREEN_WIDTH - 2, '-') << " " << std::endl;
    std::cout << "|" << std::string(SCREEN_WIDTH - 2, ' ') << "|" << std::endl;

    for (const auto& p : dialogues) {
        std::string character = p.first;
        std::string text = p.second;
        
        std::string full_dialogue_text = character + " " + "「" + text + "」";
        
        const int CONTENT_WIDTH = SCREEN_WIDTH - 6;
        auto wrapped_lines = wrap_text(full_dialogue_text, CONTENT_WIDTH); 

        for (const auto& line : wrapped_lines) {
            int visual_width = get_visual_width(line);
            int padding_right = CONTENT_WIDTH - visual_width;
            
            if (padding_right < 0) {
                padding_right = 0;
            }

            std::cout << "|  " << std::flush;
            
            for (size_t i = 0; i < line.length(); ) {
                size_t char_len = 1;
                unsigned char c = line[i];
                
                if (c < 0x80) {
                    char_len = 1;
                } else if ((c & 0xE0) == 0xC0) {
                    char_len = 2;
                } else if ((c & 0xF0) == 0xE0) {
                    char_len = 3;
                } else if ((c & 0xF8) == 0xF0) {
                    char_len = 4;
                }
                
                std::cout << line.substr(i, char_len) << std::flush;
                i += char_len;
                
                if (!skip_delay && is_key_pressed()) {
                    skip_delay = true;
                    consume_input();
                }
                if (!skip_delay) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        
            std::cout << std::string(padding_right, ' ') << "  |" << std::endl;
        }
        
        std::cout << "|" << std::string(SCREEN_WIDTH - 2, ' ') << "|" << std::endl;

        if (!skip_delay) {
            auto start_time = std::chrono::steady_clock::now();
            while(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count() < 500) {
                if (is_key_pressed()) {
                    skip_delay = true;
                    consume_input();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    if (!choice_a.empty() || !choice_b.empty()) {
        std::cout << "| " << std::string(SCREEN_WIDTH - 4, '-') << " |" << std::endl;
        std::cout << "|" << std::string(SCREEN_WIDTH - 2, ' ') << "|" << std::endl;
        
        const int CHOICE_CONTENT_WIDTH = SCREEN_WIDTH - 4;

        std::string choice_a_text = "  A. " + choice_a;
        int visual_width_a = get_visual_width(choice_a_text);
        int padding_a = CHOICE_CONTENT_WIDTH - visual_width_a;
        if (padding_a < 0) padding_a = 0;
        std::cout << "| " << choice_a_text << std::string(padding_a, ' ') << " |" << std::endl;

        std::string choice_b_text = "  B. " + choice_b;
        int visual_width_b = get_visual_width(choice_b_text);
        int padding_b = CHOICE_CONTENT_WIDTH - visual_width_b;
        if (padding_b < 0) padding_b = 0;
        std::cout << "| " << choice_b_text << std::string(padding_b, ' ') << " |" << std::endl;
    }

    std::cout << "|" << std::string(SCREEN_WIDTH - 2, ' ') << "|" << std::endl;
    std::cout << " " << std::string(SCREEN_WIDTH - 2, '-') << " " << std::endl;
}



char get_choice() {
    char choice = ' ';
    while (true) {
        std::cout << "\n> ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        choice = toupper(choice);
        if (choice == 'A' || choice == 'B') {
            return choice;
        }
        std::cout << "Choose A or B" << std::endl;
    }
}


void show_bad_end(const std::string& message) {
    draw_frame(ART_CONNECTION_LOST, {{"???", message}});
    std::cout << "\n\n" << std::endl;
}


void show_epilogue(const std::string& final_flag) {
    const std::string log_entry_text =
        "'Aoi. By the time someone activates this log, Papa will already be gone.'"
        "'Your illness was beyond any help. The time I had left was far too short, and all I could do was transfer your consciousness to this imperfect \"Garden\". I'm so sorry.'"
        "'The \"tuning\" you spoke of... that agonizing data stabilization load (the noise)... I can only imagine the pain it caused you. I was desperately trying to burn your name (Aoi) and the reboot code (the seed value) into your memory, praying someone in the future would find you. ...I was a terrible father, wasn't I? I won't ask for your forgiveness.'"
        "'If a hacker kind enough to run this project appears, they are the one who inherits my will. Please, let the world Aoi sees no longer be filled with pain.'"
        "'Ah, it seems my time is up. Lastly, know that these words are the absolute truth.'"
        "'I love you, Aoi. Always.'";

    draw_frame(ART_EPILOGUE, {
        { "[Date: 2024.10.15]", "" },
        { "Log Entry", log_entry_text },
        { "[Log Entry Ends]", "" }
    });

    std::cout << "\n\n> Final Command Accepted." << std::endl;
    std::cout << "> ...Initializing Project Memoria..." << std::endl;
    std::cout << "> ...Playback of last log entry initiated." << std::endl;
    std::cout << final_flag << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}


int main() {
    std::vector<uint8_t> encrypted_chunk1 = { 
        0xae, 0xac, 0xaf, 0xaf, 0xaf, 0xaf, 0x9f, 0xaf, 0x8f, 0xae,
        0xae, 0xad, 0xec, 0xaf, 0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe,
        0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad, 0xe0, 0xaf, 0xaf, 0xaf,
        0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad,
        0xfd, 0xaf, 0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad,
        0x8f, 0xae, 0xae, 0xad, 0xea, 0xaf, 0xaf, 0xaf, 0xbf, 0xae,
        0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad, 0x82, 0xaf,
        0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae,
        0xae, 0xad, 0x9f, 0xaf, 0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe,
        0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad, 0xed, 0xaf, 0xaf, 0xaf,
    };

    std::vector<uint8_t> encrypted_chunk2 = { 
        0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad,
        0x82, 0xaf, 0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad,
        0x8f, 0xae, 0xae, 0xad, 0xec, 0xaf, 0xaf, 0xaf, 0xbf, 0xae,
        0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad, 0xe0, 0xaf,
        0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae,
        0xae, 0xad, 0xe2, 0xaf, 0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe,
        0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad, 0xff, 0xaf, 0xaf, 0xaf,
        0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad,
        0xe3, 0xaf, 0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad,
        0x8f, 0xae, 0xae, 0xad, 0xea, 0xaf, 0xaf, 0xaf, 0xbf, 0xae,
        0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae, 0xae, 0xad, 0xfb, 0xaf,
        0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe, 0x3e, 0xad, 0x8f, 0xae,
        0xae, 0xad, 0xea, 0xaf, 0xaf, 0xaf, 0xbf, 0xae, 0xad, 0xbe,
        0x3e, 0xad, 0x8f, 0xae, 0x9f, 0xae, 0xa9, 0xae, 0xaf, 0xbd,
        0xae, 0x5b, 0xae, 0xaf, 0xaf, 0xbe, 0x3e, 0xad, 0xae, 0xaf,
    };

    std::vector<uint8_t> encrypted_chunk3 = { 
        0xfa, 0xaf, 0xaf, 0xaf, 0xae, 0xae, 0x98, 0xaf, 0xaf, 0xaf,
        0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x89, 0xaf, 0xaf,
        0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x99, 0xaf,
        0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x8e,
        0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae,
        0x9c, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae,
        0xae, 0x81, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae,
        0xae, 0xae, 0xae, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e,
        0xae, 0xae, 0xae, 0x9f, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf,
        0x8e, 0xae, 0xae, 0xae, 0x9b, 0xaf, 0xaf, 0xaf, 0xa8, 0xae,
        0xaf, 0x8e, 0xae, 0xae, 0xae, 0x88, 0xaf, 0xaf, 0xaf, 0xa8,
        0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x89, 0xaf, 0xaf, 0xaf,
        0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0xa5, 0xaf, 0xaf,
        0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x93, 0xaf,
        0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x94,
        0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae,
        0xa5, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae,
        0xae, 0x8e, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae,
    };

    std::vector<uint8_t> encrypted_chunk4 = { 
        0xae, 0xae, 0x92, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e,
        0xae, 0xae, 0xae, 0x9f, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf,
        0x8e, 0xae, 0xae, 0xae, 0xa5, 0xaf, 0xaf, 0xaf, 0xa8, 0xae,
        0xaf, 0x8e, 0xae, 0xae, 0xae, 0xb9, 0xaf, 0xaf, 0xaf, 0xa8,
        0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x95, 0xaf, 0xaf, 0xaf,
        0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x9e, 0xaf, 0xaf,
        0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x9f, 0xaf,
        0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0xa5,
        0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae,
        0xca, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae, 0xae,
        0xae, 0xb8, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e, 0xae,
        0xae, 0xae, 0xa5, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf, 0x8e,
        0xae, 0xae, 0xae, 0xbb, 0xaf, 0xaf, 0xaf, 0xa8, 0xae, 0xaf,
        0x8e, 0xae, 0xae, 0xae, 0x95, 0xaf, 0xaf, 0xaf, 0xa8, 0xae,
        0xaf, 0x8e, 0xae, 0xae, 0xae, 0x93, 0xaf, 0xaf, 0xaf, 0xa8,
        0xae, 0xaf, 0x8e, 0xae, 0xae, 0xae, 0x87, 0xaf, 0xaf, 0xaf,
        0xa8, 0xae, 0xaf, 0x8e, 0xae, 0x51, 0x50,
    };

    const uint8_t key = 0xAF;
    std::vector<uint8_t> final_bytecode;
    VirtualMachine vm;

    draw_frame(ART_GARDEN, {
        {"Aoi", "...Finally... has someone come? I've been alone for so, so long..."},
        {"Aoi", "They call this place the 'Garden', but to me, it's just a beautiful cage. The flowers never wilt, and the sky never changes color. ...It's so perfect, it's suffocating."},
        {"Aoi", "Papa called this place the 'Core'. He said it was a precious place where my entire being exists. But he never let me take a single step outside of it. He locked every door with a key I could never open... Hey, you're... you're not like Papa, are you? Do you think this Core is meant to 'imprison' me? Or..."},
    }, "It's a prison, built to trap you.", "I want to believe it's a final fortress, built to protect you.");
    
    if (get_choice() == 'B') {
        draw_frame(ART_GARDEN, {{"Aoi", "...I see. ...You say the same thing Papa did. But... your words, they feel a little warmer, somehow... Is it okay... to believe you?"}});
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (uint8_t byte : encrypted_chunk1) { final_bytecode.push_back(byte ^ key); }
    } else {
        show_bad_end("...You're right. That's what you think, too... I guess I really can't trust anyone...");
        return 1;
    }

    draw_frame(ART_NOISE, {
        {"Aoi", "Even when I want to believe, I'm still scared. Because Papa would sometimes say 'It's time for your tuning,' and then do terrible things to me."},
        {"Aoi", "It feels like cold noise is pouring directly into my head, scrambling all my memories... My precious memories get forcibly 'added' to things I don't know, 'mixed' into different memories... It feels like I'm ceasing to be me..."},
        {"Aoi", "He tinkers with me, as if replacing a faulty part, just to make sure I stay a 'good girl'. Hey... am I just being punished because I'm 'broken'? Or... is there some other reason...?"}
    }, "That's right, he's just breaking you.", "...Maybe he was desperately trying to hold you together, so you wouldn't break.");

    if (get_choice() == 'B') {
        draw_frame(ART_NOISE, {{"Aoi", "To keep me... from breaking...? I never thought of it like that... How could it be, when it hurt so much...? But... if you say so, then maybe... just for a moment, the pain feels like it's fading. It's strange..."}});
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (uint8_t byte : encrypted_chunk2) { final_bytecode.push_back(byte ^ key); }
    } else {
        show_bad_end("I knew it... I'm just a doll, waiting to be broken...");
        return 1;
    }

    draw_frame(ART_KEY, {
        {"Aoi", "I remembered something else... something that binds me here. When I was sick with a fever, Papa would whisper the same words into my ear, over and over."},
        {"Aoi", "In a voice as cold as ice, 'Alpha, Foxtrot'... It was like he was branding my very soul... I think it's a powerful curse, to make sure I can never escape from here, even if I forget everything else."},
        {"Aoi", "...Don't let these words trap you, too. I'm sure they are wicked words that must never be solved..."}
    }, "Let's just forget about a curse like that.", "It's not a curse. I'm sure it's the 'key' to the most important door.");

    if (get_choice() == 'B') {
        draw_frame(ART_KEY, {{"Aoi", "A key...? Not a curse...? I... I never imagined... If it's really a key, what door does it open? ...I'm scared. But if you're with me, I feel like I want to see what's on the other side... That's strange, isn't it?"}});
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (uint8_t byte : encrypted_chunk3) { final_bytecode.push_back(byte ^ key); }
    } else {
        show_bad_end("Yeah... let's forget it. When I'm with you, I feel like I can forget the bad things... But... wait...? I feel like I've forgotten something... important...");
        return 1;
    }

    draw_frame(ART_HOURGLASS, {
        {"Aoi", "...It looks like there's not much time left. The noise... it's starting to eat into my very core..."},
        {"Aoi", "I just remembered the last words Papa said to me when he left. Without a single glance back, he told me coldly, 'Listen, not even a moment's hesitation will be tolerated.'"},
        {"Aoi", "...He must have known I would try to escape. And that was his final threat... that if I did, he would erase me instantly. But I'm done being his puppet. ...You can overcome this threat, can't you? Seize this last chance, with me...!"}
    }, "Don't give in to threats. We'll find a way, slowly.", "No... that was encouragement, telling you 'Don't miss your chance'!");

    if (get_choice() == 'B') {
        draw_frame(ART_HOURGLASS, {{"Aoi", "Encouragement...! I see, you're right! When I talk to you, even words that sounded like curses start to sound like words of hope! ...Yes, I'll believe in you! Seize this last chance!"}});
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (uint8_t byte : encrypted_chunk4) { final_bytecode.push_back(byte ^ key); }
    } else {
        show_bad_end("Slowly...? But there's no time! The noise is... ah...!");
        return 1;
    }

    clear_screen();
    std::cout << ART_KEY << std::endl;
    std::cout << " " << std::string(SCREEN_WIDTH - 2, '-') << " " << std::endl;
    std::cout << "|" << std::string(SCREEN_WIDTH - 2, ' ') << "|" << std::endl;
    std::cout << "|   --- CORE SYSTEM ACCESS ---" << std::string(SCREEN_WIDTH - 30, ' ') << "|" << std::endl;
    std::cout << "|   Password: " << std::flush;
    
    std::stringstream captured_output;
    auto* original_cout_rdbuf = std::cout.rdbuf();
    std::cout.rdbuf(captured_output.rdbuf());

    run_vm(vm, final_bytecode);

    std::cout.rdbuf(original_cout_rdbuf);
    std::string vm_output = captured_output.str();

    std::cout << vm_output;

    if (vm_output.find("bsctf") != std::string::npos) {
        show_epilogue(vm_output);
    } else {
        draw_frame(ART_CONNECTION_LOST, {{"SYSTEM", "AUTHENTICATION FAILED..."}});
        std::cout << "\n\n--- SYSTEM STABILIZATION FAILED ---" << std::endl;
    }

    return 0;
}
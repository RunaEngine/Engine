#include "utils/system/file.h"
#include <SDL3/SDL.h>

namespace runa::runtime {
    std::vector<uint8_t> load_file(const std::string &filepath) {
        std::vector<uint8_t> buffer;
        if (filepath.empty()) {
            SDL_Log("Error checking file existence: Invalid filepath (null or empty)");
            return buffer;
        }

        SDL_IOStream *file = SDL_IOFromFile(filepath.c_str(), "rb");
        if (!file) {
            SDL_Log("Error loading file to string: Unable to open file '%s': %s", filepath.c_str(), SDL_GetError());
            return buffer;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to end of file '%s': %s", filepath.c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return buffer;
        }

        Sint64 filesize = SDL_TellIO(file);
        if (filesize < 0) {
            SDL_Log("Error loading file to string: Unable to determine size of file '%s': %s", filepath.c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return buffer;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath.c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return buffer;
        }

        //char *buffer = SDL_malloc(filesize + 1);
        buffer.resize(filesize);

        size_t bytes_read = SDL_ReadIO(file, buffer.data(), filesize);
        SDL_CloseIO(file);

        if (bytes_read != filesize) {
            SDL_Log("Error loading file to string: Read only %zu bytes of %lld from file '%s': %s",
                    bytes_read, filesize, filepath.c_str(), SDL_GetError());
            buffer.clear(); buffer.shrink_to_fit();
            return buffer;
        }

        return buffer;
    }

    std::string load_text_file(const std::string &filepath) {
        std::string text;
        if (filepath.empty()) {
            SDL_Log("Error checking file existence: Invalid filepath (null or empty)");
            return text;
        }

        SDL_IOStream *file = SDL_IOFromFile(filepath.c_str(), "rb");

        if (!file) {
            SDL_Log("Error loading file to string: Unable to open file '%s': %s", filepath.c_str(), SDL_GetError());
            return text;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to end of file '%s': %s", filepath.c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return text;
        }

        Sint64 filesize = SDL_TellIO(file);
        if (filesize < 0) {
            SDL_Log("Error loading file to string: Unable to determine size of file '%s': %s", filepath.c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return text;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath.c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return text;
        }

        std::vector<uint8_t> buffer_vec;
        buffer_vec.resize(filesize);

        size_t bytes_read = SDL_ReadIO(file, buffer_vec.data(), filesize);
        SDL_CloseIO(file);

        if (bytes_read != filesize) {
            SDL_Log("Error loading file to string: Read only %zu bytes of %lld from file '%s': %s",
                    bytes_read, filesize, filepath.c_str(), SDL_GetError());
            buffer_vec.clear(); buffer_vec.shrink_to_fit();
            return text;
        }
        text.reserve(bytes_read);
        text.assign(buffer_vec.begin(), buffer_vec.end());
        return text;
    }

    bool file_exist(const std::string &filepath) {
        if (filepath.empty()) {
            SDL_Log("Error checking file existence: Invalid filepath (null or empty)");
            return false;
        }

        SDL_IOStream *file = SDL_IOFromFile(filepath.c_str(), "rb");

        if (file) {
            SDL_CloseIO(file);
            return true;
        }

        // No need to log here as file not existing is often an expected case
        return false;
    }
}

extern "C" {
    #include "../log.h"
    #include "7z.h"
}

#include <bit7z/bit7z.hpp>

#include <windows.h>


int sevenzip_available() {
    return GetFileAttributesA("7z.dll") != 0xFFFFFFFF;
}


struct SEVENZIP_RESULT {
    std::vector<bit7z::byte_t> data;
};


SEVENZIP_RESULT *sevenzip_compress(const unsigned char *input, int inputlen) {
    SEVENZIP_RESULT *result = new SEVENZIP_RESULT;
    try {
        bit7z::Bit7zLibrary lib { "7z.dll" };
        bit7z::BitArchiveWriter writer { lib, bit7z::BitFormat::SevenZip };

        writer.setCompressionLevel(bit7z::BitCompressionLevel::Fastest);

        std::vector<bit7z::byte_t> file(input, input + inputlen);
        writer.addFile(file, { });

        writer.compressTo(result->data);

    } catch (const bit7z::BitException &ex) {
        logging_printf("bit7z error: %s", ex.what());

        delete result;
        return nullptr;
    }
    return result;
}

SEVENZIP_RESULT *sevenzip_decompress(const unsigned char *input, int inputlen) {
    SEVENZIP_RESULT *result = new SEVENZIP_RESULT;
    try {
        bit7z::Bit7zLibrary lib { "7z.dll" };

        std::vector<bit7z::byte_t> file(input, input + inputlen);
        bit7z::BitArchiveReader reader { lib, file, bit7z::BitFormat::SevenZip };

        reader.extractTo(result->data);

    } catch (const bit7z::BitException &ex) {
        logging_printf("bit7z error: %s", ex.what());

        delete result;
        return nullptr;
    }
    return result;
}

int sevenzip_res_length(SEVENZIP_RESULT *res) {
    return res->data.size();
}

int sevenzip_res_read(SEVENZIP_RESULT *res, unsigned char *output) {
    std::copy(res->data.begin(), res->data.end(), output);
    return 1;
}

void sevenzip_res_free(SEVENZIP_RESULT *res) {
    delete res;
}

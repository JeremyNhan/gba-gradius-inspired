#include "ss_save.h"

#include "bn_type_traits.h"
#include "bn_sram.h"

namespace ss::save
{

namespace
{
    // SRAM layout (bn::sram, 32 KB battery-backed SRAM; Butano embeds the SRAM_V113 tag so
    // emulators and flash carts detect the save type).
    struct save_data
    {
        char magic[4];
        int version;
        int hiscore;
        int checksum;
    };

    constexpr int save_version = 1;

    [[nodiscard]] int checksum_of(const save_data& data)
    {
        return (data.hiscore ^ 0x5A5A1234) + data.version * 7919;
    }
}

int load_hiscore()
{
    save_data data;
    bn::sram::read(data);

    bool valid = data.magic[0] == 'S' && data.magic[1] == 'S' && data.magic[2] == 'H' && data.magic[3] == 'S' &&
            data.version == save_version && data.checksum == checksum_of(data) &&
            data.hiscore >= 0 && data.hiscore <= 99999990;
    return valid ? data.hiscore : default_hiscore;
}

void store_hiscore(int hiscore)
{
    save_data data;
    data.magic[0] = 'S';
    data.magic[1] = 'S';
    data.magic[2] = 'H';
    data.magic[3] = 'S';
    data.version = save_version;
    data.hiscore = hiscore;
    data.checksum = checksum_of(data);
    bn::sram::write(data);
}

}

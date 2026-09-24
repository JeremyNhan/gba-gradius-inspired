#ifndef SS_SAVE_H
#define SS_SAVE_H

namespace ss::save
{
    /// Reads the high score from cartridge SRAM; returns the default if SRAM is blank or corrupt.
    [[nodiscard]] int load_hiscore();

    /// Writes the high score to SRAM (with magic + checksum).
    void store_hiscore(int hiscore);

    constexpr int default_hiscore = 20000;
}

#endif

#ifndef _SCANIVALVE_MPS_CMD_H_
#define _SCANIVALVE_MPS_CMD_H_

namespace mps
{

enum class Commands
{
    VESRION = 0, // -> "Aiolos (c) MPS Emulator 2024 Ver 0.1\n >"
    REBOOT,      // -> ""
    RESTART,     // -> ""
    STATUS,      // -> one of "{READY, SCAN, CAL, VAL, CALZ, CALM, EVALVE}\n >"
    STOP,        //- also <ESC> -> "\n >"
    VALVESTATE,  // -> one of "{CAL, PX, Transistion}\n >"
    TRIG,        //- also responds to <TAB> -> one frame of data (TCP?) "\n >"
    TREAD,       // TREAD [<channel>] -> "Temperature on sneor 1 is ###\nTemepearture on ... \n>"
    SCAN,        // -> returns scan data based on formatting followed by "\n>"
    SAVE,        // SAVE [<configuration>] -> "\n>"
    LIST,

    _COUNT,

};

enum class SaveCmd
{
    IP = 0,
    ID,
    S,
    PTP,
    FC,
    C,
    T,
    M,
    FTP,
    O
};

enum class ListCmd
{
    S = 0,
    C,
    O,
    ID,
    IP,
    M,
    PTP,
};

static std::string COMMANDS[(size_t)Commands::_COUNT];
} // namespace mps

#endif // _SCANIVALVE_MPS_CMD_H_


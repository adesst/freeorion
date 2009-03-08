#include "MultiplayerCommon.h"

#include "OptionsDB.h"
#include "../UI/StringTable.h"
#include "../util/Directories.h"

#include <log4cpp/Priority.hh>

#if defined(_MSC_VER) && defined(int64_t)
#undef int64_t
#endif

#include <boost/filesystem/operations.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <fstream>
#include <iostream>


namespace fs = boost::filesystem;

namespace {
    // command-line options
    void AddOptions(OptionsDB& db)
    {
        db.Add<std::string>("settings-dir", "OPTIONS_DB_SETTINGS_DIR", (GetGlobalDir() / "default").native_directory_string());
        db.Add<std::string>("log-level", "OPTIONS_DB_LOG_LEVEL", "DEBUG");
        db.Add<std::string>("stringtable-filename", "OPTIONS_DB_STRINGTABLE_FILENAME", "eng_stringtable.txt");
    }
    bool temp_bool = RegisterOptions(&AddOptions);

    std::string SettingsDir()
    {
        std::string retval = GetOptionsDB().Get<std::string>("settings-dir");
        if (retval.empty() || retval[retval.size()-1] != '/')
            retval += '/';
        return retval;
    }
    const StringTable& GetStringTable()
    {
        static std::auto_ptr<StringTable> string_table(
            new StringTable(SettingsDir() + GetOptionsDB().Get<std::string>("stringtable-filename")));
        return *string_table;
    }

    std::string ClrToString(GG::Clr clr) {
        unsigned int r = static_cast<int>(clr.r);
        unsigned int g = static_cast<int>(clr.g);
        unsigned int b = static_cast<int>(clr.b);
        unsigned int a = static_cast<int>(clr.a);
        std::string retval = "(" + boost::lexical_cast<std::string>(r) + ", " +
                                   boost::lexical_cast<std::string>(g) + ", " +
                                   boost::lexical_cast<std::string>(b) + ", " +
                                   boost::lexical_cast<std::string>(a) + ")";
        return retval;
    }
}

/////////////////////////////////////////////////////
// Free Function(s)
/////////////////////////////////////////////////////
const std::vector<GG::Clr>& EmpireColors()
{
    static std::vector<GG::Clr> colors;
    if (colors.empty()) {
        XMLDoc doc;
        std::string settings_dir = GetOptionsDB().Get<std::string>("settings-dir");
        if (!settings_dir.empty() && settings_dir[settings_dir.size() - 1] != '/')
            settings_dir += '/';
        std::ifstream ifs((settings_dir + "empire_colors.xml").c_str());
        doc.ReadDoc(ifs);
        ifs.close();
        for (int i = 0; i < doc.root_node.NumChildren(); ++i) {
            colors.push_back(XMLToClr(doc.root_node.Child(i)));
        }
    }
    return colors;
}

XMLElement ClrToXML(const GG::Clr& clr)
{
    XMLElement retval("GG::Clr");
    retval.AppendChild(XMLElement("red", boost::lexical_cast<std::string>(static_cast<int>(clr.r))));
    retval.AppendChild(XMLElement("green", boost::lexical_cast<std::string>(static_cast<int>(clr.g))));
    retval.AppendChild(XMLElement("blue", boost::lexical_cast<std::string>(static_cast<int>(clr.b))));
    retval.AppendChild(XMLElement("alpha", boost::lexical_cast<std::string>(static_cast<int>(clr.a))));
    return retval;
}

GG::Clr XMLToClr(const XMLElement& clr)
{
    GG::Clr retval = GG::CLR_BLACK;
    if (clr.ContainsAttribute("hex")) {
        // get colour components as a single string representing three pairs of hex digits
        // from 00 to FF and an optional fourth hex digit pair for alpha
        const std::string& hex_colour = clr.Attribute("hex");
        std::istringstream iss(hex_colour);
        unsigned long rgba = 0;
        if (!(iss >> std::hex >> rgba).fail()) {
            if (hex_colour.size() == 6) {
                retval.r = (rgba >> 16) & 0xFF;
                retval.g = (rgba >> 8)  & 0xFF;
                retval.b = rgba         & 0xFF;
                retval.a = 255;
            } else {
                retval.r = (rgba >> 24) & 0xFF;
                retval.g = (rgba >> 16) & 0xFF;
                retval.b = (rgba >> 8)  & 0xFF;
                retval.a = rgba         & 0xFF;
            }
            //std::cout << "hex colour: " << hex_colour << " int: " << rgba << " RGBA: " << ClrToString(retval) << std::endl;
        } else {
            std::cerr << "XMLToClr could not interpret hex colour string \"" << hex_colour << "\"" << std::endl;
        }
    } else {
        // get colours listed as RGBA components ranging 0 to 255 as integers
        if (clr.ContainsChild("red"))
            retval.r = boost::lexical_cast<int>(clr.Child("red").Text());
        if (clr.ContainsChild("green"))
            retval.g = boost::lexical_cast<int>(clr.Child("green").Text());
        if (clr.ContainsChild("blue"))
            retval.b = boost::lexical_cast<int>(clr.Child("blue").Text());
        if (clr.ContainsChild("alpha"))
            retval.a = boost::lexical_cast<int>(clr.Child("alpha").Text());
        //std::cout << "non hex colour RGBA: " << ClrToString(retval) << std::endl;
    }
    return retval;
}

int PriorityValue(const std::string& name)
{
    static std::map<std::string, int> priority_map;
    static bool init = false;
    if (!init) {
        using namespace log4cpp;
        priority_map["FATAL"] = Priority::FATAL;
        priority_map["EMERG"] = Priority::EMERG;
        priority_map["ALERT"] = Priority::ALERT;
        priority_map["CRIT"] = Priority::CRIT;
        priority_map["ERROR"] = Priority::ERROR;
        priority_map["WARN"] = Priority::WARN;
        priority_map["NOTICE"] = Priority::NOTICE;
        priority_map["INFO"] = Priority::INFO;
        priority_map["DEBUG"] = Priority::DEBUG;
        priority_map["NOTSET"] = Priority::NOTSET;
    }
    return priority_map[name];
}

const std::string& UserString(const std::string& str)
{
    return GetStringTable().String(str);
}

boost::format FlexibleFormat(const std::string &string_to_format) {
    boost::format retval(string_to_format);
    retval.exceptions(boost::io::all_error_bits ^ (boost::io::too_many_args_bit | boost::io::too_few_args_bit));
    return retval;
}

std::string RomanNumber(unsigned int n)
{
    static const char N[] = "IVXLCDM??";
    std::string retval;
    int e = 3;
    int mod = 1000;
    for (; 0 <= e; e--, mod /= 10) {
        unsigned int m = (n / mod) % 10;
        if (m % 5 == 4) {
            retval += N[e << 1];
            ++m;
            if (m == 10) {
                retval += N[(e << 1) + 2];
                continue;
            }
        }
        if (m >= 5) {
            retval += N[(e << 1) + 1];
            m -= 5;
        }
        while (m) {
            retval += N[e << 1];
            --m;
        }
    }
    return retval;
}

const std::string& Language() 
{
    return GetStringTable().Language();
}

const std::string& SinglePlayerName()
{
    static const std::string RETVAL("Happy_Player");
    return RETVAL;
}

#ifndef FREEORION_WIN32
void Sleep(int ms)
{
    boost::xtime t;
    boost::xtime_get(&t, boost::TIME_UTC);
    int ns_sum = t.nsec + ms * 1000000;
    const int NANOSECONDS_PER_SECOND = 1000000000;
    int delta_secs = ns_sum / NANOSECONDS_PER_SECOND;
    int nanosecs = ns_sum % NANOSECONDS_PER_SECOND;
    t.sec += delta_secs;
    t.nsec = nanosecs;
    boost::thread::sleep(t);
}
#endif


/////////////////////////////////////////////////////
// GalaxySetupData
/////////////////////////////////////////////////////
GalaxySetupData::GalaxySetupData():
    m_size(100),
    m_shape(SPIRAL_2),
    m_age(AGE_MATURE),
    m_starlane_freq(LANES_SEVERAL),
    m_planet_density(PD_AVERAGE),
    m_specials_freq(SPECIALS_UNCOMMON)
{}


/////////////////////////////////////////////////////
// SinglePlayerSetupData
/////////////////////////////////////////////////////
SinglePlayerSetupData::SinglePlayerSetupData():
    m_new_game(true),
    m_AIs(0)
{}


/////////////////////////////////////////////////////
// SaveGameEmpireData
/////////////////////////////////////////////////////
SaveGameEmpireData::SaveGameEmpireData():
    m_id(-1)
{}


/////////////////////////////////////////////////////
// PlayerSetupData
/////////////////////////////////////////////////////
PlayerSetupData::PlayerSetupData() :
    m_player_id(-1),
    m_empire_name("Humans"),
    m_empire_color(GG::CLR_GRAY),
    m_save_game_empire_id(-1)
{}


/////////////////////////////////////////////////////
// MultiplayerLobbyData
/////////////////////////////////////////////////////
// static(s)
const std::string MultiplayerLobbyData::MP_SAVE_FILE_EXTENSION = ".mps";

MultiplayerLobbyData::MultiplayerLobbyData() :
    m_new_game(true),
    m_save_file_index(-1),
    m_empire_colors(EmpireColors())
{}

MultiplayerLobbyData::MultiplayerLobbyData(bool build_save_game_list) :
    m_new_game(true),
    m_save_file_index(-1),
    m_empire_colors(EmpireColors())
{
    if (build_save_game_list) {
        // build a list of save files
        fs::path save_dir(GetLocalDir() / "save");
        fs::directory_iterator end_it;
        for (fs::directory_iterator it(save_dir); it != end_it; ++it) {
            try {
                if (fs::exists(*it) && !fs::is_directory(*it) && it->filename()[0] != '.') {
                    std::string filename = it->filename();
                    // disallow filenames that begin with a dot, and filenames with spaces in them
                    if (filename.find('.') != 0 && filename.find(' ') == std::string::npos && 
                        filename.find(MP_SAVE_FILE_EXTENSION) == filename.size() - MP_SAVE_FILE_EXTENSION.size()) {
                        m_save_games.push_back(filename);
                    }
                }
            } catch (const fs::filesystem_error& e) {
                // ignore files for which permission is denied, and rethrow other exceptions
                if (e.code() != boost::system::posix_error::permission_denied)
                    throw;
            }
        }
    }
}


////////////////////////////////////////////////
// PlayerInfo
////////////////////////////////////////////////
PlayerInfo::PlayerInfo() :
    name(""),
    empire_id(-1),
    AI(false),
    host(false)
{}

PlayerInfo::PlayerInfo(const std::string& player_name_, int empire_id_, bool AI_, bool host_) :
    name(player_name_),
    empire_id(empire_id_),
    AI(AI_),
    host(host_)
{}


////////////////////////////////////////////////
// CombatData
////////////////////////////////////////////////
CombatData::CombatData() :
    m_combat_turn_number(1),
    m_system(0)
{}

CombatData::CombatData(System* system, const std::map<int, UniverseObject*>& combat_universe) :
    m_combat_turn_number(1),
    m_system(system),
    m_combat_universe(combat_universe)
{}

#include "FedataFile.h"

bool FedataFile::Load(const std::string &fedataPath, FedataFile &fedataFile, uint8_t nPriColours)
{
    LOG(INFO) << "Loading Fedata File located at " << fedataPath;
    std::ifstream fedata(fedataPath, std::ios::in | std::ios::binary);

    fedataFile.m_nPriColours = nPriColours;
    bool loadStatus = fedataFile._SerializeIn(fedata);
    fedata.close();

    return loadStatus;
}

void FedataFile::Save(const std::string &fedataPath, FedataFile &fedataFile)
{
    LOG(INFO) << "Saving Fedata File to " << fedataPath;
    std::ofstream fedata(fedataPath, std::ios::out | std::ios::binary);
    fedataFile._SerializeOut(fedata);
}

bool FedataFile::_SerializeIn(std::ifstream &ifstream)
{
    // TODO: Hugely incomplete. Old style parser shoehorned into new format, need all structs. No seekg. /AS
    // Go get the offset of car name
    uint32_t menuNameOffset = 0;
    ifstream.seekg(MENU_NAME_FILEPOS_OFFSET, std::ios::beg);
    SAFE_READ(ifstream, &menuNameOffset, sizeof(uint32_t));
    ifstream.seekg(menuNameOffset, std::ios::beg);

    char carMenuName[64];
    SAFE_READ(ifstream, &carMenuName, sizeof(char) * 64);
    menuName = carMenuName;

    // Jump to location of FILEPOS table for car colour names
    ifstream.seekg(COLOUR_TABLE_OFFSET, std::ios::beg);
    // Read that table in
    uint32_t colourNameOffsets[m_nPriColours];
    SAFE_READ(ifstream, &colourNameOffsets, m_nPriColours * sizeof(uint32_t));

    for (uint8_t colourIdx = 0; colourIdx < m_nPriColours; ++colourIdx)
    {
        ifstream.seekg(colourNameOffsets[colourIdx]);
        uint32_t colourNameLength = colourIdx < (m_nPriColours - 1) ? (colourNameOffsets[colourIdx + 1] - colourNameOffsets[colourIdx]) : 32;
        char colourName[colourNameLength];
        SAFE_READ(ifstream, &colourName, colourNameLength);
        primaryColourNames.push_back(std::string(colourName));
    }

    return true;
}

void FedataFile::_SerializeOut(std::ofstream &ofstream)
{
    ASSERT(false, "Fedata output serialization is not currently implemented");
}

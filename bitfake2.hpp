#ifndef BITFAKE2_HPP
#define BITFAKE2_HPP

// bitfake2.hpp
// A AIO bitfake2 "api" header file for projects.

#include <cstdio>
#include <filesystem>
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
namespace fs = std::filesystem;

namespace BitFake
{
    // defining some basic types and structures for the bitfake2 toolkit
    // goes before getting into the juicy stuff.

    static u_int32_t readSynchsafe(const unsigned char b[4]) {
        return (uint32_t(b[0]) << 21) | (uint32_t(b[1]) << 14) | (uint32_t(b[2]) << 7) | uint32_t(b[3]);
    }

    static std::string trimNulls(std::string str) {
        while (!str.empty() && (str.back() == '\0' || str.back() == ' ')) str.pop_back();
        return str;
    }


    class Codec
    {
        public:
        enum class AudioCodecType
        {
            MP3,
            FLAC,
            WAV,
            AAC,
            OGG,
            OPUS,
            ALAC,
            AIFF,
            WMA,
            PCM
        };

        struct AudioCodecMagicStrings
        {
            static constexpr std::string_view MP3 = "FFFB"; // MPEG-1 Layer III frame sync
            static constexpr std::string_view FLAC = "664C6143"; // "fLaC"
            static constexpr std::string_view WAV = "52494646"; // "RIFF"
            static constexpr std::string_view AAC = "FFF1"; // ADTS frame sync
            static constexpr std::string_view OGG = "4F676753"; // "OggS"
            static constexpr std::string_view OPUS = "4F707573"; // "OpusS"
            static constexpr std::string_view ALAC = "616C6163"; // "alac"
            static constexpr std::string_view AIFF = "464F524D"; // "FORM"
            static constexpr std::string_view WMA = "3026B2758E66CF11"; // ASF header
            static constexpr std::string_view PCM = "0001"; // PCM doesn't have a fixed magic number, this is just a placeholder
        };
    
    };



    // input: (filepath, expected codec)
    // output: true if the file's magic string matches the expected codec, false otherwise.
    static bool CheckMagic(const fs::path& filepath, const Codec::AudioCodecType& expectedCodec)
    {
        if (!fs::exists(filepath)) {
            fprintf(stderr, "File does not exist: %s\n", filepath.string().c_str());
            return false;
        }
        std::ifstream f(filepath, std::ios::binary);
        if (!f) return false;

        std::array<char, 4096> magic{};
        f.read(magic.data(), magic.size());
        const std::size_t bytesRead = static_cast<std::size_t>(f.gcount());
        std::string_view magicStr(magic.data(), bytesRead);

        std::string hexMagic;
        hexMagic.reserve(bytesRead * 2);
        static constexpr char hexDigits[] = "0123456789ABCDEF";
        for (std::size_t i = 0; i < bytesRead; ++i)
        {
            const unsigned char byte = static_cast<unsigned char>(magic[i]);
            hexMagic.push_back(hexDigits[(byte >> 4) & 0x0F]);
            hexMagic.push_back(hexDigits[byte & 0x0F]);
        }

        switch (expectedCodec)
        {
            case Codec::AudioCodecType::MP3:
            {
                if (magicStr.size() >= 3 && magicStr.substr(0, 3) == "ID3") return true;

                for (std::size_t i = 0; i + 1 < bytesRead; ++i)
                {
                    const unsigned char b0 = static_cast<unsigned char>(magic[i]);
                    const unsigned char b1 = static_cast<unsigned char>(magic[i + 1]);
                    if (b0 == 0xFF && (b1 & 0xE0) == 0xE0) return true;
                }

                return hexMagic.find(Codec::AudioCodecMagicStrings::MP3) != std::string::npos;
            }
            case Codec::AudioCodecType::FLAC:
                return hexMagic.find(Codec::AudioCodecMagicStrings::FLAC) != std::string::npos;
            case Codec::AudioCodecType::WAV:
                return hexMagic.find(Codec::AudioCodecMagicStrings::WAV) != std::string::npos;
            case Codec::AudioCodecType::AAC:
                return hexMagic.find(Codec::AudioCodecMagicStrings::AAC) != std::string::npos;
            case Codec::AudioCodecType::OGG:
                return hexMagic.find(Codec::AudioCodecMagicStrings::OGG) != std::string::npos;
            case Codec::AudioCodecType::OPUS:
                return hexMagic.find(Codec::AudioCodecMagicStrings::OPUS) != std::string::npos;
            case Codec::AudioCodecType::ALAC:
                return hexMagic.find(Codec::AudioCodecMagicStrings::ALAC) != std::string::npos;
            case Codec::AudioCodecType::AIFF:
                return hexMagic.find(Codec::AudioCodecMagicStrings::AIFF) != std::string::npos;
            case Codec::AudioCodecType::WMA:
                return hexMagic.find(Codec::AudioCodecMagicStrings::WMA) != std::string::npos;
            case Codec::AudioCodecType::PCM:
                return hexMagic.find(Codec::AudioCodecMagicStrings::PCM) != std::string::npos;
            default:
                return false;
        }
    }

    class Type
    {
        public:
        struct BasicMD
        {
            std::string Title;
            std::string Artist;
            std::string Album;
            std::string Year;
        };

        struct ExtendedMD
        {
            std::string Title, Artist, Album, Year, Genre;
            int TrackNo, DiscNo, TotalTracks, TotalDiscs;
            int Duration; // in seconds
            int Bitrate, Channels;
            BitFake::Codec::AudioCodecType AudioCodec;
        };

        struct ReplayGainMD
        {
            float TrackGain, AlbumGain;
            int TrackPeak, AlbumPeak;
        };
    };

    class GetMD
    {
        public:
        static Type::BasicMD GetBasicMD(const fs::path& filepath, const Codec::AudioCodecType& codec)
        {
            // placeholder implementation
            Type::BasicMD md{};
            md.Title = "Unknown Title";
            md.Artist = "Unknown Artist";
            md.Album = "Unknown Album";
            md.Year = "Unknown Year";
            if (!fs::exists(filepath))
            {
                fprintf(stderr, "File does not exist: %s\n", filepath.string().c_str());
                return md;
            }
            std::ifstream f(filepath, std::ios::binary);
            if (!f) return md;

            switch (codec)
            {
                case Codec::AudioCodecType::MP3:
                {
                // ID3v2 (start of file parsing)

                char hdr[10] = {0};
                if (f.gcount() == 10 && hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3') {
                    unsigned char sz[4] = {
                        (unsigned char)hdr[6],
                        (unsigned char)hdr[7],
                        (unsigned char)hdr[8],
                        (unsigned char)hdr[9]
                    };
                    uint32_t tagSize = readSynchsafe(sz);
                    std::vector<char> tagData(tagSize);
                    f.read(tagData.data(), tagSize);

                    size_t i = 0;
                    while (i + 10 <= tagSize) {
                        const char* fr = &tagData[i];
                        if (fr[0] == 0) break;

                        std::string frameID(fr, 4);
                        uint32_t fs = (uint8_t)fr[4] << 24 | (uint8_t)fr[5] << 16 | (uint8_t)fr[6] << 8 | (uint8_t)fr[7];
                        if (fs == 0 || i + 10 + fs > tagSize) break;

                        if ((frameID == "TIT2" || frameID == "TPE1" || frameID == "TALB" || frameID == "TDRC") && fs > 1) {
                            char enc = fr[10];
                            std::string content(fr + 11, fs - 1);
                            if (enc == 0) content = trimNulls(content);
                            else if (enc == 1) content = trimNulls(content); // UTF-16, needs proper decoding
                            
                            if (frameID == "TIT2") md.Title = content;
                            else if (frameID == "TPE1") md.Artist = content;
                            else if (frameID == "TALB") md.Album = content;
                            else if (frameID == "TDRC") md.Year = content;
                        }

                        i += 10 + fs;
                    }
                }
                // old ID3v1 (end of file parsing)

                f.clear();
                f.seekg(-128, std::ios::end);
                std::streamoff len = f.tellg();
                if (len >= 128) {
                    f.seekg(-128, std::ios::end);
                    char id3v1[128];
                    f.read(id3v1, 128);
                    if (id3v1[0] == 'T' && id3v1[1] == 'A' && id3v1[2] == 'G') {
                        if (md.Title == "Unknown Title") md.Title = trimNulls(std::string(id3v1 + 3, 30));
                        if (md.Artist == "Unknown Artist") md.Artist = trimNulls(std::string(id3v1 + 33, 30));
                        if (md.Album == "Unknown Album") md.Album = trimNulls(std::string(id3v1 + 63, 30));
                        if (md.Year == "Unknown Year") md.Year = trimNulls(std::string(id3v1 + 93, 4));
                    }
                }
                
                break;
                }
            
            default:
                break;
            }

            return md;
        }
        static Type::ExtendedMD GetExtendedMD(const fs::path& filepath, const Codec::AudioCodecType& codec)
        {
            Type::ExtendedMD md{};
            md.Title = "Unknown Title";
            md.Artist = "Unknown Artist";
            md.Album = "Unknown Album";
            md.Year = "Unknown Year";
            md.Genre = "Unknown Genre";
            md.TrackNo = 0;
            md.DiscNo = 0;
            md.TotalTracks = 0;
            md.TotalDiscs = 0;
            md.Duration = 0;
            md.Bitrate = 0;
            md.Channels = 0;
            md.AudioCodec = codec;

            if (!fs::exists(filepath)) {
                fprintf(stderr, "File does not exist: %s\n", filepath.string().c_str());
                return md;
            }
            std::ifstream f(filepath, std::ios::binary);
            if (!f) return md;

            switch (codec)
            {
                case Codec::AudioCodecType::MP3:
                {
                    // ID3v2 parsing
                    char hdr[10] = {0};
                    f.read(hdr, 10);
                    if (f.gcount() == 10 && hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3') {
                        unsigned char sz[4] = {
                            (unsigned char)hdr[6],
                            (unsigned char)hdr[7],
                            (unsigned char)hdr[8],
                            (unsigned char)hdr[9]
                        };
                        uint32_t tagSize = readSynchsafe(sz);
                        std::vector<char> tagData(tagSize);
                        f.read(tagData.data(), tagSize);

                        size_t i = 0;
                        while (i + 10 <= tagSize) {
                            const char* fr = &tagData[i];
                            if (fr[0] == 0) break;

                            std::string frameID(fr, 4);
                            uint32_t fs = (uint8_t)fr[4] << 24 | (uint8_t)fr[5] << 16 | (uint8_t)fr[6] << 8 | (uint8_t)fr[7];
                            if (fs == 0 || i + 10 + fs > tagSize) break;

                            if (fs > 1) {
                                char enc = fr[10];
                                std::string content(fr + 11, fs - 1);
                                if (enc == 0) content = trimNulls(content);
                                else if (enc == 1) content = trimNulls(content); // UTF-16

                                if (frameID == "TIT2") md.Title = content;
                                else if (frameID == "TPE1") md.Artist = content;
                                else if (frameID == "TALB") md.Album = content;
                                else if (frameID == "TDRC") md.Year = content;
                                else if (frameID == "TCON") md.Genre = content;
                                else if (frameID == "TRCK") {
                                    size_t slash = content.find('/');
                                    if (slash != std::string::npos) {
                                        md.TrackNo = std::stoi(content.substr(0, slash));
                                        md.TotalTracks = std::stoi(content.substr(slash + 1));
                                    } else {
                                        md.TrackNo = std::stoi(content);
                                    }
                                }
                                else if (frameID == "TPOS") {
                                    size_t slash = content.find('/');
                                    if (slash != std::string::npos) {
                                        md.DiscNo = std::stoi(content.substr(0, slash));
                                        md.TotalDiscs = std::stoi(content.substr(slash + 1));
                                    } else {
                                        md.DiscNo = std::stoi(content);
                                    }
                                }
                            }

                            i += 10 + fs;
                        }
                    }

                    // ID3v1 fallback
                    f.clear();
                    f.seekg(0, std::ios::end);
                    std::streamoff fileSize = f.tellg();
                    if (fileSize >= 128) {
                        f.seekg(-128, std::ios::end);
                        char id3v1[128];
                        f.read(id3v1, 128);
                        if (id3v1[0] == 'T' && id3v1[1] == 'A' && id3v1[2] == 'G') {
                            if (md.Title == "Unknown Title") md.Title = trimNulls(std::string(id3v1 + 3, 30));
                            if (md.Artist == "Unknown Artist") md.Artist = trimNulls(std::string(id3v1 + 33, 30));
                            if (md.Album == "Unknown Album") md.Album = trimNulls(std::string(id3v1 + 63, 30));
                            if (md.Year == "Unknown Year") md.Year = trimNulls(std::string(id3v1 + 93, 4));
                        }
                    }

                    f.clear();
                    f.seekg(0);
                    char byte;
                    int frameCount = 0;
                    int firstBitrate = 0;
                    
                    f.seekg(10);
                    if (f.peek() == 'I') {
                        unsigned char sz[4];
                        f.seekg(6);
                        f.read((char*)sz, 4);
                        uint32_t tagSize = readSynchsafe(sz);
                        f.seekg(10 + tagSize);
                    }

                    while (f.get(byte)) {
                        if ((unsigned char)byte == 0xFF) {
                            unsigned char nextByte;
                            f.get((char&)nextByte);
                            if ((nextByte & 0xE0) == 0xE0) {
                                unsigned char third;
                                f.get((char&)third);
                                int bitrateIdx = (third >> 4) & 0xF;
                                int sampleIdx = (third >> 2) & 0x3;

                                static const int br_table[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
                                int bitrate = br_table[bitrateIdx] * 1000;
                                
                                static const int sr_table[4] = {44100, 48000, 32100, 0};
                                int sampleRate = sr_table[sampleIdx];
                                
                                if (firstBitrate == 0) firstBitrate = bitrate;
                                if (sampleRate > 0) md.Channels = 2;

                                frameCount++;
                                if (frameCount > 100) break;
                            }
                        }
                    }

                    if (frameCount > 0 && firstBitrate > 0) {
                        md.Bitrate = firstBitrate / 1000; // convert to kbps
                        md.Duration = (fileSize * 8) / (firstBitrate);
                    }

                    break;
                }
            
                default:
                    break;
            }

            return md;
        }
    };
}

#endif // BITFAKE2_HPP
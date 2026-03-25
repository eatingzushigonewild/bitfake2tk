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
#include <cctype>
#include <limits>
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

    static uint32_t readBE32(const unsigned char b[4]) {
        return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
    }

    static int parseLeadingInt(const std::string& s) {
        int value = 0;
        bool found = false;
        for (char c : s) {
            if (c >= '0' && c <= '9') {
                found = true;
                value = value * 10 + (c - '0');
            } else if (found) {
                break;
            }
        }
        return found ? value : 0;
    }

    static std::string decodeID3TextFrame(const char* data, uint32_t frameSize) {
        if (frameSize <= 1) return "";
        const unsigned char encoding = static_cast<unsigned char>(data[0]);
        const char* payload = data + 1;
        const size_t payloadSize = static_cast<size_t>(frameSize - 1);

        if (encoding == 0 || encoding == 3) {
            return trimNulls(std::string(payload, payloadSize));
        }

        std::string out;
        out.reserve(payloadSize);
        for (size_t i = 0; i < payloadSize; ++i) {
            const unsigned char ch = static_cast<unsigned char>(payload[i]);
            if (ch != 0) out.push_back(static_cast<char>(ch));
        }
        return trimNulls(out);
    }

    struct MP3FrameInfo {
        int bitrate = 0;
        int sampleRate = 0;
        int channels = 0;
        int frameSize = 0;
        int samplesPerFrame = 0;
    };

    static bool parseMP3FrameHeader(uint32_t h, MP3FrameInfo& info) {
        if ((h & 0xFFE00000u) != 0xFFE00000u) return false;

        const int versionId = (h >> 19) & 0x3;
        const int layer = (h >> 17) & 0x3;
        const int bitrateIdx = (h >> 12) & 0xF;
        const int sampleIdx = (h >> 10) & 0x3;
        const int padding = (h >> 9) & 0x1;
        const int channelMode = (h >> 6) & 0x3;

        if (versionId == 1 || layer == 0 || bitrateIdx == 0 || bitrateIdx == 15 || sampleIdx == 3) return false;

        static const int brV1L1[16] = {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0};
        static const int brV1L2[16] = {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0};
        static const int brV1L3[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
        static const int brV2L1[16] = {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0};
        static const int brV2Lx[16] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};

        static const int srV1[3] = {44100, 48000, 32000};
        static const int srV2[3] = {22050, 24000, 16000};
        static const int srV25[3] = {11025, 12000, 8000};

        const bool mpeg1 = (versionId == 3);
        const bool mpeg25 = (versionId == 0);
        int sampleRate = mpeg1 ? srV1[sampleIdx] : (mpeg25 ? srV25[sampleIdx] : srV2[sampleIdx]);
        if (sampleRate <= 0) return false;

        int bitrateKbps = 0;
        if (mpeg1) {
            if (layer == 3) bitrateKbps = brV1L1[bitrateIdx];
            else if (layer == 2) bitrateKbps = brV1L2[bitrateIdx];
            else bitrateKbps = brV1L3[bitrateIdx];
        } else {
            if (layer == 3) bitrateKbps = brV2L1[bitrateIdx];
            else bitrateKbps = brV2Lx[bitrateIdx];
        }
        if (bitrateKbps <= 0) return false;

        const int bitrate = bitrateKbps * 1000;
        int frameSize = 0;
        int samplesPerFrame = 0;

        if (layer == 3) {
            frameSize = ((12 * bitrate) / sampleRate + padding) * 4;
            samplesPerFrame = 384;
        } else if (layer == 2) {
            frameSize = (144 * bitrate) / sampleRate + padding;
            samplesPerFrame = 1152;
        } else {
            if (mpeg1) {
                frameSize = (144 * bitrate) / sampleRate + padding;
                samplesPerFrame = 1152;
            } else {
                frameSize = (72 * bitrate) / sampleRate + padding;
                samplesPerFrame = 576;
            }
        }

        if (frameSize <= 4) return false;

        info.bitrate = bitrate;
        info.sampleRate = sampleRate;
        info.channels = (channelMode == 3) ? 1 : 2;
        info.frameSize = frameSize;
        info.samplesPerFrame = samplesPerFrame;
        return true;
    }

    static uint32_t readLE32(const uint8_t* p) {
        return uint32_t(p[0]) |
               (uint32_t(p[1]) << 8) |
               (uint32_t(p[2]) << 16) |
               (uint32_t(p[3]) << 24);
    }

    static uint64_t readLE64(const uint8_t* p) {
        return uint64_t(p[0]) |
               (uint64_t(p[1]) << 8) |
               (uint64_t(p[2]) << 16) |
               (uint64_t(p[3]) << 24) |
               (uint64_t(p[4]) << 32) |
               (uint64_t(p[5]) << 40) |
               (uint64_t(p[6]) << 48) |
               (uint64_t(p[7]) << 56);
    }

    struct OggCommentData {
        std::string Title = "Unknown Title";
        std::string Artist = "Unknown Artist";
        std::string Album = "Unknown Album";
        std::string Date = "Unknown Date";
        std::string Genre = "Unknown Genre";
        int TrackNo = 0;
        int DiscNo = 0;
        int TotalTracks = 0;
        int TotalDiscs = 0;
        int Channels = 0;
        int SampleRate = 0;
        uint64_t LastGranule = 0;
    };

    static void parseVorbisCommentFields(const std::vector<uint8_t>& packet,
                                         size_t startOffset,
                                         OggCommentData& out) {
        if (packet.size() < startOffset + 8) return;

        size_t off = startOffset;
        auto need = [&](size_t n) { return off + n <= packet.size(); };

        if (!need(4)) return;
        const uint32_t vendorLen = readLE32(packet.data() + off);
        off += 4;
        if (!need(vendorLen)) return;
        off += vendorLen;

        if (!need(4)) return;
        const uint32_t commentCount = readLE32(packet.data() + off);
        off += 4;

        for (uint32_t i = 0; i < commentCount; ++i) {
            if (!need(4)) break;
            const uint32_t cLen = readLE32(packet.data() + off);
            off += 4;
            if (!need(cLen)) break;

            std::string kv(reinterpret_cast<const char*>(packet.data() + off), cLen);
            off += cLen;

            const size_t eq = kv.find('=');
            if (eq == std::string::npos) continue;

            std::string key = kv.substr(0, eq);
            std::string val = kv.substr(eq + 1);
            std::transform(key.begin(), key.end(), key.begin(),
                           [](unsigned char c) { return char(std::toupper(c)); });

            if (key == "TITLE") out.Title = val;
            else if (key == "ARTIST") out.Artist = val;
            else if (key == "ALBUM") out.Album = val;
            else if (key == "DATE" || key == "YEAR") out.Date = val;
            else if (key == "GENRE") {
                if (out.Genre == "Unknown Genre" || out.Genre.empty()) out.Genre = val;
                else if (out.Genre.find(val) == std::string::npos) out.Genre += ";" + val;
            }
            else if (key == "TRACKNUMBER") {
                size_t slash = val.find('/');
                if (slash != std::string::npos) {
                    out.TrackNo = parseLeadingInt(val.substr(0, slash));
                    out.TotalTracks = parseLeadingInt(val.substr(slash + 1));
                } else {
                    out.TrackNo = parseLeadingInt(val);
                }
            }
            else if (key == "TOTALTRACKS" || key == "TRACKTOTAL") out.TotalTracks = parseLeadingInt(val);
            else if (key == "DISCNUMBER") out.DiscNo = parseLeadingInt(val);
            else if (key == "TOTALDISCS" || key == "DISCTOTAL") out.TotalDiscs = parseLeadingInt(val);
        }
    }

    static bool parseOggFamilyMetadata(const fs::path& filepath,
                                       bool expectOpus,
                                       OggCommentData& out) {
        std::ifstream f(filepath, std::ios::binary);
        if (!f) return false;

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (data.size() < 27) return false;

        bool seenIdHeader = false;
        bool seenCommentHeader = false;
        bool isOpusStream = false;
        std::vector<uint8_t> packet;

        size_t pos = 0;
        while (pos + 27 <= data.size()) {
            if (!(data[pos] == 'O' && data[pos + 1] == 'g' && data[pos + 2] == 'g' && data[pos + 3] == 'S')) {
                break;
            }

            const uint64_t granulePos = readLE64(data.data() + pos + 6);
            if (granulePos != std::numeric_limits<uint64_t>::max()) {
                out.LastGranule = granulePos;
            }

            const uint8_t pageSegments = data[pos + 26];
            const size_t segTableStart = pos + 27;
            if (segTableStart + pageSegments > data.size()) break;

            size_t pageDataLen = 0;
            for (uint8_t i = 0; i < pageSegments; ++i) pageDataLen += data[segTableStart + i];

            const size_t pageDataStart = segTableStart + pageSegments;
            if (pageDataStart + pageDataLen > data.size()) break;

            size_t dataCursor = pageDataStart;
            for (uint8_t i = 0; i < pageSegments; ++i) {
                const uint8_t segLen = data[segTableStart + i];
                packet.insert(packet.end(), data.begin() + dataCursor, data.begin() + dataCursor + segLen);
                dataCursor += segLen;

                if (segLen < 255) {
                    if (!seenIdHeader) {
                        if (packet.size() >= 8 && std::string(reinterpret_cast<const char*>(packet.data()), 8) == "OpusHead") {
                            isOpusStream = true;
                            seenIdHeader = true;
                            if (packet.size() > 9) out.Channels = static_cast<int>(packet[9]);
                            out.SampleRate = 48000;
                        } else if (packet.size() >= 30 && packet[0] == 0x01 &&
                                   std::string(reinterpret_cast<const char*>(packet.data() + 1), 6) == "vorbis") {
                            isOpusStream = false;
                            seenIdHeader = true;
                            out.Channels = static_cast<int>(packet[11]);
                            out.SampleRate = static_cast<int>(readLE32(packet.data() + 12));
                        }
                    } else if (!seenCommentHeader) {
                        if (isOpusStream && packet.size() >= 8 &&
                            std::string(reinterpret_cast<const char*>(packet.data()), 8) == "OpusTags") {
                            parseVorbisCommentFields(packet, 8, out);
                            seenCommentHeader = true;
                        } else if (!isOpusStream && packet.size() >= 7 && packet[0] == 0x03 &&
                                   std::string(reinterpret_cast<const char*>(packet.data() + 1), 6) == "vorbis") {
                            parseVorbisCommentFields(packet, 7, out);
                            seenCommentHeader = true;
                        }
                    }

                    packet.clear();
                }
            }

            pos = pageDataStart + pageDataLen;
        }

        if (!seenIdHeader) return false;
        if (expectOpus != isOpusStream) return false;
        return true;
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
            std::string Date;
        };

        struct ExtendedMD
        {
            std::string Title, Artist, Album, Date, Genre;
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
            Type::BasicMD md{};
            md.Title = "Unknown Title";
            md.Artist = "Unknown Artist";
            md.Album = "Unknown Album";
            md.Date = "Unknown Date";
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
                if (!CheckMagic(filepath, Codec::AudioCodecType::MP3)) {
                    fprintf(stderr, "File does not appear to be a valid MP3: %s\n", filepath.string().c_str());
                    return md;
                };
                char hdr[10] = {0};
                f.read(hdr, 10);
                if (f.gcount() == 10 && hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3') {
                    const uint8_t majorVersion = static_cast<uint8_t>(hdr[3]);
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
                        if (!std::isalnum(static_cast<unsigned char>(frameID[0]))) break;
                        const unsigned char frameSizeBytes[4] = {
                            static_cast<unsigned char>(fr[4]),
                            static_cast<unsigned char>(fr[5]),
                            static_cast<unsigned char>(fr[6]),
                            static_cast<unsigned char>(fr[7])
                        };
                        uint32_t fs = (majorVersion == 4) ? readSynchsafe(frameSizeBytes) : readBE32(frameSizeBytes);
                        if (fs == 0 || i + 10 + fs > tagSize) break;

                        if ((frameID == "TIT2" || frameID == "TPE1" || frameID == "TALB" || frameID == "TDRC") && fs > 1) {
                            std::string content = decodeID3TextFrame(fr + 10, fs);
                            
                            if (frameID == "TIT2") md.Title = content;
                            else if (frameID == "TPE1") md.Artist = content;
                            else if (frameID == "TALB") md.Album = content;
                            else if (frameID == "TDRC") md.Date = content;
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
                        if (md.Date == "Unknown Date") md.Date = trimNulls(std::string(id3v1 + 93, 4));
                    }
                }
                
                break;
                }
                case Codec::AudioCodecType::FLAC:
                {
                    if (!CheckMagic(filepath, Codec::AudioCodecType::FLAC)) {
                        fprintf(stderr, "File does not appear to be a valid FLAC: %s\n", filepath.string().c_str());
                        return md;
                    }

                    f.clear();
                    f.seekg(0, std::ios::beg);

                    char sig[4] = {0};
                    f.read(sig, 4);
                    if (f.gcount() != 4 || std::string(sig, 4) != "fLaC") {
                        fprintf(stderr, "Invalid FLAC signature: %s\n", filepath.string().c_str());
                        return md;
                    }

                    bool isLast = false;
                    while (!isLast && f) {
                        uint8_t header = 0;
                        f.read(reinterpret_cast<char*>(&header), 1);
                        if (!f) break;

                        isLast = (header & 0x80) != 0;
                        const uint8_t blockType = header & 0x7F;

                        uint8_t lenBytes[3] = {0, 0, 0};
                        f.read(reinterpret_cast<char*>(lenBytes), 3);
                        if (!f) break;

                        const uint32_t blockLen =
                            (uint32_t(lenBytes[0]) << 16) |
                            (uint32_t(lenBytes[1]) << 8) |
                            uint32_t(lenBytes[2]);

                        std::vector<uint8_t> block(blockLen);
                        f.read(reinterpret_cast<char*>(block.data()), blockLen);
                        if (!f) break;

                        if (blockType != 4) continue;

                        size_t off = 0;
                        auto need = [&](size_t n) { return off + n <= block.size(); };
                        auto readU32LE = [&](size_t p) -> uint32_t {
                            return uint32_t(block[p]) |
                                   (uint32_t(block[p + 1]) << 8) |
                                   (uint32_t(block[p + 2]) << 16) |
                                   (uint32_t(block[p + 3]) << 24);
                        };

                        if (!need(4)) break;
                        const uint32_t vendorLen = readU32LE(off);
                        off += 4;
                        if (!need(vendorLen)) break;
                        off += vendorLen;

                        if (!need(4)) break;
                        const uint32_t commentCount = readU32LE(off);
                        off += 4;

                        for (uint32_t i = 0; i < commentCount; ++i) {
                            if (!need(4)) break;
                            const uint32_t cLen = readU32LE(off);
                            off += 4;
                            if (!need(cLen)) break;

                            std::string kv(reinterpret_cast<char*>(block.data() + off), cLen);
                            off += cLen;

                            const size_t eq = kv.find('=');
                            if (eq == std::string::npos) continue;

                            std::string key = kv.substr(0, eq);
                            std::string val = kv.substr(eq + 1);
                            std::transform(key.begin(), key.end(), key.begin(),
                                           [](unsigned char c) { return char(std::toupper(c)); });

                            if (key == "TITLE") md.Title = val;
                            else if (key == "ARTIST") md.Artist = val;
                            else if (key == "ALBUM") md.Album = val;
                            else if (key == "DATE" || key == "YEAR") md.Date = val;
                        }
                    }

                    break;
                }
                case Codec::AudioCodecType::OGG:
                case Codec::AudioCodecType::OPUS:
                {
                    if (!CheckMagic(filepath, codec)) {
                        fprintf(stderr, "File does not appear to be a valid OGG/OPUS stream: %s\n", filepath.string().c_str());
                        return md;
                    }

                    OggCommentData ogg{};
                    const bool isOpus = (codec == Codec::AudioCodecType::OPUS);
                    if (!parseOggFamilyMetadata(filepath, isOpus, ogg)) {
                        fprintf(stderr, "Failed to parse OGG/OPUS metadata: %s\n", filepath.string().c_str());
                        return md;
                    }

                    md.Title = ogg.Title;
                    md.Artist = ogg.Artist;
                    md.Album = ogg.Album;
                    md.Date = ogg.Date;
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
            md.Date = "Unknown Date";
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
                    std::streamoff audioStart = 0;
                    f.read(hdr, 10);
                    if (f.gcount() == 10 && hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3') {
                        const uint8_t majorVersion = static_cast<uint8_t>(hdr[3]);
                        const uint8_t flags = static_cast<uint8_t>(hdr[5]);
                        unsigned char sz[4] = {
                            (unsigned char)hdr[6],
                            (unsigned char)hdr[7],
                            (unsigned char)hdr[8],
                            (unsigned char)hdr[9]
                        };
                        uint32_t tagSize = readSynchsafe(sz);
                        audioStart = 10 + tagSize + ((flags & 0x10) ? 10 : 0);
                        std::vector<char> tagData(tagSize);
                        f.read(tagData.data(), tagSize);

                        size_t i = 0;
                        while (i + 10 <= tagSize) {
                            const char* fr = &tagData[i];
                            if (fr[0] == 0) break;

                            std::string frameID(fr, 4);
                            if (!std::isalnum(static_cast<unsigned char>(frameID[0]))) break;
                            const unsigned char frameSizeBytes[4] = {
                                static_cast<unsigned char>(fr[4]),
                                static_cast<unsigned char>(fr[5]),
                                static_cast<unsigned char>(fr[6]),
                                static_cast<unsigned char>(fr[7])
                            };
                            uint32_t fs = (majorVersion == 4) ? readSynchsafe(frameSizeBytes) : readBE32(frameSizeBytes);
                            if (fs == 0 || i + 10 + fs > tagSize) break;

                            if (fs > 1) {
                                std::string content = decodeID3TextFrame(fr + 10, fs);

                                if (frameID == "TIT2") md.Title = content;
                                else if (frameID == "TPE1") md.Artist = content;
                                else if (frameID == "TALB") md.Album = content;
                                else if (frameID == "TDRC") md.Date = content;
                                else if (frameID == "TCON") md.Genre = content;
                                else if (frameID == "TRCK") {
                                    size_t slash = content.find('/');
                                    if (slash != std::string::npos) {
                                        md.TrackNo = parseLeadingInt(content.substr(0, slash));
                                        md.TotalTracks = parseLeadingInt(content.substr(slash + 1));
                                    } else {
                                        md.TrackNo = parseLeadingInt(content);
                                    }
                                }
                                else if (frameID == "TPOS") {
                                    size_t slash = content.find('/');
                                    if (slash != std::string::npos) {
                                        md.DiscNo = parseLeadingInt(content.substr(0, slash));
                                        md.TotalDiscs = parseLeadingInt(content.substr(slash + 1));
                                    } else {
                                        md.DiscNo = parseLeadingInt(content);
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
                            if (md.Date == "Unknown Date") md.Date = trimNulls(std::string(id3v1 + 93, 4));
                            if (md.TrackNo == 0 && static_cast<unsigned char>(id3v1[125]) == 0) {
                                md.TrackNo = static_cast<unsigned char>(id3v1[126]);
                            }
                        }
                    }

                    f.clear();
                    std::streamoff audioEnd = fileSize;
                    if (fileSize >= 128) {
                        f.seekg(-128, std::ios::end);
                        char id3v1Check[3] = {0};
                        f.read(id3v1Check, 3);
                        if (f.gcount() == 3 && id3v1Check[0] == 'T' && id3v1Check[1] == 'A' && id3v1Check[2] == 'G') {
                            audioEnd -= 128;
                        }
                    }

                    if (audioEnd < audioStart) audioEnd = audioStart;

                    f.clear();
                    f.seekg(audioStart, std::ios::beg);
                    if (f) {
                        int frameCount = 0;
                        double totalDurationSec = 0.0;
                        std::streamoff pos = audioStart;

                        while (pos + 4 <= audioEnd) {
                            unsigned char h[4] = {0};
                            f.seekg(pos, std::ios::beg);
                            f.read(reinterpret_cast<char*>(h), 4);
                            if (f.gcount() != 4) break;

                            const uint32_t header = readBE32(h);
                            MP3FrameInfo frameInfo;
                            if (!parseMP3FrameHeader(header, frameInfo)) {
                                pos += 1;
                                continue;
                            }
                            if (pos + frameInfo.frameSize > audioEnd) break;

                            md.Channels = frameInfo.channels;
                            totalDurationSec += static_cast<double>(frameInfo.samplesPerFrame) / static_cast<double>(frameInfo.sampleRate);
                            frameCount++;
                            pos += frameInfo.frameSize;
                        }

                        if (frameCount > 0 && totalDurationSec > 0.0) {
                            md.Duration = static_cast<int>(totalDurationSec);
                            md.Bitrate = static_cast<int>((static_cast<double>(fileSize) * 8.0) / totalDurationSec / 1000.0);
                        }
                    }

                    break;
                }
                case Codec::AudioCodecType::FLAC:
                {
                    if (!CheckMagic(filepath, Codec::AudioCodecType::FLAC)) {
                        fprintf(stderr, "File does not appear to be a valid FLAC: %s\n", filepath.string().c_str());
                        return md;
                    };

                    f.clear();
                    f.seekg(0, std::ios::beg);

                    char sig[4] = {0};
                    f.read(sig, 4);
                    if (f.gcount() != 4 || std::string(sig, 4) != "fLaC") {
                        fprintf(stderr, "Invalid FLAC signature: %s\n", filepath.string().c_str());
                        return md;
                    }
                    
                    auto parseInt = [](const std::string& str) -> int {
                        int val = 0;
                        bool any = false;
                        for (char c : str) {
                            if (c >= '0' && c <= '9') {any = true; val = val * 10 + (c - '0');}
                            else if (any) break;
                        }
                        return any ? val : 0;
                    };

                    bool isLast = false;
                    uint32_t sampleRate =0;
                    uint64_t totalSamples = 0;
                    while (!isLast && f) {
                        uint8_t header = 0;
                        f.read(reinterpret_cast<char*>(&header), 1);
                        if (!f) break;
                        isLast = (header & 0x80) != 0;
                        uint8_t blockType = header & 0x7F;
                        if (!f) break;
                        uint8_t lenBytes[3];
                        f.read(reinterpret_cast<char*>(lenBytes), 3);
                        if (!f) break;
                        uint32_t blockLen = 
                            (uint32_t(lenBytes[0]) << 16) |
                            (uint32_t(lenBytes[1]) << 8) |
                            uint32_t(lenBytes[2]);
                        std::vector<uint8_t> block(blockLen);
                        f.read(reinterpret_cast<char*>(block.data()), blockLen);
                        if (!f) break;

                        if (blockType == 0 && blockLen >= 34) {
                            uint64_t x = 0;
                            for (int i = 10; i < 18; ++i) x = (x << 8) | block[i];
                            sampleRate = static_cast<uint32_t>((x >> 44) & 0xFFFFF);
                            md.Channels = static_cast<int>(((x >> 41) & 0x7) + 1);
                            totalSamples = x & ((1ULL << 36) - 1);
                        } else if (blockType == 4) {
                            size_t off = 0;
                            auto need = [&](size_t n) {return off + n <= block.size();};
                            auto readU32LE = [&](size_t p) -> uint32_t {
                            return uint32_t(block[p]) |
                                   (uint32_t(block[p + 1]) << 8) |
                                   (uint32_t(block[p + 2]) << 16) |
                                   (uint32_t(block[p + 3]) << 24);
                        };
                        if (!need(4)) break;
                        uint32_t vendorLen = readU32LE(off); 
                        off += 4;
                        if (!need(vendorLen)) break;
                        off += vendorLen;
                        if (!need(4)) break;

                        uint32_t commentCount = readU32LE(off);
                        off += 4;

                        for (uint32_t i = 0; i < commentCount; ++i) {
                            if (!need(4)) break;
                            uint32_t cLen = readU32LE(off); off += 4;
                            if (!need(cLen)) break;

                            std::string kv(reinterpret_cast<char*>(&block[off]), cLen);
                            off += cLen;

                            size_t eq = kv.find('=');
                            if (eq == std::string::npos) continue;

                            std::string key = kv.substr(0, eq);
                            std::string val = kv.substr(eq + 1);
                            std::transform(key.begin(), key.end(), key.begin(),
                                           [](unsigned char c) { return char(std::toupper(c)); });

                            if (key == "TITLE") md.Title = val;
                            else if (key == "ARTIST") md.Artist = val;
                            else if (key == "ALBUM") md.Album = val;
                            else if (key == "DATE" || key == "YEAR") md.Date = val;
                            else if (key == "GENRE") {
                                if (md.Genre == "Unknown Genre" || md.Genre.empty()) md.Genre = val;
                                else if (md.Genre.find(val) == std::string::npos) md.Genre += ";" + val;
                            }
                            else if (key == "TRACKNUMBER") {
                                size_t slash = val.find('/');
                                if (slash != std::string::npos) {
                                    md.TrackNo = parseInt(val.substr(0, slash));
                                    md.TotalTracks = parseInt(val.substr(slash + 1));
                                } else {
                                    md.TrackNo = parseInt(val);
                                }
                            }
                            else if (key == "TOTALTRACKS" || key == "TRACKTOTAL") md.TotalTracks = parseInt(val);
                            else if (key == "DISCNUMBER") md.DiscNo = parseInt(val);
                            else if (key == "TOTALDISCS" || key == "DISCTOTAL") md.TotalDiscs = parseInt(val);
                            }
                        }
                    }

                    if (sampleRate > 0 && totalSamples > 0) {
                        md.Duration = static_cast<int>(totalSamples / sampleRate);
                    }
                    if (md.Duration > 0) {
                        const uint64_t FILESIZE = static_cast<uint64_t>(fs::file_size(filepath));
                        md.Bitrate = static_cast<int>((FILESIZE * 8) / md.Duration / 1000); // in kbps
                    }
                    break;
                }
                case Codec::AudioCodecType::OGG:
                case Codec::AudioCodecType::OPUS:
                {
                    if (!CheckMagic(filepath, codec)) {
                        fprintf(stderr, "File does not appear to be a valid OGG/OPUS stream: %s\n", filepath.string().c_str());
                        return md;
                    }

                    OggCommentData ogg{};
                    const bool isOpus = (codec == Codec::AudioCodecType::OPUS);
                    if (!parseOggFamilyMetadata(filepath, isOpus, ogg)) {
                        fprintf(stderr, "Failed to parse OGG/OPUS metadata: %s\n", filepath.string().c_str());
                        return md;
                    }

                    md.Title = ogg.Title;
                    md.Artist = ogg.Artist;
                    md.Album = ogg.Album;
                    md.Date = ogg.Date;
                    md.Genre = ogg.Genre;
                    md.TrackNo = ogg.TrackNo;
                    md.DiscNo = ogg.DiscNo;
                    md.TotalTracks = ogg.TotalTracks;
                    md.TotalDiscs = ogg.TotalDiscs;
                    md.Channels = ogg.Channels;

                    if (ogg.SampleRate > 0 && ogg.LastGranule > 0) {
                        md.Duration = static_cast<int>(ogg.LastGranule / static_cast<uint64_t>(ogg.SampleRate));
                    }
                    if (md.Duration > 0) {
                        const uint64_t fileSize = static_cast<uint64_t>(fs::file_size(filepath));
                        md.Bitrate = static_cast<int>((fileSize * 8ULL) / static_cast<uint64_t>(md.Duration) / 1000ULL);
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
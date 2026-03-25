// small test file for the bitfake2 library, to test the basic metadata extraction functionality.
// you do NOT need to use this at all. this is just testing my shit

#include "bitfake2.hpp"
#include <iostream>
#include <cstring>

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static BitFake::Codec::AudioCodecType detectCodec(const fs::path& filepath) {
    const std::string ext = toLower(filepath.extension().string());

    if (ext == ".flac") return BitFake::Codec::AudioCodecType::FLAC;
    if (ext == ".mp3") return BitFake::Codec::AudioCodecType::MP3;
    if (ext == ".wav") return BitFake::Codec::AudioCodecType::WAV;
    if (ext == ".aac") return BitFake::Codec::AudioCodecType::AAC;
    if (ext == ".ogg") return BitFake::Codec::AudioCodecType::OGG;
    if (ext == ".opus") return BitFake::Codec::AudioCodecType::OPUS;
    if (ext == ".aiff" || ext == ".aif") return BitFake::Codec::AudioCodecType::AIFF;
    if (ext == ".wma") return BitFake::Codec::AudioCodecType::WMA;

    if (BitFake::CheckMagic(filepath, BitFake::Codec::AudioCodecType::FLAC)) return BitFake::Codec::AudioCodecType::FLAC;
    if (BitFake::CheckMagic(filepath, BitFake::Codec::AudioCodecType::WAV)) return BitFake::Codec::AudioCodecType::WAV;
    if (BitFake::CheckMagic(filepath, BitFake::Codec::AudioCodecType::OGG)) return BitFake::Codec::AudioCodecType::OGG;
    if (BitFake::CheckMagic(filepath, BitFake::Codec::AudioCodecType::OPUS)) return BitFake::Codec::AudioCodecType::OPUS;
    if (BitFake::CheckMagic(filepath, BitFake::Codec::AudioCodecType::AAC)) return BitFake::Codec::AudioCodecType::AAC;
    if (BitFake::CheckMagic(filepath, BitFake::Codec::AudioCodecType::MP3)) return BitFake::Codec::AudioCodecType::MP3;

    return BitFake::Codec::AudioCodecType::MP3;
}

static const char* codecToString(BitFake::Codec::AudioCodecType codec) {
    using AudioCodecType = BitFake::Codec::AudioCodecType;
    switch (codec) {
        case AudioCodecType::MP3: return "MP3";
        case AudioCodecType::FLAC: return "FLAC";
        case AudioCodecType::WAV: return "WAV";
        case AudioCodecType::AAC: return "AAC";
        case AudioCodecType::OGG: return "OGG";
        case AudioCodecType::OPUS: return "OPUS";
        case AudioCodecType::ALAC: return "ALAC";
        case AudioCodecType::AIFF: return "AIFF";
        case AudioCodecType::WMA: return "WMA";
        case AudioCodecType::PCM: return "PCM";
        default: return "Unknown";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audiofile>" << std::endl;
        return 1;
    }

    for (int i = 0; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " <operation> <audiofile>" << std::endl;
            return 0;
        }

        if (std::strcmp(argv[i], "-bmd") == 0 || std::strcmp(argv[i], "--basicmd") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing file path after " << argv[i] << std::endl;
                return 1;
            }
            fs::path filepath = argv[++i];
            auto codec = detectCodec(filepath);
            auto md = BitFake::GetMD::GetBasicMD(filepath, codec);
            std::cout << "Title: " << md.Title << std::endl;
            std::cout << "Artist: " << md.Artist << std::endl;
            std::cout << "Album: " << md.Album << std::endl;
            std::cout << "Year/Date: " << md.Date << std::endl;
        }

        if (std::strcmp(argv[i], "-emd") == 0 || std::strcmp(argv[i], "--extendedmd") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing file path after " << argv[i] << std::endl;
                return 1;
            }
            fs::path filepath = argv[++i];
            auto codec = detectCodec(filepath);
            auto md = BitFake::GetMD::GetExtendedMD(filepath, codec);
            std::cout << "Title: " << md.Title << std::endl;
            std::cout << "Artist: " << md.Artist << std::endl;
            std::cout << "Album: " << md.Album << std::endl;
            std::cout << "Year/Date: " << md.Date << std::endl;
            std::cout << "Genre: " << md.Genre << std::endl;
            std::cout << "Track No: " << md.TrackNo << std::endl;
            std::cout << "Disc No: " << md.DiscNo << std::endl;
            std::cout << "Total Tracks: " << md.TotalTracks << std::endl;
            std::cout << "Total Discs: " << md.TotalDiscs << std::endl;
            std::cout << "Duration: " << md.Duration << " seconds" << std::endl;
            std::cout << "Bitrate: " << md.Bitrate << " kbps" << std::endl;
            std::cout << "Channels: " << md.Channels << std::endl;
        }

        if (std::strcmp(argv[i], "-cm") == 0 || std::strcmp(argv[i], "--checkmagic") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing file path after " << argv[i] << std::endl;
                return 1;
            }
            fs::path filepath = argv[++i];
            auto codec = detectCodec(filepath);
            std::cout << "Detected codec: " << codecToString(codec) << std::endl;

            // im tired
        }
    }

    return 0;
}
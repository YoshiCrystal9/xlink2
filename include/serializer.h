#pragma once

#include "system.h"

#include <cstring>
#include <map>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace banana {

// assumes little endian
class Serializer {
public:
    explicit Serializer(System* sys) : mSystem(sys) {}

    void write(std::span<const u8> data) {
        if (mOffset + data.size() > mData.size()) {
            mData.resize(mOffset + data.size());
        }

        std::memcpy(&mData[mOffset], data.data(), data.size());
        mOffset += data.size();
    }

    void writeAt(std::span<const u8> data, size_t offset) {
        if (offset + data.size() > mData.size()) {
            mData.resize(offset + data.size());
        }

        std::memcpy(&mData[offset], data.data(), data.size());
    }

    template <typename T, typename std::enable_if_t<std::is_trivially_copyable_v<T>>* = nullptr>
    void write(T v) {
        write({reinterpret_cast<const u8*>(&v), sizeof(T)});
    }

    template <typename T, typename std::enable_if_t<std::is_trivially_copyable_v<T>>* = nullptr>
    void writeAt(T v, size_t offset) {
        writeAt({reinterpret_cast<const u8*>(&v), sizeof(T)}, offset);
    }

    void writeString(const std::string_view str) {
        write({reinterpret_cast<const u8*>(str.data()), str.size()});
        write<u8>(0);
    }

    std::vector<u8> flush() {
        return std::move(mData);
    }

    size_t tell() const {
        return mOffset;
    }

    void seek(size_t offset) {
        mOffset = offset;
    }

    void align(size_t alignment) {
        mOffset = util::align(mOffset, alignment);
    }

    void expand(size_t size) {
        mData.resize(size);
    }

    void serialize();

private:
    struct AssetKey {
        std::string_view key;
        s32 index;

        bool operator<(const AssetKey& other) const {
            if (this->key == other.key) {
                return this->index < other.index;
            }
            return this->key < other.key;
        }
    };

    struct UserInfo {
        std::map<AssetKey, u16> assetIdMap{};
        std::vector<u64> containerOffsets{};
        u64 totalSize;
        u64 triggerTableOffset;
        s32 assetCount;
        s32 randomContainerCount;
    };

    const xlink2::ResourceHeader calcOffsets();
    void writeParamDefine(const ParamDefine&);
    void writePDT();
    void writeParam(const Param&);
    void writeUser(const User&, const u32);

    System* mSystem = nullptr;
    size_t mOffset = 0;
    std::vector<u8> mData{};
    std::unordered_map<std::string_view, u64> mPDTStringOffsets{};
    std::unordered_map<std::string_view, u64> mStringOffsets{};
    std::vector<u64> mConditionOffsets{};
    std::vector<u64> mTriggerParamOffsets{};
    std::vector<u64> mAssetParamOffsets{};
    std::vector<u64> mArrangeGroupParamOffsets{};
    std::map<u32, u64> mUserOffsets{};
    std::map<u32, UserInfo> mUserInfo{};
    size_t mPDTNameTableSize;
};

} // namespace banana
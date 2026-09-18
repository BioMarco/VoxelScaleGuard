#pragma once

// =============================================================================
// A VERBATIM COPY of the deployed voxel-size reader in vc_render_tifxyz.
//
// Upstream: ScrollPrize/villa
//   commit  757f70c0140a4cfbbbd44975ef09558444b96980
//   file    volume-cartographer/apps/src/vc_render_tifxyz.cpp
//   lines   982-1003
//
// LICENCE — THIS FILE IS GPL-3.0-or-later, NOT MIT.
//   It is Volume Cartographer code, verbatim.
//   Copyright (C) 2023 EduceLab.
//   Licensed under the GNU General Public License, version 3 or (at your
//   option) any later version: see LICENSE-GPL-3.0.txt at the repository root
//   and NOTICE.md section 2. It is NOT covered by this repository's MIT
//   licence in LICENSE.
//   Unmodified: no change of any kind has been made to the copied lines.
//
// It is reproduced here — rather than simulated — so that the tests exercise the
// actual decision procedure, including its exact key order and its exact
// exception behaviour, instead of a paraphrase of it.
//
// This file is NOT the deliverable and must never be "fixed": its whole purpose
// is to keep the broken behaviour executable so the defect can be demonstrated
// and so the patch can be shown to change it.
//
// Do not edit. To re-sync with upstream, re-copy the lines above.
// =============================================================================

#include <filesystem>
#include <optional>

namespace vsguard::upstream
{

// --- BEGIN verbatim upstream body (vc_render_tifxyz.cpp:986-1003) ---
inline std::optional<double> readVolumeVoxelSize(const std::filesystem::path& volPath)
{
    auto tryFile = [](const std::filesystem::path& p, const char* key) -> std::optional<double> {
        if (!std::filesystem::exists(p)) return std::nullopt;
        try {
            utils::Json j = utils::Json::parse_file(p.string());
            utils::Json sub = key ? (j.is_object() && j.contains(key) ? j[key] : utils::Json{}) : j;
            if (key && !sub.is_object()) return std::nullopt;
            if (sub.is_object() && sub.contains("voxelsize") && sub["voxelsize"].is_number())
                return sub["voxelsize"].get_double();
        } catch (...) {}
        return std::nullopt;
    };
    if (auto v = tryFile(volPath / "meta.json", nullptr)) return v;
    if (auto v = tryFile(volPath / "metadata.json", "scan")) return v;
    if (auto v = tryFile(volPath / "metadata.json", nullptr)) return v;
    return std::nullopt;
}
// --- END verbatim upstream body ---

} // namespace vsguard::upstream

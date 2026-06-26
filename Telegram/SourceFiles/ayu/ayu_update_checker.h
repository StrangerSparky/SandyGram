#pragma once

#include <QtCore/QString>

namespace AyuUpdate {

// The current local version. Increment when you release a new build.
inline constexpr int kSandyGramVersion = 1;

// Opens the GitHub releases page where users can check for and download updates.
QString sandygramReleasesUrl();
QString currentSandyGramVersionText();

} // namespace AyuUpdate

#pragma once

#include <QtCore/QString>
#include <functional>

namespace AyuUpdate {

// Increment when you release a new build.
inline constexpr int kSandyGramVersion = 1;

// Starts an async check against version.txt on GitHub.
void startSandyGramUpdateCheck(std::function<void()> onDone = nullptr);

bool isSandyGramUpdateAvailable();
int latestSandyGramVersion();
QString sandygramReleasesUrl();
QString currentSandyGramVersionText();

} // namespace AyuUpdate

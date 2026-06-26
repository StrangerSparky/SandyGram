#pragma once

#include <QtCore/QString>
#include <functional>

namespace AyuUpdate {

// Increment when you release a new SandyGram version.
// Create a "version.txt" file in your repo root with the matching number.
inline constexpr int kSandyGramVersion = 1;

// Starts an async HTTP check. When done, calls onDone() on the main thread.
void startSandyGramUpdateCheck(std::function<void()> onDone = nullptr);

bool isSandyGramUpdateAvailable();
int latestSandyGramVersion();
QString sandygramReleasesUrl();
QString currentSandyGramVersionText();

} // namespace AyuUpdate

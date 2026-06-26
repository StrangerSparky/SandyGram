#include "ayu_update_checker.h"

#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

namespace AyuUpdate {
namespace {

constexpr auto kRepoOwner   = "StrangerSparky";
constexpr auto kRepoName    = "SandyGram";

} // namespace

QString sandygramReleasesUrl() {
	return QString("https://github.com/%1/%2/releases")
		.arg(kRepoOwner, kRepoName);
}

QString currentSandyGramVersionText() {
	return QString("SandyGram v%1").arg(kSandyGramVersion);
}

} // namespace AyuUpdate

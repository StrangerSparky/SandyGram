#include "ayu_update_checker.h"

#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace AyuUpdate {
namespace {

constexpr auto kRepoOwner   = "StrangerSparky";
constexpr auto kRepoName    = "SandyGram";
constexpr auto kVersionFileBranch = "dev";

int g_latestVersion = 0;
bool g_updateAvailable = false;

} // namespace

QString sandygramVersionFileUrl() {
	return QString("https://raw.githubusercontent.com/%1/%2/%3/version.txt")
		.arg(kRepoOwner, kRepoName, kVersionFileBranch);
}

QString sandygramReleasesUrl() {
	return QString("https://github.com/%1/%2/releases")
		.arg(kRepoOwner, kRepoName);
}

QString currentSandyGramVersionText() {
	return QString("SandyGram v%1").arg(kSandyGramVersion);
}

int latestSandyGramVersion() {
	return g_latestVersion;
}

bool isSandyGramUpdateAvailable() {
	return g_updateAvailable;
}

void startSandyGramUpdateCheck(std::function<void()> onDone) {
	static auto manager = std::make_unique<QNetworkAccessManager>();
	auto reply = manager->get(QNetworkRequest(QUrl(sandygramVersionFileUrl())));
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto data = reply->readAll().trimmed();
			bool ok = false;
			const auto version = data.toInt(&ok);
			if (ok) {
				g_latestVersion = version;
				g_updateAvailable = (version > kSandyGramVersion);
			}
		}
		reply->deleteLater();
		if (onDone) {
			onDone();
		}
	});
}

} // namespace AyuUpdate

// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "ayu_autoreply.h"

#include "ayu_settings.h"

#include "apiwrap.h"
#include "api/api_common.h"
#include "api/api_send_progress.h"
#include "core/application.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "data/data_photo.h"
#include "data/data_document.h"
#include "data/business/data_shortcut_messages.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"

#include "ayu/utils/telegram_helpers.h"

#include <QStringList>
#include <QRegularExpression>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <functional>
#include <memory>

namespace AyuAutoReply {

namespace {

// Set of peer IDs we have already replied to in this session so we don't
// send duplicate auto-replies while a delayed reply is still pending.
using SessionId = uint64;
using PeerId = uint64;
std::unordered_map<SessionId, std::unordered_set<PeerId>> _pendingReplies;

// Peers to which we have already sent the first-message quick reply this session.
std::unordered_map<SessionId, std::unordered_set<PeerId>> _firstMessageSent;

// Peers that sent an image/photo — auto-reply is stopped for them for this session.
std::unordered_map<SessionId, std::unordered_set<PeerId>> _stoppedPeers;

// Trigger words already used per peer — each word fires at most once per peer per session.
std::unordered_map<SessionId, std::unordered_map<PeerId, std::unordered_set<QString>>> _usedTriggers;

[[nodiscard]] bool markUsedTrigger(SessionId sessionId, PeerId peerId, const QString &triggerWord) {
	auto &perPeer = _usedTriggers[sessionId];
	auto &used = perPeer[peerId];
	if (used.count(triggerWord)) {
		return false;
	}
	used.insert(triggerWord);
	return true;
}

[[nodiscard]] QString normalizedShortcutName(const QString &value) {
	auto result = value.trimmed();
	if (result.startsWith(u'/', Qt::CaseInsensitive)) {
		result = result.mid(1).trimmed();
	}
	return result;
}

[[nodiscard]] QStringList configuredShortcutNames(
		const QString &primaryShortcut,
		const QString &extraShortcuts) {
	QStringList result;
	const auto add = [&](const QString &value) {
		const auto normalized = normalizedShortcutName(value);
		if (normalized.isEmpty()) {
			return;
		}
		for (const auto &existing : result) {
			if (!QString::compare(existing, normalized, Qt::CaseInsensitive)) {
				return;
			}
		}
		result.push_back(normalized);
	};
	add(primaryShortcut);
	for (const auto &entry : extraShortcuts.split(
		QRegularExpression(u"[\\s,;|]+"_q),
		Qt::SkipEmptyParts)) {
		add(entry);
	}
	return result;
}

[[nodiscard]] bool isSelectedClientSession(not_null<Main::Session*> session) {
	return (&session->account() == &Core::App().activeAccount());
}

[[nodiscard]] bool markPendingReply(SessionId sessionId, PeerId peerId) {
	auto &pending = _pendingReplies[sessionId];
	if (pending.count(peerId)) {
		return false;
	}
	pending.insert(peerId);
	return true;
}

[[nodiscard]] bool markStoppedPeer(SessionId sessionId, PeerId peerId) {
	auto &stopped = _stoppedPeers[sessionId];
	if (stopped.count(peerId)) {
		return false;
	}
	stopped.insert(peerId);
	return true;
}

[[nodiscard]] bool isPeerStopped(SessionId sessionId, PeerId peerId) {
	const auto i = _stoppedPeers.find(sessionId);
	if (i == end(_stoppedPeers)) {
		return false;
	}
	return i->second.count(peerId) > 0;
}

void clearPendingReply(SessionId sessionId, PeerId peerId) {
	const auto i = _pendingReplies.find(sessionId);
	if (i == end(_pendingReplies)) {
		return;
	}
	i->second.erase(peerId);
	if (i->second.empty()) {
		_pendingReplies.erase(i);
	}
}

[[nodiscard]] BusinessShortcutId lookupShortcutId(
		not_null<Main::Session*> session,
		const QString &name) {
	const auto wanted = normalizedShortcutName(name);
	if (wanted.isEmpty()) {
		return {};
	}
	const auto &list = session->data().shortcutMessages().shortcuts().list;
	for (const auto &[id, shortcut] : list) {
		if (!QString::compare(shortcut.name, wanted, Qt::CaseInsensitive)) {
			return id;
		}
	}
	return {};
}

[[nodiscard]] bool hasAnyImageMedia(not_null<HistoryItem*> item) {
	const auto media = item->media();
	if (!media) {
		return false;
	}
	if (const auto photo = media->photo()) {
		if (!photo->hasVideo()) {
			return true;
		}
	}
	if (const auto document = media->document()) {
		if (document->isImage()) {
			return true;
		}
	}
	return false;
}

// Returns true when the incoming message text matches at least one trigger
// word from the configured list (case-insensitive).
[[nodiscard]] bool matchesTriggerWords(
		const QString &messageText,
		const QString &triggerWords) {
	const auto words = triggerWords.split(
		QRegularExpression(u"[,;|\\n\\r]+"_q),
		Qt::SkipEmptyParts);
	for (const auto &raw : words) {
		const auto word = raw.trimmed();
		if (word.isEmpty()) {
			continue;
		}
		if (messageText.contains(word, Qt::CaseInsensitive)) {
			return true;
		}
	}
	return false;
}

struct AutoReplyRule {
	QString triggerWord;   // lowercase, trimmed
	QString shortcutName;  // already normalised
};

// Parse "autoReplyRules" setting: one rule per line, format "word=shortcut".
// Lines without '=' are ignored.
[[nodiscard]] std::vector<AutoReplyRule> parseAutoReplyRules(const QString &raw) {
	std::vector<AutoReplyRule> result;
	const auto lines = raw.split(
		QRegularExpression(u"[\\s,;|]+"_q),
		Qt::SkipEmptyParts);
	for (const auto &line : lines) {
		const auto eqPos = line.indexOf(u'=');
		if (eqPos < 1) {
			continue; // no '=' or empty key
		}
		AutoReplyRule rule;
		rule.triggerWord = line.left(eqPos).trimmed().toLower();
		rule.shortcutName = normalizedShortcutName(line.mid(eqPos + 1));
		if (rule.triggerWord.isEmpty() || rule.shortcutName.isEmpty()) {
			continue;
		}
		result.push_back(std::move(rule));
	}
	return result;
}

// Find the first rule whose trigger word appears in messageText.
// Returns the normalised shortcut name, or empty string if no rule matches.
[[nodiscard]] QString findMatchingRuleShortcut(
		const std::vector<AutoReplyRule> &rules,
		const QString &messageText) {
	const auto lower = messageText.toLower();
	for (const auto &rule : rules) {
		if (lower.contains(rule.triggerWord)) {
			return rule.shortcutName;
		}
	}
	return {};
}

void sendReply(
		not_null<Main::Session*> session,
		not_null<History*> history,
		QStringList configuredValues,
		Fn<void(bool)> done) {
	const auto kRetryDelayMs = 700;
	const auto kMaxRetries = 12;

	auto trySendAndFinish = std::make_shared<std::function<void(int)>>();
	*trySendAndFinish = [=](int retriesLeft) {
		if (!isSelectedClientSession(session)) {
			done(false);
			return;
		}

		if (configuredValues.empty()) {
			done(false);
			return;
		}

		auto &shortcuts = session->data().shortcutMessages();
		shortcuts.preloadShortcuts();

		auto shortcutCandidates = QStringList();
		shortcutCandidates.reserve(configuredValues.size());
		for (const auto &value : configuredValues) {
			const auto normalized = normalizedShortcutName(value);
			if (!normalized.isEmpty()) {
				shortcutCandidates.push_back(normalized);
			}
		}
		if (shortcutCandidates.empty()) {
			done(false);
			return;
		}

		static std::mt19937 rng(std::random_device{}());
		const auto shortcutOffset = !shortcutCandidates.empty()
			? std::uniform_int_distribution<int>(0, shortcutCandidates.size() - 1)(rng)
			: 0;

		for (int i = 0; i != shortcutCandidates.size(); ++i) {
			const auto &name = shortcutCandidates[(shortcutOffset + i) % shortcutCandidates.size()];
			const auto shortcutId = shortcuts.lookupShortcutId(name);
			if (shortcutId) {
				session->api().sendShortcutMessages(history->peer, shortcutId);
				done(true);
				return;
			}
		}

		if (retriesLeft > 0) {
			dispatchToMainThread([=] {
				(*trySendAndFinish)(retriesLeft - 1);
			}, kRetryDelayMs);
		} else {
			done(false);
		}
	};

	(*trySendAndFinish)(kMaxRetries);
}

void scheduleReply(
		not_null<Main::Session*> session,
		not_null<History*> history,
		QStringList configuredValues,
		SessionId sessionId,
		PeerId peerId,
		bool simulateTyping,
		int typingDelayMs,
		int sendingDelayMs) {
	if (simulateTyping && typingDelayMs > 0) {
		// Start typing indicator immediately, then send after typingDelay + sendingDelay.
		dispatchToMainThread([session, history, configuredValues, sessionId, peerId, typingDelayMs, sendingDelayMs]() {
			if (!isSelectedClientSession(session)) {
				clearPendingReply(sessionId, peerId);
				return;
			}
			session->sendProgressManager().update(
				history,
				Api::SendProgressType::Typing);

			// Schedule actual send after typing delay has elapsed.
			dispatchToMainThread([session, history, configuredValues, sessionId, peerId, sendingDelayMs]() {
				if (!isSelectedClientSession(session)) {
					clearPendingReply(sessionId, peerId);
					return;
				}
				session->sendProgressManager().cancelTyping(history);

				if (sendingDelayMs > 0) {
					dispatchToMainThread([session, history, configuredValues, sessionId, peerId]() {
						if (!isSelectedClientSession(session)) {
							clearPendingReply(sessionId, peerId);
							return;
						}
						sendReply(session, history, configuredValues, [=](bool) {
							clearPendingReply(sessionId, peerId);
						});
					}, sendingDelayMs);
				} else {
					sendReply(session, history, configuredValues, [=](bool) {
						clearPendingReply(sessionId, peerId);
					});
				}
			}, typingDelayMs);
		});
	} else if (sendingDelayMs > 0) {
		dispatchToMainThread([session, history, configuredValues, sessionId, peerId]() {
			if (!isSelectedClientSession(session)) {
				clearPendingReply(sessionId, peerId);
				return;
			}
			sendReply(session, history, configuredValues, [=](bool) {
				clearPendingReply(sessionId, peerId);
			});
		}, sendingDelayMs);
	} else {
		dispatchToMainThread([session, history, configuredValues, sessionId, peerId]() {
			if (!isSelectedClientSession(session)) {
				clearPendingReply(sessionId, peerId);
				return;
			}
			sendReply(session, history, configuredValues, [=](bool) {
				clearPendingReply(sessionId, peerId);
			});
		});
	}
}

void processItem(not_null<HistoryItem*> item) {
	// Only handle incoming, non-outgoing, regular (not service) messages.
	if (item->out() || item->isService()) {
		return;
	}

	const auto &settings = AyuSettings::getInstance();
	if (!settings.autoReplyEnabled) {
		return;
	}

	const auto history = item->history();
	const auto peer = history->peer;

	// Never reply to channels or broadcast-type chats where we can't DM back.
	// Only reply to private chats (user) and optionally groups.
	if (!peer->isUser()) {
		return;
	}

	// Don't reply to ourselves (Saved Messages).
	if (peer->isSelf()) {
		return;
	}

	// Don't reply to bots.
	if (const auto user = peer->asUser()) {
		if (user->isBot()) {
			return;
		}
	}

	const auto session = &history->session();
	if (!isSelectedClientSession(session)) {
		return;
	}
	const auto sessionId = session->uniqueId();
	const auto peerId = peer->id.value;

	// If peer has been stopped (sent an image), don't auto-reply anymore.
	if (isPeerStopped(sessionId, peerId)) {
		return;
	}

	// If message contains an image/photo, stop auto-replying to this peer.
	if (hasAnyImageMedia(item)) {
		(void)markStoppedPeer(sessionId, peerId);
		return;
	}

	const auto configuredShortcuts = configuredShortcutNames(
		settings.autoReplyMessage,
		settings.autoReplyMessages);
	if (configuredShortcuts.empty()) {
		return;
	}

	// ── First-message quick reply ────────────────────────────────────────────
	// If enabled and this is the first message we've ever seen from this peer
	// this session, check for trigger-word rules first, then fall back to
	// the first-message shortcut.
	if (settings.autoReplyFirstMessageEnabled) {
		auto &firstSent = _firstMessageSent[sessionId];
		if (!firstSent.count(peerId)) {
			const auto &rulesRaw = settings.autoReplyRules;
			QString shortcutToSend;
			const auto text = item->originalText().text;

			// If trigger-word blacklist is enabled and the first message contains
			// any trigger word, skip the first-message auto-reply entirely.
			if (settings.autoReplyTriggerWordsOnly && !settings.autoReplyTriggerWords.trimmed().isEmpty()) {
				if (matchesTriggerWords(text, settings.autoReplyTriggerWords)) {
					return;
				}
			}

			// Check if this first message matches any trigger-word rule
			if (!rulesRaw.trimmed().isEmpty()) {
				const auto rules = parseAutoReplyRules(rulesRaw);
				const auto lower = text.toLower();
				QString matchedTrigger;
				for (const auto &rule : rules) {
					if (lower.contains(rule.triggerWord)) {
						matchedTrigger = rule.triggerWord;
						shortcutToSend = rule.shortcutName;
						break;
					}
				}
				if (!matchedTrigger.isEmpty()) {
					// Each trigger word fires at most once per peer.
					(void)markUsedTrigger(sessionId, peerId, matchedTrigger);
				}
			}

			// Fall back to first-message shortcut if no trigger matched
			if (shortcutToSend.isEmpty()) {
				const auto firstShortcut = normalizedShortcutName(settings.autoReplyFirstMessage);
				auto firstCandidates = firstShortcut.isEmpty()
					? configuredShortcuts
					: configuredShortcutNames(firstShortcut, QString());
				if (!firstCandidates.empty()) {
					shortcutToSend = firstCandidates.first();
				}
			}

			if (!shortcutToSend.isEmpty() && markPendingReply(sessionId, peerId)) {
				firstSent.insert(peerId);
				const auto simulateTyping = settings.autoReplySimulateTyping;
				const auto typingDelayMs = simulateTyping ? settings.autoReplyTypingDelayMs : 0;
				const auto sendingDelayMs = settings.autoReplySendingDelayMs;
				scheduleReply(
					session,
					history,
					QStringList{ shortcutToSend },
					sessionId,
					peerId,
					simulateTyping,
					typingDelayMs,
					sendingDelayMs);
				return;
			}
		}
	}

	// ── Normal auto-reply ────────────────────────────────────────────────────
	// Only use per-trigger-word rules ("word=shortcut" lines).
	// If no rule matches, don't reply.

	const auto &rulesRaw = settings.autoReplyRules;
	if (rulesRaw.trimmed().isEmpty()) {
		// No rules configured -> don't reply
		return;
	}

	const auto rules = parseAutoReplyRules(rulesRaw);
	const auto text = item->originalText().text;

	// If trigger-word blacklist is enabled and message contains any trigger word,
	// skip the auto-reply for this message.
	if (settings.autoReplyTriggerWordsOnly && !settings.autoReplyTriggerWords.trimmed().isEmpty()) {
		if (matchesTriggerWords(text, settings.autoReplyTriggerWords)) {
			return;
		}
	}

	const auto lower = text.toLower();
	QString matchedTrigger;
	QString matchedShortcut;
	for (const auto &rule : rules) {
		if (lower.contains(rule.triggerWord)) {
			matchedTrigger = rule.triggerWord;
			matchedShortcut = rule.shortcutName;
			break;
		}
	}

	if (matchedShortcut.isEmpty()) {
		// No rule matched -> don't reply
		return;
	}

	// Each trigger word fires at most once per peer per session.
	if (!markUsedTrigger(sessionId, peerId, matchedTrigger)) {
		return;
	}

	QStringList shortcutsToSend{ matchedShortcut };

	// De-duplicate only while a delayed reply is in-flight.
	if (!markPendingReply(sessionId, peerId)) {
		return;
	}

	// Schedule the work with delays on the main thread.
	const bool simulateTyping = settings.autoReplySimulateTyping;
	const int typingDelayMs = settings.autoReplyTypingDelayMs;
	const int sendingDelayMs = settings.autoReplySendingDelayMs;

	scheduleReply(
		session,
		history,
		shortcutsToSend,
		sessionId,
		peerId,
		simulateTyping,
		typingDelayMs,
		sendingDelayMs);
}

} // namespace

void initForSession(not_null<Main::Session*> session) {
	// Ensure shortcut list starts loading for premium quick replies.
	session->data().shortcutMessages().preloadShortcuts();

	// Subscribe to every new item added to this session's data store.
	// The subscription lives as long as the session does.
	session->data().newItemAdded(
	) | rpl::on_next([](not_null<HistoryItem*> item) {
		processItem(item);
	}, session->lifetime());
}

} // namespace AyuAutoReply

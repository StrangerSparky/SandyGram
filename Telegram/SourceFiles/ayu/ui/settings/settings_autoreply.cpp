// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "settings_autoreply.h"

#include "lang_auto.h"
#include "settings_ayu_utils.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/boxes/edit_mark_box.h"
#include "settings/settings_common.h"
#include "styles/style_settings.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {
namespace {

[[nodiscard]] int DelayToIndex(int delayMs) {
	switch (delayMs) {
	case 0: return 0;
	case 1000: return 1;
	case 2000: return 2;
	case 3000: return 3;
	case 5000: return 4;
	default: return 2;
	}
}

[[nodiscard]] int IndexToDelay(int index) {
	switch (index) {
	case 0: return 0;
	case 1: return 1000;
	case 2: return 2000;
	case 3: return 3000;
	case 4: return 5000;
	default: return 2000;
	}
}

[[nodiscard]] QString TriggerWordsSummary(const QString &value) {
	const auto trimmed = value.trimmed();
	return trimmed.isEmpty()
		? tr::ayu_AutoReplyTriggerWordsNotSet(tr::now)
		: trimmed;
}

[[nodiscard]] QString RulesSummary(const QString &value) {
	const auto lines = value.trimmed().split(
		QRegularExpression(u"[\\n\\r]+"_q),
		Qt::SkipEmptyParts);
	// Count only valid "word=shortcut" lines.
	int valid = 0;
	for (const auto &line : lines) {
		if (line.contains(u'=')) {
			++valid;
		}
	}
	if (valid == 0) {
		return tr::ayu_AutoReplyRulesNotSet(tr::now);
	}
	if (valid == 1) {
		// Show the single rule inline.
		for (const auto &line : lines) {
			if (line.contains(u'=')) {
				return line.trimmed();
			}
		}
	}
	return tr::ayu_AutoReplyRulesCount(tr::now, lt_count, valid);
}

[[nodiscard]] QString ReplyMessageSummary(const QString &value) {
	const auto trimmed = value.trimmed();
	return trimmed.isEmpty()
		? tr::ayu_AutoReplyMessageNotSet(tr::now)
		: trimmed;
}

[[nodiscard]] QString CombinedShortcuts(const QString &primary, const QString &extra) {
	QStringList result;
	const auto add = [&](const QString &value) {
		const auto trimmed = value.trimmed();
		if (trimmed.isEmpty()) {
			return;
		}
		for (const auto &existing : result) {
			if (!QString::compare(existing, trimmed, Qt::CaseInsensitive)) {
				return;
			}
		}
		result.push_back(trimmed);
	};
	add(primary);
	for (const auto &entry : extra.split(QRegularExpression(u"[,;|\\n\\r]+"_q), Qt::SkipEmptyParts)) {
		add(entry);
	}
	return result.join(u'\n');
}

[[nodiscard]] QString ShortcutListSummary(const QString &value) {
	const auto lines = value.split(QRegularExpression(u"[\\n\\r]+"_q), Qt::SkipEmptyParts);
	if (lines.empty()) {
		return tr::ayu_AutoReplyMessageNotSet(tr::now);
	}
	if (lines.size() == 1) {
		return lines.front().trimmed();
	}
	return tr::ayu_AutoReplyMultipleRepliesCount(tr::now, lt_count, lines.size());
}

} // namespace

rpl::producer<QString> AyuAutoReply::title() {
	return tr::ayu_AutoReplyTitle();
}

AyuAutoReply::AyuAutoReply(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
	: Section(parent) {
	setupContent(controller);
}

void AyuAutoReply::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto *settings = &AyuSettings::getInstance();

	AddSkip(content);
	AddSubsectionTitle(content, tr::ayu_AutoReplyTitle());

	AddButtonWithIcon(
		content,
		tr::ayu_AutoReplyToggle(),
		st::settingsButtonNoIcon
	)->toggleOn(
		AyuSettings::get_autoReplyEnabledReactive()
	)->toggledValue(
	) | rpl::on_next(
		[=](bool enabled) {
			AyuSettings::set_autoReplyEnabled(enabled);
			AyuSettings::save();
		},
		content->lifetime());

	AddSkip(content);
	AddDivider(content);
	AddSkip(content);

	AddSubsectionTitle(content, tr::ayu_AutoReplyMessage());

	const auto shortcutsValue = content->lifetime().make_state<rpl::variable<QString>>(
		ShortcutListSummary(CombinedShortcuts(
			settings->autoReplyMessage,
			settings->autoReplyMessages)));

	AddButtonWithLabel(
		content,
		tr::ayu_AutoReplyMessage(),
		shortcutsValue->value(),
		st::settingsButtonNoIcon
	)->addClickHandler([=] {
		auto box = Box<EditMarkBox>(
			tr::ayu_AutoReplyMessage(),
			CombinedShortcuts(
				settings->autoReplyMessage,
				settings->autoReplyMessages),
			tr::ayu_AutoReplyMessagePlaceholder(tr::now),
			[=](const QString &value) {
				const auto lines = value.split(QRegularExpression(u"[,;|\\n\\r]+"_q), Qt::SkipEmptyParts);
				QStringList cleaned;
				for (const auto &line : lines) {
					const auto trimmed = line.trimmed();
					if (!trimmed.isEmpty()) {
						cleaned.push_back(trimmed);
					}
				}
				AyuSettings::set_autoReplyMessage(cleaned.isEmpty() ? QString() : cleaned.front());
				AyuSettings::set_autoReplyMessages(cleaned.size() > 1
					? cleaned.mid(1).join(u'\n')
					: QString());
				AyuSettings::save();
				shortcutsValue->force_assign(ShortcutListSummary(cleaned.join(u'\n')));
			});
		Ui::show(std::move(box));
	});

	AddSkip(content);
	AddDividerText(content, tr::ayu_AutoReplyMessageHint());
	AddSkip(content);

	// ── First Message Quick Reply ────────────────────────────────────────────
	AddSubsectionTitle(content, tr::ayu_AutoReplyFirstMessageTitle());

	AddButtonWithIcon(
		content,
		tr::ayu_AutoReplyFirstMessageToggle(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->autoReplyFirstMessageEnabled)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled) {
			return (enabled != settings->autoReplyFirstMessageEnabled);
		}
	) | rpl::on_next(
		[=](bool enabled) {
			AyuSettings::set_autoReplyFirstMessageEnabled(enabled);
			AyuSettings::save();
		},
		content->lifetime());

	const auto firstMsgValue = content->lifetime().make_state<rpl::variable<QString>>(
		ReplyMessageSummary(settings->autoReplyFirstMessage));

	AddButtonWithLabel(
		content,
		tr::ayu_AutoReplyFirstMessage(),
		firstMsgValue->value(),
		st::settingsButtonNoIcon
	)->addClickHandler([=] {
		auto box = Box<EditMarkBox>(
			tr::ayu_AutoReplyFirstMessage(),
			settings->autoReplyFirstMessage,
			tr::ayu_AutoReplyMessagePlaceholder(tr::now),
			[=](const QString &value) {
				AyuSettings::set_autoReplyFirstMessage(value);
				AyuSettings::save();
				firstMsgValue->force_assign(ReplyMessageSummary(value));
			});
		Ui::show(std::move(box));
	});

	AddSkip(content);
	AddDividerText(content, tr::ayu_AutoReplyFirstMessageHint());
	AddSkip(content);

	// ── Timing ───────────────────────────────────────────────────────────────
	AddButtonWithIcon(
		content,
		tr::ayu_AutoReplySimulateTyping(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->autoReplySimulateTyping)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled) {
			return (enabled != settings->autoReplySimulateTyping);
		}
	) | rpl::on_next(
		[=](bool enabled) {
			AyuSettings::set_autoReplySimulateTyping(enabled);
			AyuSettings::save();
		},
		content->lifetime());

	const auto delayOptions = std::vector<QString>{
		tr::ayu_AutoReplyDelayInstant(tr::now),
		tr::ayu_AutoReplyDelay1s(tr::now),
		tr::ayu_AutoReplyDelay2s(tr::now),
		tr::ayu_AutoReplyDelay3s(tr::now),
		tr::ayu_AutoReplyDelay5s(tr::now),
	};

	AddChooseButtonWithIconAndRightText(
		content,
		controller,
		DelayToIndex(settings->autoReplyTypingDelayMs),
		delayOptions,
		tr::ayu_AutoReplyTypingDelay(),
		tr::ayu_AutoReplyTypingDelay(),
		[=](int index) {
			AyuSettings::set_autoReplyTypingDelayMs(IndexToDelay(index));
			AyuSettings::save();
		});

	AddChooseButtonWithIconAndRightText(
		content,
		controller,
		DelayToIndex(settings->autoReplySendingDelayMs),
		delayOptions,
		tr::ayu_AutoReplySendingDelay(),
		tr::ayu_AutoReplySendingDelay(),
		[=](int index) {
			AyuSettings::set_autoReplySendingDelayMs(IndexToDelay(index));
			AyuSettings::save();
		});

	AddSkip(content);
	AddDivider(content);
	AddSkip(content);

	// ── Trigger Word Rules ───────────────────────────────────────────────────
	// Each rule links one trigger word to a specific quick reply shortcut.
	// Format per line: "word=shortcutName"  (e.g. "hello=away")
	// The first matching rule wins. When no rules are configured, the global
	// shortcut pool above is used (optionally filtered by trigger words).
	AddSubsectionTitle(content, tr::ayu_AutoReplyRules());

	const auto rulesValue = content->lifetime().make_state<rpl::variable<QString>>(
		RulesSummary(settings->autoReplyRules));

	AddButtonWithLabel(
		content,
		tr::ayu_AutoReplyRules(),
		rulesValue->value(),
		st::settingsButtonNoIcon
	)->addClickHandler([=] {
		auto box = Box<EditMarkBox>(
			tr::ayu_AutoReplyRules(),
			settings->autoReplyRules,
			tr::ayu_AutoReplyRulesPlaceholder(tr::now),
			[=](const QString &value) {
				AyuSettings::set_autoReplyRules(value);
				AyuSettings::save();
				rulesValue->force_assign(RulesSummary(value));
			});
		Ui::show(std::move(box));
	});

	AddSkip(content);
	AddDividerText(content, tr::ayu_AutoReplyRulesHint());
	AddSkip(content);

	// ── Legacy trigger-words toggle (used when no rules are configured) ──────
	AddButtonWithIcon(
		content,
		tr::ayu_AutoReplyTriggerWordsOnly(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->autoReplyTriggerWordsOnly)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled) {
			return (enabled != settings->autoReplyTriggerWordsOnly);
		}
	) | rpl::on_next(
		[=](bool enabled) {
			AyuSettings::set_autoReplyTriggerWordsOnly(enabled);
			AyuSettings::save();
		},
		content->lifetime());

	const auto triggerWordsValue = content->lifetime().make_state<rpl::variable<QString>>(
		TriggerWordsSummary(settings->autoReplyTriggerWords));

	AddButtonWithLabel(
		content,
		tr::ayu_AutoReplyTriggerWords(),
		triggerWordsValue->value(),
		st::settingsButtonNoIcon
	)->addClickHandler([=] {
		auto box = Box<EditMarkBox>(
			tr::ayu_AutoReplyTriggerWords(),
			settings->autoReplyTriggerWords,
			tr::ayu_AutoReplyTriggerWordsPlaceholder(tr::now),
			[=](const QString &value) {
				AyuSettings::set_autoReplyTriggerWords(value);
				AyuSettings::save();
				triggerWordsValue->force_assign(TriggerWordsSummary(value));
			});
		Ui::show(std::move(box));
	});

	AddSkip(content);
	AddDividerText(content, tr::ayu_AutoReplyDescription());
	AddSkip(content);

	ResizeFitChild(this, content);
}

} // namespace Settings

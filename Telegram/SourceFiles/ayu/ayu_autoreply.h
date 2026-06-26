// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#pragma once

#include "base/basic_types.h"

namespace Main {
class Session;
} // namespace Main

namespace AyuAutoReply {

// Call once per session when it is ready (logged in).
void initForSession(not_null<Main::Session*> session);

} // namespace AyuAutoReply

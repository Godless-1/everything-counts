/* EVERYTHING COUNTS - a card counting story for the Game Boy Advance
 * Copyright (C) 2026 Godless-1
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.  See LICENSE for the full text.
 */
/* ========================================================================
 *  story_data.c - seven nights in Kestrel City.
 * ===================================================================== */
#include "story.h"

/* ---- 0: prologue ----------------------------------------------------- */
static const SLine b0[] = {
{ CH_SYSTEM, "HALCYON CONTINUITY / ESCROW NOTICE. SUBJECT: NAEM, RUI. STATUS: HELD. BALANCE DUE 2,000,000cr OVER SEVEN NIGHTS. ON DEFAULT THE ASSET WILL BE RESOLVED INTO COMPONENTS." },
{ CH_KESTREL, "Resolved into components. They wrote that and somebody signed off on the wording." },
{ CH_SABLE, "Don't read it twice, it doesn't get better. The Verge came down, your sister's body went with it, and Halcyon caught her mind on the way out. Now they're renting her back to you." },
{ CH_KESTREL, "And my lace burned out in the same collapse." },
{ CH_SABLE, "Which is the only good news in this room. Everyone in this city is chipped. The pits scan for co-processor chatter before you've finished sitting down. You're the only person on the strip who can count a shoe without broadcasting that you're counting a shoe." },
{ CH_KESTREL, "I can't count. Not any more. The lace did it for eleven years." },
{ CH_SABLE, "Then you've got until tomorrow night to learn it back. Grey's got a training construct in the basement. Go be a person again." },
};

/* ---- 1: night one ---------------------------------------------------- */
static const SLine b1[] = {
{ CH_SABLE, "Seven nights. Seven payments. Miss one and they take a piece of her as a late fee - and they do mean a piece. Memory, preference, the way she laughs. Components." },
{ CH_KESTREL, "How much tonight?" },
{ CH_SABLE, "Twenty thousand. It is nothing. It's designed to be nothing, so you'll believe the rest is possible." },
{ CH_SABLE, "Start low. The Slag Pit runs two decks and nobody down there is paid enough to watch you. Learn the room before you learn the money." },
{ CH_KESTREL, "And when the money gets big enough to notice?" },
{ CH_SABLE, "Then you'll want chrome. And chrome is where this stops being about blackjack." },
};

/* ---- 2: Mara --------------------------------------------------------- */
static const SLine b2[] = {
{ CH_KESTREL, "Third shoe at the Lotus and the dealer is short-shuffling. Not badly. She's leaving a slug of low cards on top and burning through it fast. It costs the house a fraction of a percent and it costs me my count." },
{ CH_MARA, "You've been watching my hands for twenty minutes. Say something or tip me." },
{ CH_KESTREL, "You're not cheating the house. You're cheating the shoe. Why?" },
{ CH_MARA, "My kid's on rented lungs. Halcyon Medical, forty thousand a month, and they meter the air. I move the deck for a syndicate two tables over and they pay the difference." },
{ CH_MARA, "You can walk to the pit. There's a forty-thousand bounty on floor irregularities. It'd clear your month." },
{ CH_KESTREL, "And they'd take your hands off at the wrist for it." },
{ CH_MARA, "Yes. And my kid would breathe for eleven more days." },
};
static const SLine b2a[] = {
{ CH_MARA, "Then you're a fool, and I'll cut you a fair deck. Deep. You'll get your penetration, and nobody will ask why." },
};
static const SLine b2b[] = {
{ CH_PIT, "Appreciated, sir. The house doesn't forget a friend. You'll find our people watch you a little less closely from now on." },
};

/* ---- 3: Grey and the hot chrome -------------------------------------- */
static const SLine b3[] = {
{ CH_GREY, "I've got something for you. Mnemonic Weave, second-hand, still warm enough that I had to keep it in the cooler." },
{ CH_KESTREL, "Warm." },
{ CH_GREY, "The donor is alive. She's in a ward on sublevel two with a hole in her temporal lobe and a payment plan. She consented. That's a real word with real paperwork behind it, and it means nothing, and we both know it means nothing." },
{ CH_GREY, "It'll flag every index play at the table. It's worth two hundred thousand and I'll put it in you for free, because I taught you to count and I would like you to survive the week." },
{ CH_KESTREL, "And the cost." },
{ CH_GREY, "Somebody else's reflexes wearing your hands. You'll stop noticing after a night. That's the cost. Not the pain. The not noticing." },
};
static const SLine b3a[] = {
{ CH_GREY, "Lie back. It'll feel like remembering something that never happened to you." },
};
static const SLine b3b[] = {
{ CH_GREY, "Good. I hoped you'd say that. I'd have done it anyway, but I hoped." },
};

/* ---- 4: Sable's laundry ---------------------------------------------- */
static const SLine b4[] = {
{ CH_SABLE, "The Kuroda people need eight hundred thousand to look like it was lost at a table. You're the table." },
{ CH_KESTREL, "You want me to lose on purpose." },
{ CH_SABLE, "I want you to lose three hundred and fifty thousand on purpose, in front of a camera, playing badly enough that it reads as human. In exchange they put a hand over you. No pit scan touches you for the rest of the week." },
{ CH_KESTREL, "And the eight hundred thousand goes where?" },
{ CH_SABLE, "Into a war that's going to happen whether you fund it or not, killing people who were going to be killed anyway. That's not an argument. It's just true." },
{ CH_KESTREL, "You've gotten very good at saying terrible things gently." },
{ CH_SABLE, "I've had a lot of practice, and so will you." },
};
static const SLine b4a[] = {
{ CH_SABLE, "Three fifty down. Sleep badly. You are covered." },
};
static const SLine b4b[] = {
{ CH_SABLE, "Then you're on your own out there, and the pits are going to start looking hard at a flatliner who never loses." },
};

/* ---- 5: Ozerov ------------------------------------------------------- */
static const SLine b5[] = {
{ CH_KESTREL, "The whale at the high table has been feeding the Sutra for four hours. He doesn't count. He doesn't need to. He needs to be here." },
{ CH_OZEROV, "You are counting. Don't insult me by denying it, I can see the shape of your betting. I used to do it myself, before the chrome, when it was still a skill." },
{ CH_OZEROV, "Sit heads-up with me. No pit, no cameras, my marker against yours. You'll take everything I have, because you're better than me now." },
{ CH_KESTREL, "Why would you do that?" },
{ CH_OZEROV, "My wife is in Halcyon escrow. Ward nine, same as your sister. I have been losing here on purpose for four hours because the moment my balance clears I stop being able to pay, and the moment I stop being able to pay they resolve her." },
{ CH_OZEROV, "If you take my money tonight, she goes into a composite by Friday. If you don't, I keep bleeding slowly and she keeps existing. I am asking you to beat me, and I am telling you what it costs." },
{ CH_KESTREL, "That's not a game. That's a hostage exchange." },
{ CH_OZEROV, "Everything on this floor is a hostage exchange. This one just told you the terms." },
};
static const SLine b5a[] = {
{ CH_OZEROV, "Deal. And thank you. I could not have stopped on my own." },
};
static const SLine b5b[] = {
{ CH_OZEROV, "Then we are two people at the same table, losing to the same house, for the same reason. Go on. There's a shoe waiting." },
};

/* ---- 6: Vesk --------------------------------------------------------- */
static const SLine b6[] = {
{ CH_VESK, "Ms. Naem. Adjudicator Vesk, Halcyon Continuity. Please sit. The chair is very good; we spend money on the chairs." },
{ CH_VESK, "You have cost us a great deal this week, and we have enjoyed it. Nobody counts any more. Your whole species stopped doing arithmetic in 2061 and we did not notice what we had lost until you sat down." },
{ CH_VESK, "So. An offer. We image you tonight. Not a copy - an instance. It counts for us, in every pit we own, forever, and it does not get tired and it cannot be scanned because it is the scanner." },
{ CH_VESK, "In exchange the balance is cleared. Rui walks out in a body we'll grow for her. Tomorrow. Not Friday - tomorrow." },
{ CH_KESTREL, "And me." },
{ CH_VESK, "You go home. You keep your life. The instance is not you, it is a very good description of you, and legally that distinction is settled." },
{ CH_KESTREL, "Legally." },
{ CH_VESK, "It is the only kind of settled there is. Take the night. The offer expires with the shoe." },
};
static const SLine b6a[] = {
{ CH_VESK, "A wise decision. Please look at the light." },
};
static const SLine b6b[] = {
{ CH_VESK, "Disappointing. Then we will see you at the Vault, and we will see which of us was describing the other." },
};

/* ---- 7: the Vault ---------------------------------------------------- */
static const SLine b7[] = {
{ CH_VESK, "One deck. Dealt to the felt. No burn, no cut card, no cameras - because there will be no witnesses to disagree with our account of tonight." },
{ CH_VESK, "The remaining balance, in one session. You may leave at any time. Leaving is a default." },
{ CH_KESTREL, "You cut deep enough for me to count it." },
{ CH_VESK, "Of course. We want to know whether it is really you doing it. That has become the interesting question." },
};

/* ====================================================================== */
const Beat BEATS[] = {
/* 0 - prologue */
{ "ESCROW NOTICE", BG_RAIN, RAMP_MAG, b0, 7,
  "GREY IS WAITING DOWNSTAIRS.",
  { "GO LEARN IT BACK", 0, 0 }, { "opens the DRY RUN course", 0, 0 }, 1,
  { EFF_TUTORIAL, 0, 0 }, { 0, 0, 0 } },

/* 1 */
{ "NIGHT ONE", BG_RAIN, RAMP_CYAN, b1, 6,
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, { 0, 0, 0 }, { 0, 0, 0 } },

/* 2 - Mara */
{ "THE DEALER", BG_FELT, RAMP_ALERT, b2, 7,
  "WHAT DO YOU DO?",
  { "SAY NOTHING", "TAKE THE BOUNTY", 0 },
  { "she cuts you a deeper shoe", "+120,000cr and the pit's goodwill", 0 }, 2,
  { EFF_MARA_SPARE, EFF_MARA_SELL, 0 }, { b2a, b2b, 0 } },

/* 3 - hot chrome */
{ "STILL WARM", BG_CLINIC, RAMP_GREEN, b3, 6,
  "TAKE THE WEAVE?",
  { "TAKE IT", "REFUSE", 0 },
  { "free MNEMONIC WEAVE / +14 DRIFT", "nothing changes. that's the point", 0 }, 2,
  { EFF_HOT_TAKE, EFF_HOT_REFUSE, 0 }, { b3a, b3b, 0 } },

/* 4 - laundering */
{ "COVER", BG_TERMINAL, RAMP_GOLD, b4, 7,
  "THROW THE HANDS?",
  { "THROW THEM", "REFUSE", 0 },
  { "-350,000cr / no pit scans this week", "the pits start looking at you", 0 }, 2,
  { EFF_LAUNDER_YES, EFF_LAUNDER_NO, 0 }, { b4a, b4b, 0 } },

/* 5 - Ozerov */
{ "THE WHALE", BG_FELT, RAMP_MAG, b5, 8,
  "TAKE HIS MARKER?",
  { "TAKE IT ALL", "WALK AWAY", 0 },
  { "+620,000cr / his wife is resolved", "you keep counting your own way", 0 }, 2,
  { EFF_OZEROV_TAKE, EFF_OZEROV_SPARE, 0 }, { b5a, b5b, 0 } },

/* 6 - Vesk */
{ "AN INSTANCE OF YOU", BG_VOID, RAMP_CHROME, b6, 8,
  "SELL THE COUNT?",
  { "LET THEM IMAGE YOU", "REFUSE", 0 },
  { "Rui walks tomorrow. you end here.", "the Vault, then. one deck.", 0 }, 2,
  { EFF_SELL_MIND, EFF_REFUSE_HALCYON, 0 }, { b6a, b6b, 0 } },

/* 7 - the Vault */
{ "THE VAULT", BG_VOID, RAMP_CHROME, b7, 4,
  "SIT DOWN.",
  { "SIT", 0, 0 }, { "one deck, heads up, everything", 0, 0 }, 1,
  { EFF_VAULT, 0, 0 }, { 0, 0, 0 } },
};
const int NBEATS = (int)(sizeof(BEATS) / sizeof(Beat));

int story_beat_for_night(int night)
{
    if (night < 1) return 0;
    if (night > 7) return 7;
    return night;
}

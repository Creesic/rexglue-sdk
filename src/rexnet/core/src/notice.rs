// @file        rexnet/core/src/notice.rs
// @brief       Signed developer notices: a bouncing logo or a line of text.

//! Signed developer notices (docs/technical/rexnet-notices.md).
//!
//! One ed25519 key signs every notice and every roster ask; runners verify
//! against [`NOTICE_PUBLIC_KEY`]. Effects are a closed enum: no executable
//! commands, guest addresses or arbitrary memory writes travel on this channel.

use libp2p::identity::{ed25519, Keypair};
use libp2p::PeerId;
use serde::{Deserialize, Serialize};

/// Paste the line `rexnet-notice keygen` prints. All zeros accepts nothing.
pub const NOTICE_PUBLIC_KEY: [u8; 32] = [0x3a, 0x8a, 0x50, 0x11, 0x60, 0xf1, 0xd8, 0x62, 0xbb, 0xd4, 0x52, 0x09, 0xce, 0x68, 0x00, 0x2c, 0x9c, 0x56, 0x24, 0xbf, 0x32, 0x42, 0x63, 0xd2, 0x71, 0x06, 0xf1, 0x79, 0xe3, 0xf7, 0x7b, 0x0a];

pub const TEXT_MAX_CHARS: usize = 120;
/// Furthest a notice's end time may lie ahead.
pub const MAX_LIFETIME_SECS: u64 = 7 * 24 * 60 * 60;
/// How far a roster ask's timestamp may be from local time.
pub const ASK_SKEW_MS: u64 = 5 * 60 * 1000;
/// Clock skew allowed past MAX_LIFETIME_SECS; same 5 minutes as ASK_SKEW_MS.
pub const LIFETIME_SLACK_SECS: u64 = 5 * 60;
/// Real bodies are a few hundred bytes.
pub const BODY_MAX: usize = 2048;
/// Where a banner is centered by default, in percent of the window: the lower third.
pub const DEFAULT_AT: (u8, u8) = (50, 79);
pub const SIZE_MIN: u16 = 1;
pub const SIZE_MAX: u16 = 1000;
/// Most titles one broadcast may name.
pub const TITLES_MAX: usize = 16;

/// Bytes exactly as signed, and their ed25519 signature. Receivers verify
/// these bytes before decoding them, so nothing is ever re-encoded.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Signed {
    pub body: Vec<u8>,
    pub sig: Vec<u8>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum Kind {
    Dvd,
    Text(String),
    /// Ends whatever its slot showed, and outlives it.
    Clear,
    /// Optional singleplayer effect, interpreted by the title add-on.
    /// Appended to preserve the existing postcard enum discriminants.
    MagneticTraffic { strength: u16 },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Notice {
    pub schema: u8, // = 1
    /// Issue time in Unix milliseconds; higher replaces lower.
    pub seq: u64,
    pub kind: Kind,
    /// End time, Unix seconds.
    pub not_after: u64,
    /// Shown only by runners built before this build code.
    pub built_before: Option<String>,
    /// Peer id bytes of the targets; empty is a broadcast.
    pub to: Vec<Vec<u8>>,
    /// The titles that show it. Empty means every title.
    pub titles: Vec<u32>,
    /// How a text notice is drawn; only text may carry one.
    pub style: Option<TextStyle>,
}

/// Banner text color: fixed, or sweeping through the hues.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum Color {
    Rgb([u8; 3]),
    Cycle,
}

/// How a text notice moves. Append-only: postcard encodes the variant index.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize)]
pub enum Motion {
    /// Where `at` puts it.
    #[default]
    Still,
    /// Around the window like the logo, changing color at each wall.
    Bounce,
    /// One line scrolling right to left at `at`'s height.
    Marquee,
}

/// How a text notice turns. Append-only, as `Motion`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize)]
pub enum Spin {
    #[default]
    None,
    /// In the plane of the screen.
    Flat,
    /// Around the vertical axis: edge-on, then mirrored (XP 3D Text's "Spin").
    Spin3d,
    /// Rocking in the plane.
    SeeSaw,
    /// Gentle rocking on two axes.
    Wobble,
    /// Turning on all three axes.
    Tumble,
}

/// How a text notice is drawn. Only valid on `Kind::Text`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct TextStyle {
    pub color: Color,
    /// Center of the banner, in percent of the window (0..=100 each).
    pub at: (u8, u8),
    /// Percent of the normal size (SIZE_MIN..=SIZE_MAX).
    pub size: u16,
    /// `motion` and `spin` are the last bytes of a notice body, so runners
    /// built before them stop reading at `size` and show the text still.
    pub motion: Motion,
    pub spin: Spin,
}

impl Default for TextStyle {
    /// White, in the lower third, at the normal size, still.
    fn default() -> Self {
        TextStyle { color: Color::Rgb([255, 255, 255]), at: DEFAULT_AT, size: 100, motion: Motion::Still, spin: Spin::None }
    }
}

/// Roster ask: answered only when signed, fresh, and sent by `panel_peer`.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Ask {
    pub schema: u8, // = 1
    pub panel_peer: Vec<u8>,
    pub issued_ms: u64,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Reject {
    Signature,
    Malformed,
    Expired,
    TooFarAhead,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Slot {
    Broadcast,
    Personal,
}

/// Signed bytes start with the message type, so a signature over one type
/// can never be accepted as the other.
pub trait Signable: Serialize {
    const CONTEXT: &'static [u8];
}

impl Signable for Notice {
    const CONTEXT: &'static [u8] = b"rexnet-notice/1\0";
}

impl Signable for Ask {
    const CONTEXT: &'static [u8] = b"rexnet-ask/1\0";
}

fn signed_message(context: &[u8], body: &[u8]) -> Vec<u8> {
    [context, body].concat()
}

pub fn sign<T: Signable>(key: &Keypair, value: &T) -> Signed {
    let body = postcard::to_allocvec(value).expect("notice bodies always encode");
    let sig = key.sign(&signed_message(T::CONTEXT, &body)).expect("ed25519 signing cannot fail");
    Signed { body, sig }
}

/// The 32 bytes `NOTICE_PUBLIC_KEY` holds for `key`.
pub fn public_bytes(key: &Keypair) -> Option<[u8; 32]> {
    key.public().try_into_ed25519().ok().map(|public| public.to_bytes())
}

/// The body, if `key` signed it. The all-zero key decodes to a small-order
/// point that plain ed25519 verification can be forged against, so it is
/// refused outright rather than trusted to fail.
fn verified<'a>(key: &[u8; 32], signed: &'a Signed, context: &[u8]) -> Option<&'a [u8]> {
    if *key == [0; 32] || signed.body.len() > BODY_MAX {
        return None;
    }
    let key = ed25519::PublicKey::try_from_bytes(key).ok()?;
    key.verify(&signed_message(context, &signed.body), &signed.sig).then_some(signed.body.as_slice())
}

pub fn open_notice(key: &[u8; 32], signed: &Signed, now: u64) -> Result<Notice, Reject> {
    let body = verified(key, signed, Notice::CONTEXT).ok_or(Reject::Signature)?;
    let mut notice: Notice = postcard::from_bytes(body).map_err(|_| Reject::Malformed)?;
    if notice.schema != 1 || notice.titles.len() > TITLES_MAX {
        return Err(Reject::Malformed);
    }
    if matches!(notice.kind, Kind::MagneticTraffic { strength } if strength > 100) {
        return Err(Reject::Malformed);
    }
    if notice.built_before.as_deref().is_some_and(|code| build_code_minutes(code).is_none()) {
        return Err(Reject::Malformed);
    }
    if let Some(TextStyle { at: (x, y), size, .. }) = notice.style {
        if !matches!(notice.kind, Kind::Text(_)) || x > 100 || y > 100 || !(SIZE_MIN..=SIZE_MAX).contains(&size) {
            return Err(Reject::Malformed);
        }
    }
    if notice.not_after <= now {
        return Err(Reject::Expired);
    }
    if notice.not_after > now.saturating_add(MAX_LIFETIME_SECS + LIFETIME_SLACK_SECS) {
        return Err(Reject::TooFarAhead);
    }
    if let Kind::Text(text) = &mut notice.kind {
        *text = clean_text(text);
    }
    Ok(notice)
}

/// True when `signed` is a fresh ask signed by `key` and sent by `from`.
pub fn ask_is_valid(key: &[u8; 32], signed: &Signed, from: &PeerId, now_ms: u64) -> bool {
    let Some(body) = verified(key, signed, Ask::CONTEXT) else { return false };
    let Ok(ask) = postcard::from_bytes::<Ask>(body) else { return false };
    ask.schema == 1 && ask.panel_peer == from.to_bytes() && ask.issued_ms.abs_diff(now_ms) <= ASK_SKEW_MS
}

/// Control characters become spaces; at most TEXT_MAX_CHARS characters.
pub fn clean_text(text: &str) -> String {
    text.chars().map(|c| if c.is_control() { ' ' } else { c }).take(TEXT_MAX_CHARS).collect()
}

/// Build codes count whole minutes since this instant (2026-01-01 00:00 UTC).
pub const BUILD_EPOCH_UNIX: u64 = 1_767_225_600;
const BUILD_CODE_LEN: usize = 5;

/// A runner's 5-character base-36 code (`0-9A-Z`) for the Unix time it was
/// linked. Fixed width, so codes sort in build order. None before the epoch or
/// past "ZZZZZ" (year 2141).
pub fn build_code(unix_seconds: u64) -> Option<String> {
    let mut minutes = unix_seconds.checked_sub(BUILD_EPOCH_UNIX)? / 60;
    if minutes >= 36u64.pow(BUILD_CODE_LEN as u32) {
        return None;
    }
    let mut code = ['0'; BUILD_CODE_LEN];
    for digit in code.iter_mut().rev() {
        *digit = char::from_digit((minutes % 36) as u32, 36)?.to_ascii_uppercase();
        minutes /= 36;
    }
    Some(code.into_iter().collect())
}

/// Minutes since BUILD_EPOCH_UNIX for a code: exactly 5 characters of 0-9 and
/// A-Z (lowercase accepted). None for anything else.
pub fn build_code_minutes(code: &str) -> Option<u64> {
    if code.len() != BUILD_CODE_LEN {
        return None;
    }
    code.chars().try_fold(0u64, |minutes, c| Some(minutes * 36 + u64::from(c.to_digit(36)?)))
}

/// True when `mine` is a build code strictly earlier than the `below` code.
/// Anything that is not a build code is never older.
pub fn older_than(mine: &str, below: &str) -> bool {
    matches!((build_code_minutes(mine), build_code_minutes(below)), (Some(mine), Some(below)) if mine < below)
}

pub fn unix_ms() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_millis() as u64)
        .unwrap_or(0)
}

/// What a runner holds: the broadcast notice, kept signed to pass on, and
/// the one addressed to it.
#[derive(Debug, Default)]
pub struct Slots {
    broadcast: Option<(Notice, Signed)>,
    personal: Option<Notice>,
}

impl Slots {
    /// Keep `notice` when it is newer than its slot's. A runner keeps a
    /// targeted notice only when it is named. Returns the slot that changed.
    /// A runner running `title` keeps only notices for it; `None` (the
    /// engine's own publish) skips that check. An ended notice blocks nothing.
    pub fn offer(&mut self, notice: Notice, signed: &Signed, me: &PeerId, title: Option<u32>, now: u64) -> Option<Slot> {
        if title.is_some_and(|title| !notice.titles.is_empty() && !notice.titles.contains(&title)) {
            return None;
        }
        let blocks = |held: &Notice| held.not_after > now && held.seq >= notice.seq;
        if notice.to.is_empty() {
            if self.broadcast.as_ref().is_some_and(|(held, _)| blocks(held)) {
                return None;
            }
            self.broadcast = Some((notice, signed.clone()));
            return Some(Slot::Broadcast);
        }
        if !notice.to.contains(&me.to_bytes()) || self.personal.as_ref().is_some_and(blocks) {
            return None;
        }
        self.personal = Some(notice);
        Some(Slot::Personal)
    }

    /// The broadcast notice to give a peer that just connected, while it lasts.
    pub fn handoff(&self, now: u64) -> Option<Signed> {
        self.broadcast.as_ref().filter(|(held, _)| held.not_after > now).map(|(_, signed)| signed.clone())
    }

    /// What a runner with build code `build` shows in `slot` now: unexpired,
    /// not a Clear, and built before the notice's `built_before` when it
    /// names one.
    pub fn visible(&self, slot: Slot, now: u64, build: &str) -> Option<Notice> {
        let held = match slot {
            Slot::Broadcast => self.broadcast.as_ref().map(|(held, _)| held),
            Slot::Personal => self.personal.as_ref(),
        }?;
        let gated_out = held.built_before.as_deref().is_some_and(|before| !older_than(build, before));
        (held.not_after > now && held.kind != Kind::Clear && !gated_out).then(|| held.clone())
    }

    /// Newest eligible slot controls the effect. Include Clear so a newer
    /// personal clear can stop an older broadcast effect (and vice versa).
    pub fn magnetic_strength(&self, now: u64, build: &str) -> u16 {
        let latest = [self.broadcast.as_ref().map(|(held, _)| held), self.personal.as_ref()]
            .into_iter().flatten()
            .filter(|held| held.not_after > now && held.built_before.as_deref()
                .is_none_or(|before| older_than(build, before)))
            .max_by_key(|held| held.seq);
        match latest.map(|held| &held.kind) {
            Some(Kind::MagneticTraffic { strength }) if *strength <= 100 => *strength,
            _ => 0,
        }
    }
}

#[cfg(test)]
pub(crate) fn test_keypair() -> Keypair {
    let secret = ed25519::SecretKey::try_from_bytes([7u8; 32]).unwrap();
    Keypair::from(ed25519::Keypair::from(secret))
}

#[cfg(test)]
pub(crate) fn test_public() -> [u8; 32] {
    public_bytes(&test_keypair()).unwrap()
}

#[cfg(test)]
mod tests {
    use super::*;

    const NOW: u64 = 1_800_000_000;
    /// A runner's build code: 2026-10-05 00:31 UTC.
    const BUILD: &str = "08JSV";
    const BUILD_UNIX: u64 = BUILD_EPOCH_UNIX + 398_911 * 60;

    fn peer() -> PeerId {
        Keypair::generate_ed25519().public().to_peer_id()
    }

    fn notice(kind: Kind, seq: u64, to: Vec<Vec<u8>>) -> Notice {
        Notice { schema: 1, seq, kind, not_after: NOW + 600, built_before: None, to, titles: Vec::new(), style: None }
    }

    #[test]
    fn a_notice_signed_by_the_key_opens() {
        let n = notice(Kind::Dvd, 1, vec![]);
        assert_eq!(open_notice(&test_public(), &sign(&test_keypair(), &n), NOW), Ok(n));
    }

    #[test]
    fn magnetic_strength_is_signed_bounded_and_keeps_legacy_wire_tags() {
        for strength in [0, 1, 100] {
            let n = notice(Kind::MagneticTraffic { strength }, 1, vec![]);
            let signed = sign(&test_keypair(), &n);
            assert_eq!(open_notice(&test_public(), &signed, NOW), Ok(n));
            let mut tampered = signed;
            *tampered.body.last_mut().unwrap() ^= 1;
            assert_eq!(open_notice(&test_public(), &tampered, NOW), Err(Reject::Signature));
        }
        let bad = notice(Kind::MagneticTraffic { strength: 101 }, 1, vec![]);
        assert_eq!(open_notice(&test_public(), &sign(&test_keypair(), &bad), NOW), Err(Reject::Malformed));
        for (kind, tag) in [(Kind::Dvd, 0), (Kind::Text("x".into()), 1), (Kind::Clear, 2)] {
            assert_eq!(postcard::to_allocvec(&notice(kind, 1, vec![])).unwrap()[2], tag);
        }
    }

    #[test]
    fn magnetic_effect_respects_expiry_build_gate_and_newer_clear_in_either_slot() {
        let me = peer();
        let mut slots = Slots::default();
        let signed = sign(&test_keypair(), &notice(Kind::Dvd, 1, vec![]));
        let mut effect = notice(Kind::MagneticTraffic { strength: 100 }, 1, vec![]);
        effect.built_before = Some("08JSW".into());
        slots.offer(effect, &signed, &me, None, NOW);
        assert_eq!(slots.magnetic_strength(NOW, BUILD), 100);
        assert_eq!(slots.magnetic_strength(NOW, "08JSW"), 0);
        assert_eq!(slots.magnetic_strength(NOW + 600, BUILD), 0);
        slots.offer(notice(Kind::Clear, 2, vec![me.to_bytes()]), &signed, &me, None, NOW);
        assert_eq!(slots.magnetic_strength(NOW, BUILD), 0, "personal clear stops broadcast");
        slots.offer(notice(Kind::MagneticTraffic { strength: 25 }, 3, vec![me.to_bytes()]), &signed, &me, None, NOW);
        assert_eq!(slots.magnetic_strength(NOW, BUILD), 25);
        slots.offer(notice(Kind::Clear, 4, vec![]), &signed, &me, None, NOW);
        assert_eq!(slots.magnetic_strength(NOW, BUILD), 0, "broadcast clear stops personal");
    }

    #[test]
    fn another_key_or_a_changed_body_is_refused() {
        let signed = sign(&test_keypair(), &notice(Kind::Dvd, 1, vec![]));
        let other = public_bytes(&Keypair::generate_ed25519()).unwrap();
        assert_eq!(open_notice(&other, &signed, NOW), Err(Reject::Signature));
        let mut changed = signed.clone();
        *changed.body.last_mut().unwrap() ^= 1;
        assert_eq!(open_notice(&test_public(), &changed, NOW), Err(Reject::Signature));
    }

    #[test]
    fn the_all_zero_key_is_forgeable_so_it_is_refused() {
        // R = the identity point and s = 0 verifies under plain ed25519
        // against the order-4 point the zero key decodes to, whenever the
        // challenge hash is a multiple of 4: about one body in four.
        let zero = ed25519::PublicKey::try_from_bytes(&[0; 32]).unwrap();
        let mut sig = vec![0u8; 64];
        sig[0] = 1;
        let forged: Vec<Signed> = (0..64u64)
            .map(|seq| Signed { body: postcard::to_allocvec(&notice(Kind::Dvd, seq, vec![])).unwrap(), sig: sig.clone() })
            .filter(|s| zero.verify(&signed_message(Notice::CONTEXT, &s.body), &s.sig))
            .collect();
        assert!(!forged.is_empty(), "plain verification accepts forgeries against the zero key");
        for signed in &forged {
            assert_eq!(open_notice(&[0; 32], signed, NOW), Err(Reject::Signature));
        }
    }

    #[test]
    fn expired_and_far_future_end_times_are_refused() {
        let key = test_keypair();
        let mut n = notice(Kind::Dvd, 1, vec![]);
        n.not_after = NOW;
        assert_eq!(open_notice(&test_public(), &sign(&key, &n), NOW), Err(Reject::Expired));
        n.not_after = NOW + MAX_LIFETIME_SECS + LIFETIME_SLACK_SECS + 1;
        assert_eq!(open_notice(&test_public(), &sign(&key, &n), NOW), Err(Reject::TooFarAhead));
        n.not_after = NOW + MAX_LIFETIME_SECS + LIFETIME_SLACK_SECS;
        assert!(open_notice(&test_public(), &sign(&key, &n), NOW).is_ok());
    }

    #[test]
    fn junk_bodies_and_unknown_schemas_are_malformed() {
        let key = test_keypair();
        let junk = Signed { body: vec![0xFF; 10], sig: key.sign(&signed_message(Notice::CONTEXT, &[0xFF; 10])).unwrap() };
        assert_eq!(open_notice(&test_public(), &junk, NOW), Err(Reject::Malformed));
        let mut n = notice(Kind::Dvd, 1, vec![]);
        n.schema = 2;
        assert_eq!(open_notice(&test_public(), &sign(&key, &n), NOW), Err(Reject::Malformed));
    }

    #[test]
    fn text_loses_control_characters_and_stops_at_120_characters() {
        assert_eq!(clean_text("new\nupdate\t08JSV\u{7}"), "new update 08JSV ");
        assert_eq!(clean_text(&"é".repeat(200)).chars().count(), TEXT_MAX_CHARS);
        let n = notice(Kind::Text("a\r\nb".into()), 1, vec![]);
        let opened = open_notice(&test_public(), &sign(&test_keypair(), &n), NOW).unwrap();
        assert_eq!(opened.kind, Kind::Text("a  b".into()));
    }

    #[test]
    fn a_link_time_encodes_to_a_five_character_base_36_minute_count() {
        // 2026-10-05 00:31 UTC (20:31 EDT on the 4th) is minute 398,911.
        assert_eq!(BUILD_UNIX, 1_791_160_260);
        assert_eq!(build_code(BUILD_UNIX).as_deref(), Some("08JSV"));
        assert_eq!(build_code_minutes("08JSV"), Some(398_911));
        // Whole minutes: the seconds are dropped, the next minute is the next code.
        assert_eq!(build_code(BUILD_UNIX + 59).as_deref(), Some("08JSV"));
        assert_eq!(build_code(BUILD_UNIX + 60).as_deref(), Some("08JSW"));
        // The epoch, and the last code.
        assert_eq!(build_code(BUILD_EPOCH_UNIX).as_deref(), Some("00000"));
        let last = BUILD_EPOCH_UNIX + (36u64.pow(5) - 1) * 60;
        assert_eq!(build_code(last).as_deref(), Some("ZZZZZ"));
        assert_eq!(build_code_minutes("ZZZZZ"), Some(36u64.pow(5) - 1));
        assert_eq!(build_code(last + 60), None, "past ZZZZZ");
        assert_eq!(build_code(BUILD_EPOCH_UNIX - 1), None, "before the epoch");
        assert_eq!(build_code(0), None);
    }

    #[test]
    fn codes_round_trip_and_sort_in_build_order() {
        let times = [0u64, 1, 35, 36, 1295, 1296, 46_655, 46_656, 398_911, 1_679_615, 1_679_616, 36u64.pow(5) - 1];
        let codes: Vec<String> = times.iter().map(|m| build_code(BUILD_EPOCH_UNIX + m * 60).unwrap()).collect();
        for (minutes, code) in times.iter().zip(&codes) {
            assert_eq!(code.len(), 5, "{code}");
            assert_eq!(build_code_minutes(code), Some(*minutes), "{code}");
        }
        let mut sorted = codes.clone();
        sorted.sort();
        assert_eq!(sorted, codes, "fixed width, so text order is build order");
    }

    #[test]
    fn lowercase_codes_are_accepted_and_malformed_ones_are_not() {
        assert_eq!(build_code_minutes("08jsv"), Some(398_911));
        assert_eq!(build_code_minutes("08JsV"), Some(398_911));
        for bad in ["", "0", "08JS", "08JSV0", "08JS ", " 08JSV", "08-SV", "08JS+", "08J\u{c9}V", "0\u{e9}JS", "08JSVV", "\u{ff10}8JSV", "0.1.5"] {
            assert_eq!(build_code_minutes(bad), None, "{bad:?}");
        }
    }

    #[test]
    fn older_than_means_a_valid_earlier_build_and_garbage_is_never_older() {
        assert!(older_than("08JSV", "08JSW"));
        assert!(older_than("00000", "ZZZZZ"));
        assert!(older_than("08jsv", "08JSW"), "case does not matter");
        assert!(!older_than("08JSV", "08JSV"));
        assert!(!older_than("08JSW", "08JSV"));
        for bad in ["", "0.1.5", "08JS", "08JSVV", "08-SV"] {
            assert!(!older_than("08JSV", bad), "{bad:?}");
            assert!(!older_than(bad, "08JSW"), "{bad:?}");
        }
    }

    #[test]
    fn a_built_before_that_is_not_a_build_code_is_malformed() {
        let key = test_keypair();
        let open = |built_before: &str| {
            let mut n = notice(Kind::Dvd, 1, vec![]);
            n.built_before = Some(built_before.into());
            open_notice(&test_public(), &sign(&key, &n), NOW)
        };
        assert!(open("08JSV").is_ok());
        assert!(open("08jsv").is_ok());
        for bad in ["", "0.1.5", "08JS", "08JSVV", "08-SV"] {
            assert_eq!(open(bad), Err(Reject::Malformed), "{bad:?}");
        }
    }

    #[test]
    fn broadcast_keeps_the_newest_and_hands_off_until_it_ends() {
        let (me, key) = (peer(), test_keypair());
        let mut slots = Slots::default();
        let newer = notice(Kind::Dvd, 5, vec![]);
        assert_eq!(slots.offer(newer.clone(), &sign(&key, &newer), &me, None, NOW), Some(Slot::Broadcast));
        let older = notice(Kind::Text("old".into()), 4, vec![]);
        assert_eq!(slots.offer(older.clone(), &sign(&key, &older), &me, None, NOW), None);
        assert_eq!(slots.offer(newer.clone(), &sign(&key, &newer), &me, None, NOW), None, "a resend is not newer");
        assert_eq!(slots.handoff(NOW), Some(sign(&key, &newer)));
        assert_eq!(slots.handoff(NOW + 600), None);
        assert_eq!(slots.visible(Slot::Broadcast, NOW, BUILD), Some(newer));
        assert_eq!(slots.visible(Slot::Broadcast, NOW + 600, BUILD), None, "ends mid-display");
    }

    #[test]
    fn clear_hides_its_slot_but_still_beats_older_notices() {
        let (me, key) = (peer(), test_keypair());
        let mut slots = Slots::default();
        let dvd = notice(Kind::Dvd, 1, vec![]);
        slots.offer(dvd.clone(), &sign(&key, &dvd), &me, None, NOW);
        let clear = notice(Kind::Clear, 2, vec![]);
        assert_eq!(slots.offer(clear.clone(), &sign(&key, &clear), &me, None, NOW), Some(Slot::Broadcast));
        assert_eq!(slots.visible(Slot::Broadcast, NOW, BUILD), None);
        assert_eq!(slots.offer(dvd.clone(), &sign(&key, &dvd), &me, None, NOW), None);
        assert_eq!(slots.handoff(NOW), Some(sign(&key, &clear)), "newcomers learn the clear");
    }

    #[test]
    fn a_targeted_notice_fills_only_the_named_runners_personal_slot() {
        let (me, other, key) = (peer(), peer(), test_keypair());
        let mut slots = Slots::default();
        let mine = notice(Kind::Text("hi".into()), 1, vec![vec![1, 2, 3], me.to_bytes()]);
        assert_eq!(slots.offer(mine.clone(), &sign(&key, &mine), &me, None, NOW), Some(Slot::Personal));
        assert_eq!(slots.handoff(NOW), None, "personal notices are never passed on");
        let theirs = notice(Kind::Dvd, 2, vec![other.to_bytes()]);
        assert_eq!(slots.offer(theirs.clone(), &sign(&key, &theirs), &me, None, NOW), None);
        assert_eq!(slots.visible(Slot::Personal, NOW, BUILD), Some(mine));
    }

    #[test]
    fn a_build_gated_notice_shows_only_on_runners_built_before_it() {
        let (me, key) = (peer(), test_keypair());
        let mut slots = Slots::default();
        let mut n = notice(Kind::Text("update".into()), 1, vec![]);
        n.built_before = Some("08JSW".into());
        slots.offer(n.clone(), &sign(&key, &n), &me, None, NOW);
        assert_eq!(slots.visible(Slot::Broadcast, NOW, "08JSV"), Some(n.clone()), "built a minute earlier");
        assert_eq!(slots.visible(Slot::Broadcast, NOW, "00000"), Some(n));
        assert_eq!(slots.visible(Slot::Broadcast, NOW, "08JSW"), None, "the same build");
        assert_eq!(slots.visible(Slot::Broadcast, NOW, "08JSX"), None, "newer");
        assert_eq!(slots.visible(Slot::Broadcast, NOW, "garbage"), None);
        assert_eq!(slots.visible(Slot::Broadcast, NOW, "?????"), None, "a runner that could not read its link time");
    }

    #[test]
    fn an_ask_counts_only_when_fresh_signed_and_from_its_sender() {
        let (panel, key) = (peer(), test_keypair());
        let now_ms = NOW * 1000;
        let ask = |p: &PeerId, issued_ms: u64| Ask { schema: 1, panel_peer: p.to_bytes(), issued_ms };
        let good = sign(&key, &ask(&panel, now_ms));
        assert!(ask_is_valid(&test_public(), &good, &panel, now_ms));
        assert!(!ask_is_valid(&test_public(), &good, &peer(), now_ms), "replayed by someone else");
        assert!(!ask_is_valid(&test_public(), &sign(&key, &ask(&panel, now_ms - ASK_SKEW_MS - 1)), &panel, now_ms));
        assert!(ask_is_valid(&test_public(), &sign(&key, &ask(&panel, now_ms + ASK_SKEW_MS)), &panel, now_ms));
        let stranger = sign(&Keypair::generate_ed25519(), &ask(&panel, now_ms));
        assert!(!ask_is_valid(&test_public(), &stranger, &panel, now_ms));
        assert!(!ask_is_valid(&[0; 32], &good, &panel, now_ms));
    }

    #[test]
    fn a_signature_over_one_type_is_not_accepted_as_the_other() {
        let (panel, key) = (peer(), test_keypair());
        let now_ms = NOW * 1000;
        let ask = Ask { schema: 1, panel_peer: panel.to_bytes(), issued_ms: now_ms };
        let notice = notice(Kind::Dvd, 1, vec![]);
        let signed_ask = sign(&key, &ask);
        let signed_notice = sign(&key, &notice);
        assert_eq!(open_notice(&test_public(), &signed_ask, NOW), Err(Reject::Signature));
        assert!(!ask_is_valid(&test_public(), &signed_notice, &panel, now_ms));
    }

    #[test]
    fn a_notice_shows_only_in_the_titles_it_names() {
        let (me, key) = (peer(), test_keypair());
        let (listed, unlisted) = (0x4D53_0064, 0x1122_3344);
        let mut scoped = notice(Kind::Dvd, 5, vec![]);
        scoped.titles = vec![0x0BAD_0001, listed];
        let signed = sign(&key, &scoped);
        assert_eq!(Slots::default().offer(scoped.clone(), &signed, &me, Some(listed), NOW), Some(Slot::Broadcast));
        assert_eq!(Slots::default().offer(scoped.clone(), &signed, &me, None, NOW), Some(Slot::Broadcast), "the publisher");
        let everywhere = notice(Kind::Dvd, 6, vec![]);
        assert_eq!(Slots::default().offer(everywhere.clone(), &sign(&key, &everywhere), &me, Some(unlisted), NOW), Some(Slot::Broadcast));
        // Refused for an unlisted title, and neither slot changes.
        let mut slots = Slots::default();
        let (older, mine) = (notice(Kind::Dvd, 1, vec![]), notice(Kind::Dvd, 1, vec![me.to_bytes()]));
        slots.offer(older.clone(), &sign(&key, &older), &me, Some(unlisted), NOW);
        slots.offer(mine.clone(), &sign(&key, &mine), &me, Some(unlisted), NOW);
        assert_eq!(slots.offer(scoped.clone(), &signed, &me, Some(unlisted), NOW), None);
        let mut scoped_mine = notice(Kind::Dvd, 5, vec![me.to_bytes()]);
        scoped_mine.titles = vec![listed];
        assert_eq!(slots.offer(scoped_mine.clone(), &sign(&key, &scoped_mine), &me, Some(unlisted), NOW), None);
        assert_eq!(slots.visible(Slot::Broadcast, NOW, BUILD), Some(older));
        assert_eq!(slots.visible(Slot::Personal, NOW, BUILD), Some(mine));
    }

    #[test]
    fn an_ended_notice_no_longer_blocks_an_older_one() {
        let (me, key) = (peer(), test_keypair());
        let mut slots = Slots::default();
        let (ended, ended_mine) = (notice(Kind::Dvd, 9, vec![]), notice(Kind::Dvd, 9, vec![me.to_bytes()]));
        slots.offer(ended.clone(), &sign(&key, &ended), &me, None, NOW);
        slots.offer(ended_mine.clone(), &sign(&key, &ended_mine), &me, None, NOW);
        let mut older = notice(Kind::Text("later".into()), 5, vec![]);
        older.not_after = NOW + 1200;
        let mut older_mine = older.clone();
        older_mine.to = vec![me.to_bytes()];
        assert_eq!(slots.offer(older.clone(), &sign(&key, &older), &me, None, NOW), None, "blocked while it lasts");
        let end = ended.not_after;
        assert_eq!(slots.offer(older.clone(), &sign(&key, &older), &me, None, end), Some(Slot::Broadcast));
        assert_eq!(slots.offer(older_mine.clone(), &sign(&key, &older_mine), &me, None, end), Some(Slot::Personal));
        assert_eq!(slots.visible(Slot::Broadcast, end, BUILD), Some(older));
        assert_eq!(slots.visible(Slot::Personal, end, BUILD), Some(older_mine));
    }

    #[test]
    fn too_many_titles_and_bad_styles_are_malformed() {
        let key = test_keypair();
        let open = |n: &Notice| open_notice(&test_public(), &sign(&key, n), NOW);
        let mut n = notice(Kind::Text("hi".into()), 1, vec![]);
        n.titles = vec![7; TITLES_MAX + 1];
        assert_eq!(open(&n), Err(Reject::Malformed));
        n.titles = vec![7; TITLES_MAX];
        assert!(open(&n).is_ok());
        let style = TextStyle::default();
        let bad = [
            TextStyle { at: (101, 50), ..style },
            TextStyle { at: (50, 101), ..style },
            TextStyle { size: SIZE_MIN - 1, ..style },
            TextStyle { size: SIZE_MAX + 1, ..style },
        ];
        for bad in bad {
            let mut n = notice(Kind::Text("hi".into()), 1, vec![]);
            n.style = Some(bad);
            assert_eq!(open(&n), Err(Reject::Malformed), "{bad:?}");
        }
        for kind in [Kind::Dvd, Kind::Clear] {
            let mut n = notice(kind, 1, vec![]);
            n.style = Some(style);
            assert_eq!(open(&n), Err(Reject::Malformed), "only text has a style");
        }
    }

    #[test]
    fn a_styled_text_notice_round_trips() {
        let mut n = notice(Kind::Text("New update out".into()), 1, vec![]);
        n.titles = vec![0x4D53_0064];
        for style in [
            TextStyle { color: Color::Rgb([1, 2, 3]), at: (0, 100), size: SIZE_MAX, ..TextStyle::default() },
            TextStyle { color: Color::Cycle, at: (100, 0), size: SIZE_MIN, ..TextStyle::default() },
        ] {
            n.style = Some(style);
            assert_eq!(open_notice(&test_public(), &sign(&test_keypair(), &n), NOW), Ok(n.clone()));
        }
    }

    #[test]
    fn effects_round_trip_and_sizes_run_from_1_to_1000() {
        let key = test_keypair();
        let open = |n: &Notice| open_notice(&test_public(), &sign(&key, n), NOW);
        let spins = [Spin::None, Spin::Flat, Spin::Spin3d, Spin::SeeSaw, Spin::Wobble, Spin::Tumble];
        let mut n = notice(Kind::Text("spin me".into()), 1, vec![]);
        for motion in [Motion::Still, Motion::Bounce, Motion::Marquee] {
            for spin in spins {
                n.style = Some(TextStyle { motion, spin, ..TextStyle::default() });
                assert_eq!(open(&n), Ok(n.clone()), "{motion:?} {spin:?}");
            }
        }
        for size in [1, 1000] {
            n.style = Some(TextStyle { size, ..TextStyle::default() });
            assert_eq!(open(&n), Ok(n.clone()), "{size}");
        }
        for size in [0, 1001] {
            n.style = Some(TextStyle { size, ..TextStyle::default() });
            assert_eq!(open(&n), Err(Reject::Malformed), "{size}");
        }
        assert_eq!((TextStyle::default().motion, TextStyle::default().spin), (Motion::Still, Spin::None));
    }

    #[test]
    fn effects_are_the_last_bytes_so_older_runners_read_still_text() {
        // The style as runners built before effects decode it.
        #[derive(Deserialize)]
        struct OldStyle {
            color: Color,
            at: (u8, u8),
            size: u16,
        }
        #[derive(Deserialize)]
        #[allow(dead_code)]
        struct OldNotice {
            schema: u8,
            seq: u64,
            kind: Kind,
            not_after: u64,
            built_before: Option<String>,
            to: Vec<Vec<u8>>,
            titles: Vec<u32>,
            style: Option<OldStyle>,
        }
        let mut n = notice(Kind::Text("hi".into()), 1, vec![]);
        n.style = Some(TextStyle {
            color: Color::Rgb([1, 2, 3]), at: (10, 20), size: 150, motion: Motion::Marquee, spin: Spin::Tumble,
        });
        let body = sign(&test_keypair(), &n).body;
        let style = postcard::from_bytes::<OldNotice>(&body).unwrap().style.unwrap();
        assert_eq!((style.color, style.at, style.size), (Color::Rgb([1, 2, 3]), (10, 20), 150));
        // The other way, a styled notice from a panel built before effects ends
        // two bytes early: a new runner refuses it rather than misreading it.
        let short = body[..body.len() - 2].to_vec();
        let sig = test_keypair().sign(&signed_message(Notice::CONTEXT, &short)).unwrap();
        assert_eq!(open_notice(&test_public(), &Signed { body: short, sig }, NOW), Err(Reject::Malformed));
    }

    #[test]
    fn the_compiled_key_is_a_real_ed25519_key() {
        // Catches a bad paste of keygen's line before a release ships it.
        assert_ne!(NOTICE_PUBLIC_KEY, [0; 32]);
        assert!(ed25519::PublicKey::try_from_bytes(&NOTICE_PUBLIC_KEY).is_ok());
    }
}

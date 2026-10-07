//! The developer's notice panel (docs/technical/rexnet-notices.md).

use std::collections::{hash_map::Entry, BTreeMap, HashMap, HashSet};
use std::io::{BufRead, Write};
use std::path::{Path, PathBuf};
use std::time::Duration;

use libp2p::identity::Keypair;
use libp2p::PeerId;
use rexnet_core::engine::{self, Command, EngineConfig, Event};
use rexnet_core::notice::{self, Color, Kind, Motion, Notice, Spin, TextStyle, NOTICE_PUBLIC_KEY};
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use tokio::sync::mpsc;

const USAGE: &str = "usage:
  rexnet-notice keygen --out <file outside any git repository>
  rexnet-notice roster --key <file> --title <hex>... [--wait 60s] [--cache <file>]
  rexnet-notice live --key <file> --title <hex>... [--cache <file>]   (JSON lines on stdin and stdout)
  rexnet-notice send dvd|clear|text \"<message>\"|magnetic --key <file>
                (--title <hex>... | --to <peer id or gamertag>... [--friends <friends.txt>])
                [--below <build code or date>] [--for 10m]
                [--color rrggbb|cycle] [--at X,Y] [--size 1-1000]   (text only)
                [--motion still|bounce|marquee] [--spin none|flat|3d|seesaw|wobble|tumble]   (text only)
                [--strength 0-100]   (magnetic only; default 1, singleplayer)
  rexnet-notice stamp [<build code or date>]
  A broadcast shows only in the --title titles it is sent to.
  --below shows it only on runners built before that build. A build code is the
  5 characters F4 shows (08JSV); a date is your local time, YYYY-MM-DD or
  YYYY-MM-DDTHH:MM. stamp turns a code into its date and a date into its code,
  and with no argument prints the code for now.
  rexnet-notice daemon --key <file> --title <hex>... [--cache <file>]
                (live over the local pipes \\\\.\\pipe\\rexnet-notice-events and -commands; Windows)";

/// How often a running panel seeds titles (and `send` retries targets).
const RESEED: Duration = Duration::from_secs(300);
/// How often a roster scans the DHT for runners not yet listed. A runner is
/// also asked the moment it connects.
const SCAN: Duration = Duration::from_secs(5);
/// How often a roster re-asks every listed runner, for changed names.
const REFRESH: Duration = Duration::from_secs(30);
/// How often `live` resends a targeted notice that has not been delivered.
const RETRY: Duration = Duration::from_secs(30);
/// Remembered players older than this are not dialled.
const CACHE_DAYS: u64 = 14;
const CACHE_MAX: usize = 500;

#[derive(Debug, Default, PartialEq)]
struct Args {
    command: String,
    what: Vec<String>,
    key: Option<PathBuf>,
    out: Option<PathBuf>,
    titles: Vec<u32>,
    to: Vec<String>,
    friends: Option<PathBuf>,
    below: Option<String>,
    lifetime: Option<Duration>,
    wait: Option<Duration>,
    color: Option<Color>,
    at: Option<(u8, u8)>,
    motion: Option<Motion>,
    spin: Option<Spin>,
    size: Option<u16>,
    strength: Option<u16>,
    cache: Option<PathBuf>,
}

fn parse_args(args: &[String]) -> Result<Args, String> {
    let mut it = args.iter();
    let mut parsed = Args { command: it.next().ok_or_else(|| USAGE.to_string())?.clone(), ..Args::default() };
    while let Some(arg) = it.next() {
        let mut value = || it.next().cloned().ok_or_else(|| format!("{arg} needs a value"));
        match arg.as_str() {
            "--key" => parsed.key = Some(value()?.into()),
            "--out" => parsed.out = Some(value()?.into()),
            "--title" => parsed.titles.push(parse_title(&value()?)?),
            "--to" => parsed.to.push(value()?),
            "--friends" => parsed.friends = Some(value()?.into()),
            "--below" => parsed.below = Some(build_of(&value()?, &local_offset).map_err(|e| format!("--below {e}"))?),
            "--for" => parsed.lifetime = Some(parse_duration(&value()?)?),
            "--wait" => parsed.wait = Some(parse_duration(&value()?)?),
            "--color" => parsed.color = Some(parse_color(&value()?)?),
            "--at" => parsed.at = Some(parse_at(&value()?)?),
            "--motion" => parsed.motion = Some(parse_motion(&value()?)?),
            "--spin" => parsed.spin = Some(parse_spin(&value()?)?),
            "--size" => parsed.size = Some(parse_size(&value()?)?),
            "--strength" => parsed.strength = Some(parse_strength(&value()?)?),
            "--cache" => parsed.cache = Some(value()?.into()),
            flag if flag.starts_with("--") => return Err(format!("unknown option {flag}\n{USAGE}")),
            _ => parsed.what.push(arg.clone()),
        }
    }
    Ok(parsed)
}

fn parse_title(text: &str) -> Result<u32, String> {
    u32::from_str_radix(text.trim_start_matches("0x"), 16).map_err(|_| format!("--title {text}: expected 8 hex digits"))
}

fn parse_strength(text: &str) -> Result<u16, String> {
    text.parse::<u16>().ok().filter(|n| *n <= 100)
        .ok_or_else(|| "--strength must be a whole number from 0 to 100".into())
}

/// `rrggbb` or `cycle`.
fn parse_color(text: &str) -> Result<Color, String> {
    if text == "cycle" {
        return Ok(Color::Cycle);
    }
    if text.len() != 6 || !text.bytes().all(|b| b.is_ascii_hexdigit()) {
        return Err(format!("--color {text}: expected rrggbb (6 hex digits) or cycle"));
    }
    let rgb = u32::from_str_radix(text, 16).expect("6 hex digits");
    Ok(Color::Rgb([(rgb >> 16) as u8, (rgb >> 8) as u8, rgb as u8]))
}

/// `X,Y`, each a percent of the window.
fn parse_at(text: &str) -> Result<(u8, u8), String> {
    let bad = || format!("--at {text}: expected X,Y, each a whole number from 0 to 100");
    let percent = |part: &str| part.parse::<u8>().ok().filter(|p| *p <= 100).ok_or_else(bad);
    let (x, y) = text.split_once(',').ok_or_else(bad)?;
    Ok((percent(x)?, percent(y)?))
}

fn parse_size(text: &str) -> Result<u16, String> {
    text.parse::<u16>()
        .ok()
        .filter(|size| (notice::SIZE_MIN..=notice::SIZE_MAX).contains(size))
        .ok_or_else(|| format!("--size {text}: expected a whole number from {} to {}", notice::SIZE_MIN, notice::SIZE_MAX))
}

/// still, bounce or marquee.
fn parse_motion(text: &str) -> Result<Motion, String> {
    match text {
        "still" => Ok(Motion::Still),
        "bounce" => Ok(Motion::Bounce),
        "marquee" => Ok(Motion::Marquee),
        _ => Err(format!("--motion {text}: expected still, bounce or marquee")),
    }
}

/// none, flat, 3d, seesaw, wobble or tumble.
fn parse_spin(text: &str) -> Result<Spin, String> {
    match text {
        "none" => Ok(Spin::None),
        "flat" => Ok(Spin::Flat),
        "3d" => Ok(Spin::Spin3d),
        "seesaw" => Ok(Spin::SeeSaw),
        "wobble" => Ok(Spin::Wobble),
        "tumble" => Ok(Spin::Tumble),
        _ => Err(format!("--spin {text}: expected none, flat, 3d, seesaw, wobble or tumble")),
    }
}

/// A whole number then s, m, h or d.
fn parse_duration(text: &str) -> Result<Duration, String> {
    let bad = || format!("{text}: expected a whole number then s, m, h or d (e.g. 10m)");
    let unit = text.chars().last().ok_or_else(bad)?;
    let number: u64 = text[..text.len() - unit.len_utf8()].parse().map_err(|_| bad())?;
    let seconds = match unit {
        's' => 1,
        'm' => 60,
        'h' => 3600,
        'd' => 86_400,
        _ => return Err(bad()),
    };
    Ok(Duration::from_secs(number * seconds))
}

/// Seconds since 1970 for a civil date and time read as UTC; None for one that does not exist.
fn civil_to_unix(year: i64, month: u32, day: u32, hour: u32, minute: u32) -> Option<i64> {
    let y = year - i64::from(month <= 2);
    let era = y.div_euclid(400);
    let yoe = y - era * 400;
    let doy = (153 * i64::from(if month > 2 { month - 3 } else { month + 9 }) + 2) / 5 + i64::from(day) - 1;
    let days = era * 146_097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719_468;
    let secs = days * 86_400 + i64::from(hour) * 3600 + i64::from(minute) * 60;
    // Out-of-range fields do not survive the trip back.
    (unix_to_civil(secs) == (year, month, day, hour, minute)).then_some(secs)
}

/// The civil year, month, day, hour and minute of Unix seconds read as UTC.
fn unix_to_civil(secs: i64) -> (i64, u32, u32, u32, u32) {
    let (days, rest) = (secs.div_euclid(86_400), secs.rem_euclid(86_400));
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = (doy - (153 * mp + 2) / 5 + 1) as u32;
    let month = (if mp < 10 { mp + 3 } else { mp - 9 }) as u32;
    let year = yoe + era * 400 + i64::from(month <= 2);
    (year, month, day, (rest / 3600) as u32, (rest % 3600 / 60) as u32)
}

/// Seconds to add to Unix time `utc` for this machine's local time, daylight
/// saving included.
#[cfg(windows)]
fn local_offset(utc: i64) -> i64 {
    #[repr(C)]
    #[derive(Default)]
    struct SystemTime {
        year: u16,
        month: u16,
        day_of_week: u16,
        day: u16,
        hour: u16,
        minute: u16,
        second: u16,
        milliseconds: u16,
    }
    #[link(name = "kernel32")]
    extern "system" {
        fn SystemTimeToTzSpecificLocalTime(zone: *const std::ffi::c_void, utc: *const SystemTime, local: *mut SystemTime) -> i32;
    }
    let (year, month, day, hour, minute) = unix_to_civil(utc);
    let from = SystemTime { year: year as u16, month: month as u16, day: day as u16, hour: hour as u16, minute: minute as u16, ..Default::default() };
    let mut local = SystemTime::default();
    // SAFETY: both pointers are to live SystemTime values; a null zone is this machine's.
    if unsafe { SystemTimeToTzSpecificLocalTime(std::ptr::null(), &from, &mut local) } == 0 {
        return 0;
    }
    civil_to_unix(local.year.into(), local.month.into(), local.day.into(), local.hour.into(), local.minute.into())
        .map_or(0, |local| local - (utc - utc.rem_euclid(60)))
}

/// Off Windows local time is taken to be UTC.
#[cfg(not(windows))]
fn local_offset(_utc: i64) -> i64 {
    0
}

/// Unix time of local `local` (its civil time read as UTC), given the zone's offset from UTC at any instant.
fn local_to_utc(local: i64, offset: &impl Fn(i64) -> i64) -> i64 {
    // Right except within hours of a clock change, where the second pass settles it.
    local - offset(local - offset(local))
}

/// `YYYY-MM-DD` (midnight) or `YYYY-MM-DDTHH:MM`, as Unix seconds read as UTC.
fn parse_local(text: &str) -> Option<i64> {
    let (date, time) = text.split_once('T').unwrap_or((text, "00:00"));
    let (year, rest) = date.split_once('-')?;
    let (month, day) = rest.split_once('-')?;
    let (hour, minute) = time.split_once(':')?;
    let number = |part: &str, digits: usize| -> Option<u32> {
        if part.len() == digits && part.bytes().all(|b| b.is_ascii_digit()) { part.parse().ok() } else { None }
    };
    civil_to_unix(number(year, 4)?.into(), number(month, 2)?, number(day, 2)?, number(hour, 2)?, number(minute, 2)?)
}

const OUT_OF_RANGE: &str = "outside the build code range (2026-01-01 UTC to the year 2141)";

/// Unix seconds of a build code, or of a local date and time in the zone `offset` describes.
fn when(text: &str, offset: &impl Fn(i64) -> i64) -> Result<u64, String> {
    if let Some(minutes) = notice::build_code_minutes(text) {
        return Ok(notice::BUILD_EPOCH_UNIX + minutes * 60);
    }
    let local = parse_local(text)
        .ok_or_else(|| format!("{text}: expected a build code like 08JSV, or a local date as YYYY-MM-DD or YYYY-MM-DDTHH:MM"))?;
    Ok(u64::try_from(local_to_utc(local, offset)).unwrap_or(0))
}

/// The build code for a build code or a local date and time.
fn build_of(text: &str, offset: &impl Fn(i64) -> i64) -> Result<String, String> {
    notice::build_code(when(text, offset)?).ok_or_else(|| format!("{text}: {OUT_OF_RANGE}"))
}

/// What `stamp` prints: the build code with its local and UTC times, for a
/// build code, a local date and time, or (None) now.
fn stamp_line(what: Option<&str>, now: u64, offset: &impl Fn(i64) -> i64) -> Result<String, String> {
    let unix = match what {
        Some(text) => when(text, offset)?,
        None => now,
    };
    let code = notice::build_code(unix).ok_or_else(|| format!("{}: {OUT_OF_RANGE}", what.unwrap_or("now")))?;
    let at = |secs: i64| {
        let (year, month, day, hour, minute) = unix_to_civil(secs);
        format!("{year:04}-{month:02}-{day:02} {hour:02}:{minute:02}")
    };
    let utc = unix as i64;
    Ok(format!("{code}  {} local  {} UTC", at(utc + offset(utc)), at(utc)))
}

/// A peer id as given, or a gamertag looked up in the developer's friends.txt.
fn resolve_target(target: &str, friends: Option<&Path>) -> Result<PeerId, String> {
    if let Ok(peer) = target.parse::<PeerId>() {
        return Ok(peer);
    }
    let Some(path) = friends else {
        return Err(format!("--to {target}: not a peer id; pass --friends <friends.txt> to look up gamertags"));
    };
    let dir = if path.is_dir() { path } else { path.parent().unwrap_or(Path::new(".")) };
    let store = rexnet_core::friends::FriendStore::load(dir);
    let known: Vec<(PeerId, String)> = store.entries().map(|(peer, entry)| (*peer, entry.name.clone())).collect();
    let matches: Vec<&(PeerId, String)> =
        known.iter().filter(|(_, name)| name.eq_ignore_ascii_case(target.trim())).collect();
    let list = |rows: &[&(PeerId, String)]| rows.iter().map(|(peer, name)| format!("  {peer}  {name}")).collect::<Vec<_>>().join("\n");
    match matches.as_slice() {
        [(peer, _)] => Ok(*peer),
        [] => Err(format!("--to {target}: no such gamertag in {}; known:\n{}", path.display(), list(&known.iter().collect::<Vec<_>>()))),
        many => Err(format!("--to {target}: ambiguous, use a peer id:\n{}", list(many))),
    }
}

/// Runners verify against the key compiled into rexnet-core; a notice signed
/// with any other key would be sent and then refused everywhere.
fn check_key(key: &Keypair) -> Result<(), String> {
    match notice::public_bytes(key) {
        Some(public) if public == NOTICE_PUBLIC_KEY => Ok(()),
        _ => Err("this key is not NOTICE_PUBLIC_KEY in thirdparty/rexnet-core/src/notice.rs; \
                  paste the line keygen printed there and rebuild runners and this panel".into()),
    }
}

/// A private key must never land somewhere git could pick it up.
fn inside_git_repo(path: &Path) -> bool {
    let absolute = if path.is_absolute() {
        path.to_path_buf()
    } else {
        std::env::current_dir().unwrap_or_default().join(path)
    };
    absolute.ancestors().skip(1).any(|dir| dir.join(".git").exists())
}

#[tokio::main]
async fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    let filter = || tracing_subscriber::EnvFilter::try_from_default_env().unwrap_or_else(|_| "warn".into());
    // The daemon has no console to write to. Everything else logs to stderr,
    // so stdout carries only rows (roster) or JSON lines (live).
    match args.first().filter(|command| *command == "daemon").and_then(|_| LogFile::open(daemon_file("log")).ok()) {
        Some(log) => tracing_subscriber::fmt().with_env_filter(filter()).with_ansi(false).with_writer(std::sync::Mutex::new(log)).init(),
        None => tracing_subscriber::fmt().with_env_filter(filter()).with_writer(std::io::stderr).init(),
    }
    let result = match parse_args(&args) {
        Ok(args) => match args.command.as_str() {
            "keygen" => keygen(args.out.as_deref()),
            "roster" => roster(&args).await,
            "live" => live(&args).await,
            "daemon" => daemon(&args).await,
            "send" => send(&args).await,
            "stamp" => stamp(&args),
            _ => Err(USAGE.into()),
        },
        Err(err) => Err(err),
    };
    if let Err(err) = result {
        eprintln!("rexnet-notice: {err}");
        std::process::exit(2);
    }
}

fn stamp(args: &Args) -> Result<(), String> {
    let what = match args.what.as_slice() {
        [] => None,
        [one] => Some(one.as_str()),
        _ => return Err(USAGE.into()),
    };
    println!("{}", stamp_line(what, rexnet_core::mailbox::unix_now(), &local_offset)?);
    Ok(())
}

fn keygen(out: Option<&Path>) -> Result<(), String> {
    let out = out.ok_or("keygen needs --out <file>")?;
    if inside_git_repo(out) {
        return Err(format!("{} is inside a git repository; keep the private key outside it", out.display()));
    }
    let key = Keypair::generate_ed25519();
    let bytes = key.to_protobuf_encoding().map_err(|e| e.to_string())?;
    let mut file = std::fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(out)
        .map_err(|e| format!("{}: {e} (keygen never overwrites a key)", out.display()))?;
    std::io::Write::write_all(&mut file, &bytes).map_err(|e| e.to_string())?;
    let public = notice::public_bytes(&key).expect("an ed25519 key");
    let bytes = public.iter().map(|b| format!("0x{b:02x}")).collect::<Vec<_>>().join(", ");
    println!("Private key written to {}. Keep it private and out of the repository.", out.display());
    println!("Replace NOTICE_PUBLIC_KEY in thirdparty/rexnet-core/src/notice.rs with:");
    println!("pub const NOTICE_PUBLIC_KEY: [u8; 32] = [{bytes}];");
    Ok(())
}

fn load_key(path: Option<&Path>) -> Result<Keypair, String> {
    let path = path.ok_or("--key <file> is required")?;
    let bytes = std::fs::read(path).map_err(|e| format!("{}: {e}", path.display()))?;
    let key = Keypair::from_protobuf_encoding(&bytes).map_err(|e| format!("{}: {e}", path.display()))?;
    check_key(&key)?;
    Ok(key)
}

/// An engine that never joins a shard, so the panel is never counted as a
/// player. `roster`, `live` and `send` give it a throwaway network identity,
/// so they can run beside the daemon; the daemon keeps one
/// (`daemon_file("identity")`).
async fn start_engine(identity: Keypair) -> Result<engine::EngineHandles, String> {
    let config = EngineConfig {
        data_dir: std::env::temp_dir().join(format!("rexnet-notice-{}", std::process::id())),
        title_id: 0,
        display_name: "rexnet-notice".into(),
        bootstrap: rexnet_core::AMINO_BOOTSTRAP.iter().map(|s| s.to_string()).collect(),
        listen_port: 0,
        game_port: 0,
        relays: Vec::new(),
        // No game traffic, so no UPnP mapping for a game port.
        force_tunnel: true,
        notice_key: NOTICE_PUBLIC_KEY,
    };
    engine::spawn(identity, config).await.map_err(|e| e.to_string())
}

async fn roster(args: &Args) -> Result<(), String> {
    let key = load_key(args.key.as_deref())?;
    if args.titles.is_empty() {
        return Err("roster needs --title <hex>".into());
    }
    let mut engine = start_engine(Keypair::generate_ed25519()).await?;
    let cache = cache_path(args.cache.as_deref());
    let mut players = load_players(&cache, rexnet_core::mailbox::unix_now());
    dial_players(&engine, &players);
    let deadline = tokio::time::Instant::now() + args.wait.unwrap_or(Duration::from_secs(60));
    let (mut scans, mut refreshes) = roster_timers();
    let mut seen: HashMap<PeerId, bool> = HashMap::new(); // peer -> on an old build
    println!("{:<10} {:<54} GAMERTAG", "TITLE", "PEER ID");
    loop {
        tokio::select! {
            _ = tokio::time::sleep_until(deadline) => break,
            _ = scans.tick() => roster_round(&engine, &key, &args.titles, false),
            _ = refreshes.tick() => roster_round(&engine, &key, &args.titles, true),
            event = engine.evt_rx.recv() => {
                let Some(event) = event else { return Err("engine stopped".into()) };
                if !for_session(&event, &args.titles) {
                    continue;
                }
                remember_runner(&cache, &mut players, &event);
                match event {
                    // An answer wins over an old-build row for the same peer.
                    Event::RosterEntry { peer, title_id, display_name, .. } => {
                        if seen.insert(peer, false) != Some(false) {
                            println!("{}", roster_row(title_id, &peer, &display_name));
                        }
                    }
                    Event::RosterOldBuild { peer, title_id } => {
                        if let Entry::Vacant(slot) = seen.entry(peer) {
                            slot.insert(true);
                            println!("{}", roster_row(title_id, &peer, OLD_BUILD));
                        }
                    }
                    _ => {}
                }
            }
        }
    }
    let _ = engine.cmd_tx.send(Command::Shutdown);
    let old = seen.values().filter(|old| **old).count();
    println!("{}", roster_summary(seen.len() - old, old));
    Ok(())
}

/// A roster scan, or with `refresh` a re-ask of every listed runner: one ask
/// for every title, signed afresh because runners refuse asks over 5 minutes
/// old. The engine also asks each runner that connects until the next one.
fn roster_round(engine: &engine::EngineHandles, key: &Keypair, titles: &[u32], refresh: bool) {
    let ask = notice::sign(key, &notice::Ask { schema: 1, panel_peer: engine.peer_id.to_bytes(), issued_ms: notice::unix_ms() });
    for &title_id in titles {
        let _ = engine.cmd_tx.send(Command::Roster { title_id, ask: ask.clone(), refresh });
    }
}

/// The scan timer, which fires at once, and the refresh timer, which first
/// fires a refresh period in: the first scan finds everyone anyway.
fn roster_timers() -> (tokio::time::Interval, tokio::time::Interval) {
    (skipping(tokio::time::interval(SCAN)), skipping(tokio::time::interval_at(tokio::time::Instant::now() + REFRESH, REFRESH)))
}

/// After a sleep, `timer` fires once rather than once per period missed.
fn skipping(mut timer: tokio::time::Interval) -> tokio::time::Interval {
    timer.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
    timer
}

/// A runner that answered a roster, kept so the next panel dials it at once
/// instead of waiting for the DHT. All of it is public: no key material.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
struct Player {
    peer: String,
    name: String,
    title: String,
    addr: Option<String>,
    /// Unix seconds.
    seen: u64,
}

/// `--cache`, else `%APPDATA%\xrecomp\rexnet-notice-peers.json`, with the
/// home directory standing in for `%APPDATA%` where it is not set.
fn cache_path(arg: Option<&Path>) -> PathBuf {
    if let Some(path) = arg {
        return path.into();
    }
    let base = ["APPDATA", "USERPROFILE", "HOME"].iter().find_map(std::env::var_os).unwrap_or_default();
    PathBuf::from(base).join("xrecomp").join("rexnet-notice-peers.json")
}

/// The players worth dialling. A missing or damaged file is none.
fn load_players(path: &Path, now: u64) -> Vec<Player> {
    let players = std::fs::read(path).ok().and_then(|bytes| serde_json::from_slice(&bytes).ok()).unwrap_or_default();
    prune(players, now)
}

/// Drop players with no address or not seen for 14 days; keep the newest 500.
fn prune(mut players: Vec<Player>, now: u64) -> Vec<Player> {
    players.retain(|p| p.addr.is_some() && now.saturating_sub(p.seen) <= CACHE_DAYS * 86_400);
    players.sort_by(|a, b| b.seen.cmp(&a.seen));
    players.truncate(CACHE_MAX);
    players
}

/// `player` replaces any entry for its peer.
fn remember(players: &mut Vec<Player>, player: Player, now: u64) {
    players.retain(|p| p.peer != player.peer);
    players.push(player);
    *players = prune(std::mem::take(players), now);
}

/// Written beside `path` and renamed over it, so a crash never leaves half a file.
fn save_players(path: &Path, players: &[Player]) -> std::io::Result<()> {
    if let Some(dir) = path.parent() {
        std::fs::create_dir_all(dir)?;
    }
    let temp = path.with_extension("json.tmp");
    std::fs::write(&temp, serde_json::to_vec_pretty(players)?)?;
    std::fs::rename(&temp, path)
}

/// Dial every remembered player now: a direct or relay address needs no DHT,
/// and the engine asks each one as it identifies. Many have gone since, so a
/// failed dial is only logged at debug.
fn dial_players(engine: &engine::EngineHandles, players: &[Player]) {
    for addr in players.iter().filter_map(|p| p.addr.clone()) {
        let _ = engine.cmd_tx.send(Command::Dial { multiaddr: addr });
    }
}

/// Remember a runner that answered with an address, and save at once.
fn remember_runner(cache: &Path, players: &mut Vec<Player>, event: &Event) {
    let Event::RosterEntry { peer, title_id, display_name, addr: Some(addr) } = event else { return };
    let now = rexnet_core::mailbox::unix_now();
    let player = Player {
        peer: peer.to_base58(),
        name: display_name.clone(),
        title: format!("{title_id:08X}"),
        addr: Some(addr.clone()),
        seen: now,
    };
    remember(players, player, now);
    if let Err(err) = save_players(cache, players) {
        tracing::warn!(cache = %cache.display(), %err, "could not save remembered players");
    }
}

/// The roster row's gamertag column for a runner that cannot answer.
const OLD_BUILD: &str = "(old build)";

/// One roster row. The GUI parses this layout (tools/rexnet_notice_gui/cli.py).
fn roster_row(title_id: u32, peer: &PeerId, gamertag: &str) -> String {
    format!("{title_id:08X}   {:<54} {gamertag}", peer.to_base58())
}

fn roster_summary(answered: usize, old_build: usize) -> String {
    format!("{answered} runner(s) answered, {old_build} on an old build")
}

/// The notice `send` signs, from its options and resolved `--to` targets.
fn build_notice(args: &Args, targets: &[PeerId], now: u64) -> Result<Notice, String> {
    let kind = match args.what.as_slice() {
        [what] if what == "dvd" => Kind::Dvd,
        [what] if what == "clear" => Kind::Clear,
        [what] if what == "magnetic" => Kind::MagneticTraffic { strength: args.strength.unwrap_or(1) },
        [what, text] if what == "text" => {
            if text.trim().is_empty() {
                return Err("text is empty: give the message to show".into());
            }
            if text.chars().count() > notice::TEXT_MAX_CHARS || text.chars().any(char::is_control) {
                return Err(format!("text must be one line of at most {} characters", notice::TEXT_MAX_CHARS));
            }
            Kind::Text(text.clone())
        }
        _ => return Err(USAGE.into()),
    };
    let styled = args.color.is_some() || args.at.is_some() || args.size.is_some()
        || args.motion.is_some() || args.spin.is_some();
    if args.strength.is_some() && !matches!(kind, Kind::MagneticTraffic { .. }) {
        return Err("--strength applies only to magnetic traffic".into());
    }
    if styled && !matches!(kind, Kind::Text(_)) {
        return Err("--color, --at, --size, --motion and --spin apply only to text".into());
    }
    let style = styled.then(|| {
        let default = TextStyle::default();
        TextStyle {
            color: args.color.unwrap_or(default.color),
            at: args.at.unwrap_or(default.at),
            size: args.size.unwrap_or(default.size),
            motion: args.motion.unwrap_or(default.motion),
            spin: args.spin.unwrap_or(default.spin),
        }
    });
    let default = if kind == Kind::Clear { notice::MAX_LIFETIME_SECS } else { 600 };
    let lifetime = args.lifetime.map_or(default, |d| d.as_secs());
    if lifetime == 0 || lifetime > notice::MAX_LIFETIME_SECS {
        return Err("--for must be between 1s and 7d".into());
    }
    match (targets.is_empty(), args.titles.is_empty()) {
        (true, true) => return Err("send needs --title <hex> (everyone) or --to <player>".into()),
        (false, false) => return Err("use --title or --to, not both".into()),
        _ => {}
    }
    if args.titles.len() > notice::TITLES_MAX {
        return Err(format!("at most {} --title values", notice::TITLES_MAX));
    }
    Ok(Notice {
        schema: 1,
        seq: notice::unix_ms(),
        kind,
        not_after: now + lifetime,
        built_before: args.below.clone(),
        to: targets.iter().map(|peer| peer.to_bytes()).collect(),
        // Empty for a targeted notice: --title and --to never mix.
        titles: args.titles.clone(),
        style,
    })
}

/// The notice `args` describe, signed, with its resolved targets: every check
/// `send` and `live` make before anything goes out.
fn prepare(args: &Args, key: &Keypair, now: u64) -> Result<(Notice, notice::Signed, Vec<PeerId>), String> {
    let targets = args.to.iter().map(|t| resolve_target(t, args.friends.as_deref())).collect::<Result<Vec<_>, _>>()?;
    let notice = build_notice(args, &targets, now)?;
    let signed = notice::sign(key, &notice);
    // Runners refuse bodies over BODY_MAX; with many --to targets this is reachable.
    if signed.body.len() > notice::BODY_MAX {
        return Err(format!("notice is {} bytes, over the {}-byte limit: name fewer --to targets", signed.body.len(), notice::BODY_MAX));
    }
    Ok((notice, signed, targets))
}

async fn send(args: &Args) -> Result<(), String> {
    let key = load_key(args.key.as_deref())?;
    let now = rexnet_core::mailbox::unix_now();
    let (notice, signed, targets) = prepare(args, &key, now)?;
    let lifetime = notice.not_after - now;
    let mut engine = start_engine(Keypair::generate_ed25519()).await?;
    if targets.is_empty() {
        engine.cmd_tx.send(Command::PublishNotice { signed: signed.clone() }).map_err(|e| e.to_string())?;
    }
    let mut pending: HashSet<PeerId> = targets.iter().copied().collect();
    let end = tokio::time::Instant::now() + Duration::from_secs(lifetime);
    let mut round_at = tokio::time::Instant::now();
    println!("notice {} lasts {lifetime}s; Ctrl+C stops the panel (runners keep the notice until it ends)", notice.seq);
    loop {
        tokio::select! {
            _ = tokio::signal::ctrl_c() => break,
            _ = tokio::time::sleep_until(end) => break,
            _ = tokio::time::sleep_until(round_at) => {
                if !args.titles.is_empty() {
                    println!("seeding {} title(s)", args.titles.len());
                }
                for &title_id in &args.titles {
                    let _ = engine.cmd_tx.send(Command::SeedTitle { title_id });
                }
                for &peer in &pending {
                    let _ = engine.cmd_tx.send(Command::SendNoticeTo { peer, signed: signed.clone() });
                }
                round_at = tokio::time::Instant::now() + RESEED;
            }
            event = engine.evt_rx.recv() => match event {
                Some(Event::NoticeDelivered { peer }) => {
                    println!("delivered to {peer}");
                    if pending.remove(&peer) && pending.is_empty() && args.titles.is_empty() {
                        break;
                    }
                }
                Some(_) => {}
                None => return Err("engine stopped".into()),
            },
        }
    }
    let _ = engine.cmd_tx.send(Command::Shutdown);
    if !pending.is_empty() {
        println!("not delivered: {}", pending.iter().map(|p| p.to_string()).collect::<Vec<_>>().join(", "));
    }
    Ok(())
}

/// One line of `live`'s stdin.
#[derive(Debug, PartialEq)]
enum LiveCommand {
    Roster,
    /// The `send` arguments the command stands for.
    Send(Args),
    Stop,
    Quit,
}

/// A live `send`. Each field is the CLI option of the same name, checked the same way.
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct LiveSend {
    #[serde(rename = "cmd")]
    _cmd: String,
    kind: String,
    text: Option<String>,
    color: Option<String>,
    at: Option<(u8, u8)>,
    motion: Option<String>,
    spin: Option<String>,
    size: Option<u16>,
    strength: Option<u16>,
    #[serde(default)]
    to: Vec<String>,
    #[serde(rename = "for")]
    lifetime: Option<String>,
    below: Option<String>,
}

/// Parse one stdin line. A broadcast goes to the session's `titles`.
fn live_command(line: &str, titles: &[u32]) -> Result<LiveCommand, String> {
    let value: Value = serde_json::from_str(line).map_err(|e| format!("not a JSON command: {e}"))?;
    match value.get("cmd").and_then(Value::as_str) {
        Some("roster") => Ok(LiveCommand::Roster),
        Some("stop") => Ok(LiveCommand::Stop),
        Some("quit") => Ok(LiveCommand::Quit),
        Some("send") => {
            let send: LiveSend = serde_json::from_value(value).map_err(|e| format!("send: {e}"))?;
            // There is no --friends here to look a gamertag up in.
            if let Some(name) = send.to.iter().find(|to| to.parse::<PeerId>().is_err()) {
                return Err(format!("to {name}: peer ids only in live mode"));
            }
            Ok(LiveCommand::Send(Args {
                command: "send".into(),
                what: std::iter::once(send.kind).chain(send.text).collect(),
                titles: if send.to.is_empty() { titles.to_vec() } else { Vec::new() },
                to: send.to,
                below: send.below.map(|b| build_of(&b, &local_offset).map_err(|e| format!("--below {e}"))).transpose()?,
                lifetime: send.lifetime.as_deref().map(parse_duration).transpose()?,
                color: send.color.as_deref().map(parse_color).transpose()?,
                at: send.at.map(|(x, y)| parse_at(&format!("{x},{y}"))).transpose()?,
                motion: send.motion.as_deref().map(parse_motion).transpose()?,
                spin: send.spin.as_deref().map(parse_spin).transpose()?,
                size: send.size.map(|size| parse_size(&size.to_string())).transpose()?,
                strength: send.strength.map(|n| parse_strength(&n.to_string())).transpose()?,
                ..Args::default()
            }))
        }
        _ => Err(format!("unknown command: {}", line.trim())),
    }
}

/// False for a roster row of a title outside the session's `--title` list:
/// another panel answers as title 0, and mDNS reaches runners of other games.
/// An old-build row carries the asking round's title, so it always passes.
fn for_session(event: &Event, titles: &[u32]) -> bool {
    match event {
        Event::RosterEntry { title_id, .. } | Event::RosterOldBuild { title_id, .. } => titles.contains(title_id),
        _ => true,
    }
}

/// The `live` line for an engine event, or None for one the panel does not report.
fn event_json(event: &Event) -> Option<Value> {
    Some(match event {
        Event::RosterEntry { peer, title_id, display_name, .. } => {
            json!({"ev": "runner", "peer": peer.to_base58(), "title": format!("{title_id:08X}"), "name": display_name})
        }
        Event::RosterOldBuild { peer, title_id } => {
            json!({"ev": "old_build", "peer": peer.to_base58(), "title": format!("{title_id:08X}")})
        }
        Event::RosterGone { peer } => json!({"ev": "gone", "peer": peer.to_base58()}),
        Event::NoticeDelivered { peer } => json!({"ev": "delivered", "peer": peer.to_base58()}),
        _ => return None,
    })
}

/// Where a session's events go: writes one JSON line, and says whether the
/// client is still there to read it.
type Sink = Box<dyn FnMut(&str) -> bool + Send>;

/// A notice this session is still sending out.
#[derive(Debug, Clone)]
struct Active {
    seq: u64,
    kind: String,
    /// Unix seconds.
    ends: u64,
}

/// What `live` and `daemon` know: who is online and what is still going out,
/// so a client that attaches late is told it at once.
struct Session {
    me: PeerId,
    /// The runner or old_build line of each peer listed and not gone, by peer id.
    listed: BTreeMap<String, Value>,
    /// The latest broadcast, re-seeded until it ends.
    broadcast: Option<Active>,
    /// Targeted notices not yet delivered, by target.
    pending: HashMap<PeerId, (notice::Signed, Active)>,
    /// The one attached client.
    client: Option<Sink>,
}

impl Session {
    fn new(me: PeerId) -> Self {
        Session { me, listed: BTreeMap::new(), broadcast: None, pending: HashMap::new(), client: None }
    }

    /// One line to the attached client. A client that has gone is dropped.
    fn emit(&mut self, line: &Value) {
        if let Some(client) = &mut self.client {
            if !client(&line.to_string()) {
                self.client = None;
            }
        }
    }

    /// `client` replaces any other, and hears the snapshot before anything else.
    fn attach(&mut self, client: Sink, now: u64) {
        self.client = Some(client);
        for line in self.snapshot(now) {
            self.emit(&line);
        }
    }

    /// `ready`, the line of each runner listed, then an `active` line for the
    /// broadcast and for each targeted notice with targets still to deliver to.
    fn snapshot(&self, now: u64) -> Vec<Value> {
        let mut lines = vec![json!({"ev": "ready", "peer": self.me.to_base58()})];
        lines.extend(self.listed.values().cloned());
        if let Some(active) = self.broadcast.as_ref().filter(|active| active.ends > now) {
            lines.push(json!({"ev": "active", "seq": active.seq, "kind": active.kind, "ends": active.ends}));
        }
        let mut targeted: BTreeMap<u64, (&Active, Vec<String>)> = BTreeMap::new();
        for (peer, (_, active)) in self.pending.iter().filter(|(_, (_, active))| active.ends > now) {
            targeted.entry(active.seq).or_insert((active, Vec::new())).1.push(peer.to_base58());
        }
        for (active, mut pending) in targeted.into_values() {
            pending.sort();
            lines.push(json!({"ev": "active", "seq": active.seq, "kind": active.kind, "ends": active.ends, "pending": pending}));
        }
        lines
    }

    /// Note what an engine event changes, and return its line if the panel reports it.
    fn event(&mut self, event: &Event) -> Option<Value> {
        let line = event_json(event)?;
        match event {
            Event::NoticeDelivered { peer } => {
                self.pending.remove(peer);
            }
            Event::RosterEntry { peer, .. } | Event::RosterOldBuild { peer, .. } => {
                self.listed.insert(peer.to_base58(), line.clone());
            }
            // A peer this session never listed leaves silently.
            Event::RosterGone { peer } => {
                self.listed.remove(&peer.to_base58())?;
            }
            _ => {}
        }
        Some(line)
    }
}

/// One command line, or None for a blank one. A line that is not UTF-8 is a
/// bad command like any other: the session goes on.
fn read_command(line: &[u8], titles: &[u32]) -> Option<Result<LiveCommand, String>> {
    match std::str::from_utf8(line) {
        Err(_) => Some(Err("a command line must be UTF-8".into())),
        Ok(text) if text.trim().is_empty() => None,
        Ok(text) => Some(live_command(text, titles)),
    }
}

/// stdin's lines, read on a thread of their own; the channel closes with stdin.
fn stdin_lines() -> mpsc::UnboundedReceiver<Vec<u8>> {
    let (tx, rx) = mpsc::unbounded_channel();
    std::thread::spawn(move || {
        for line in std::io::stdin().lock().split(b'\n') {
            let Ok(line) = line else { break };
            if tx.send(line).is_err() {
                break;
            }
        }
    });
    rx
}

/// A panel that stays up for the GUI, speaking JSON lines on stdin and stdout
/// (see the docs).
async fn live(args: &Args) -> Result<(), String> {
    let key = load_key(args.key.as_deref())?;
    if args.titles.is_empty() {
        return Err("live needs --title <hex>".into());
    }
    let engine = start_engine(Keypair::generate_ed25519()).await?;
    let (attach, clients) = mpsc::unbounded_channel::<Sink>();
    // Flushed, so the GUI sees each line at once.
    let _ = attach.send(Box::new(|line: &str| {
        let mut out = std::io::stdout().lock();
        writeln!(out, "{line}").and_then(|()| out.flush()).is_ok()
    }));
    serve(args, &key, engine, stdin_lines(), clients).await
}

/// `live` kept warm in the background: it beacons its titles so runners dial
/// it, and a client attaches over local named pipes (see the docs).
#[cfg(windows)]
async fn daemon(args: &Args) -> Result<(), String> {
    let key = load_key(args.key.as_deref())?;
    if args.titles.is_empty() {
        return Err("daemon needs --title <hex>".into());
    }
    // Before the engine, so a second daemon fails at once.
    let (events, commands) = pipes::first()?;
    // One peer id across restarts: a fresh one each start left a stale beacon
    // provider record (kept up to 48 h) for every runner to dial. This is the
    // panel's network identity, not the notice signing key.
    let path = daemon_file("identity");
    let identity = rexnet_core::identity::load_or_generate_file(&path).map_err(|e| format!("{}: {e}", path.display()))?;
    let engine = start_engine(identity).await?;
    for &title_id in &args.titles {
        let _ = engine.cmd_tx.send(Command::Beacon { title_id });
    }
    let (attach, clients) = mpsc::unbounded_channel();
    let (lines_tx, lines) = mpsc::unbounded_channel();
    tokio::spawn(pipes::serve_events(events, attach));
    tokio::spawn(pipes::serve_commands(commands, lines_tx));
    tracing::info!(peer = %engine.peer_id, titles = ?args.titles, "daemon up");
    let result = serve(args, &key, engine, lines, clients).await;
    tracing::info!("daemon down");
    result
}

#[cfg(not(windows))]
async fn daemon(_args: &Args) -> Result<(), String> {
    Err("daemon mode needs Windows named pipes".into())
}

/// What `live` and `daemon` share: the engine, its timers and the session.
/// Commands arrive on `lines` and run whether or not a client is attached;
/// events go to the latest client from `clients`. Ends on `quit`, or when
/// `lines` closes.
async fn serve(
    args: &Args,
    key: &Keypair,
    mut engine: engine::EngineHandles,
    mut lines: mpsc::UnboundedReceiver<Vec<u8>>,
    mut clients: mpsc::UnboundedReceiver<Sink>,
) -> Result<(), String> {
    let cache = cache_path(args.cache.as_deref());
    let mut players = load_players(&cache, rexnet_core::mailbox::unix_now());
    dial_players(&engine, &players);
    let (mut scans, mut refreshes) = roster_timers();
    let (mut reseed, mut retry) = (skipping(tokio::time::interval(RESEED)), skipping(tokio::time::interval(RETRY)));
    let mut session = Session::new(engine.peer_id);
    loop {
        tokio::select! {
            // A client that attaches hears the snapshot before anything else.
            biased;
            Some(client) = clients.recv() => session.attach(client, rexnet_core::mailbox::unix_now()),
            _ = scans.tick() => roster_round(&engine, key, &args.titles, false),
            _ = refreshes.tick() => roster_round(&engine, key, &args.titles, true),
            _ = reseed.tick() => {
                if session.broadcast.as_ref().is_some_and(|active| active.ends > rexnet_core::mailbox::unix_now()) {
                    for &title_id in &args.titles {
                        let _ = engine.cmd_tx.send(Command::SeedTitle { title_id });
                    }
                }
            }
            _ = retry.tick() => {
                let now = rexnet_core::mailbox::unix_now();
                session.pending.retain(|_, (_, active)| active.ends > now);
                for (&peer, (signed, _)) in &session.pending {
                    let _ = engine.cmd_tx.send(Command::SendNoticeTo { peer, signed: signed.clone() });
                }
            }
            line = lines.recv() => {
                let Some(line) = line else { break };
                let Some(command) = read_command(&line, &args.titles) else { continue };
                match command {
                    Err(msg) => session.emit(&json!({"ev": "error", "msg": msg})),
                    Ok(LiveCommand::Quit) => break,
                    Ok(LiveCommand::Roster) => {
                        roster_round(&engine, key, &args.titles, true);
                        refreshes.reset();
                    }
                    Ok(LiveCommand::Stop) => {
                        session.broadcast = None;
                        session.pending.clear();
                    }
                    Ok(LiveCommand::Send(send)) => {
                        let now = rexnet_core::mailbox::unix_now();
                        let (notice, signed, targets) = match prepare(&send, key, now) {
                            Ok(prepared) => prepared,
                            Err(msg) => {
                                session.emit(&json!({"ev": "error", "msg": msg}));
                                continue;
                            }
                        };
                        let active = Active { seq: notice.seq, kind: send.what[0].clone(), ends: notice.not_after };
                        if targets.is_empty() {
                            let _ = engine.cmd_tx.send(Command::PublishNotice { signed: signed.clone() });
                            session.broadcast = Some(active.clone());
                            reseed.reset_immediately();
                        }
                        for peer in targets {
                            session.pending.insert(peer, (signed.clone(), active.clone()));
                            let _ = engine.cmd_tx.send(Command::SendNoticeTo { peer, signed: signed.clone() });
                        }
                        session.emit(&json!({"ev": "sent", "seq": notice.seq, "kind": send.what[0], "lasts": notice.not_after - now}));
                    }
                }
            }
            event = engine.evt_rx.recv() => {
                let Some(event) = event else { return Err("engine stopped".into()) };
                if !for_session(&event, &args.titles) {
                    continue;
                }
                remember_runner(&cache, &mut players, &event);
                if let Some(line) = session.event(&event) {
                    session.emit(&line);
                }
            }
        }
    }
    let _ = engine.cmd_tx.send(Command::Shutdown);
    Ok(())
}

/// The daemon's local named pipes (docs/technical/rexnet-notices.md): two, so
/// a client never reads and writes through one handle.
#[cfg(windows)]
mod pipes {
    use super::Sink;
    use std::time::Duration;
    use tokio::io::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt, BufReader};
    use tokio::net::windows::named_pipe::{NamedPipeServer, ServerOptions};
    use tokio::sync::mpsc::{self, UnboundedSender};

    pub const EVENTS: &str = r"\\.\pipe\rexnet-notice-events";
    pub const COMMANDS: &str = r"\\.\pipe\rexnet-notice-commands";

    /// An instance of `name` for local clients only. The default DACL gives
    /// the owner, LocalSystem and administrators full access and everyone else
    /// read: another local user can at most read events, never send a command.
    fn instance(name: &str, first: bool) -> std::io::Result<NamedPipeServer> {
        ServerOptions::new().first_pipe_instance(first).reject_remote_clients(true).create(name)
    }

    /// Both pipes' first instances; they exist only while a daemon does.
    pub fn first() -> Result<(NamedPipeServer, NamedPipeServer), String> {
        let open = |name: &str| instance(name, true).map_err(|err| format!("{name}: {err} (is a daemon already running?)"));
        Ok((open(EVENTS)?, open(COMMANDS)?))
    }

    /// The next instance of `name`, made before the last one is dropped so the
    /// pipe never disappears while the daemon runs.
    async fn next(name: &str) -> NamedPipeServer {
        loop {
            match instance(name, false) {
                Ok(server) => return server,
                Err(err) => {
                    tracing::warn!(name, %err, "could not reopen pipe; retrying");
                    tokio::time::sleep(Duration::from_secs(1)).await;
                }
            }
        }
    }

    /// One events client at a time. Each gets a sink on the session, whose
    /// lines this writes until the client goes. Nothing it sends is used: the
    /// read only notices it closing.
    pub async fn serve_events(mut server: NamedPipeServer, attach: UnboundedSender<Sink>) {
        loop {
            if server.connect().await.is_ok() {
                tracing::info!("events client attached");
                // ponytail: unbounded; a client that stops reading costs memory at a few lines a minute.
                let (tx, mut rx) = mpsc::unbounded_channel::<String>();
                if attach.send(Box::new(move |line: &str| tx.send(format!("{line}\n")).is_ok())).is_err() {
                    return;
                }
                let mut byte = [0u8; 1];
                loop {
                    tokio::select! {
                        line = rx.recv() => {
                            let Some(line) = line else { break };
                            if server.write_all(line.as_bytes()).await.is_err() {
                                break;
                            }
                        }
                        read = server.read(&mut byte) => if !matches!(read, Ok(n) if n > 0) { break },
                    }
                }
                tracing::info!("events client detached");
            }
            server = next(EVENTS).await;
        }
    }

    /// One commands client at a time; each line it writes goes to the session.
    pub async fn serve_commands(mut server: NamedPipeServer, lines: UnboundedSender<Vec<u8>>) {
        loop {
            if server.connect().await.is_ok() {
                let mut reader = BufReader::new(&mut server);
                let mut line = Vec::new();
                while matches!(reader.read_until(b'\n', &mut line).await, Ok(n) if n > 0) {
                    if lines.send(std::mem::take(&mut line)).is_err() {
                        return;
                    }
                }
            }
            server = next(COMMANDS).await;
        }
    }
}

/// `%LOCALAPPDATA%\xrecomp\rexnet-notice-daemon.<ext>`: the `log`, and the
/// network `identity`.
fn daemon_file(ext: &str) -> PathBuf {
    let base = std::env::var_os("LOCALAPPDATA").map_or_else(std::env::temp_dir, PathBuf::from);
    base.join("xrecomp").join(format!("rexnet-notice-daemon.{ext}"))
}

/// Past this the daemon's log moves to `.log.old`, replacing the last one.
const LOG_MAX: u64 = 1 << 20;

/// The daemon's log: appended across runs, and capped by moving it aside.
struct LogFile {
    path: PathBuf,
    /// None after a move whose reopen failed: nothing is written until an
    /// open succeeds, rather than into the moved file.
    file: Option<std::fs::File>,
    size: u64,
}

impl LogFile {
    fn open(path: PathBuf) -> std::io::Result<Self> {
        let mut log = LogFile { path, file: None, size: 0 };
        log.reopen()?;
        Ok(log)
    }

    fn reopen(&mut self) -> std::io::Result<()> {
        if let Some(dir) = self.path.parent() {
            std::fs::create_dir_all(dir)?;
        }
        let file = std::fs::OpenOptions::new().create(true).append(true).open(&self.path)?;
        self.size = file.metadata()?.len();
        self.file = Some(file);
        Ok(())
    }
}

impl Write for LogFile {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        if self.file.is_some() && self.size > 0 && self.size + buf.len() as u64 > LOG_MAX {
            // One retry, then empty the log in place rather than grow past the cap.
            if (0..2).any(|_| std::fs::rename(&self.path, self.path.with_extension("log.old")).is_ok()) {
                self.file = None;
            } else {
                // Appends follow the end of the file, so ours carry on at 0.
                std::fs::File::create(&self.path)?;
                self.size = 0;
            }
        }
        if self.file.is_none() {
            self.reopen()?;
        }
        let written = self.file.as_mut().expect("opened above").write(buf)?;
        self.size += written as u64;
        Ok(written)
    }

    fn flush(&mut self) -> std::io::Result<()> {
        self.file.as_mut().map_or(Ok(()), |file| file.flush())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn args(text: &[&str]) -> Result<Args, String> {
        parse_args(&text.iter().map(|s| s.to_string()).collect::<Vec<_>>())
    }

    fn peer() -> PeerId {
        Keypair::generate_ed25519().public().to_peer_id()
    }

    #[test]
    fn durations_take_a_unit() {
        assert_eq!(parse_duration("90s"), Ok(Duration::from_secs(90)));
        assert_eq!(parse_duration("10m"), Ok(Duration::from_secs(600)));
        assert_eq!(parse_duration("2h"), Ok(Duration::from_secs(7200)));
        assert_eq!(parse_duration("7d"), Ok(Duration::from_secs(604_800)));
        for bad in ["10", "m", "", "1.5h", "10é", "-1m"] {
            assert!(parse_duration(bad).is_err(), "{bad}");
        }
    }

    #[test]
    fn options_parse_and_bad_builds_or_titles_are_refused() {
        let parsed = args(&["send", "text", "New update out", "--title", "4D530064", "--below", "08jsv", "--for", "2h"]).unwrap();
        assert_eq!(parsed.what, ["text", "New update out"]);
        assert_eq!(parsed.titles, [0x4D53_0064]);
        assert_eq!(parsed.below.as_deref(), Some("08JSV"), "a code is stored upper case");
        assert_eq!(parsed.lifetime, Some(Duration::from_secs(7200)));
        let dated = args(&["send", "dvd", "--below", "2026-10-04T20:31"]).unwrap();
        assert_eq!(dated.below, Some(build_of("2026-10-04T20:31", &local_offset).unwrap()), "a date in this machine's zone");
        assert!(args(&["send", "dvd", "--below", "v0.1.5"]).unwrap_err().starts_with("--below v0.1.5:"));
        assert!(args(&["send", "dvd", "--below", "2026-02-30"]).is_err());
        assert!(args(&["send", "dvd", "--title", "MC3"]).is_err());
        assert!(args(&["send", "dvd", "--bogus"]).is_err());
        assert!(args(&["send", "dvd", "--key"]).is_err(), "missing value");
        assert!(args(&[]).is_err());
    }

    #[test]
    fn color_at_and_size_parse_and_bad_values_are_refused() {
        let parsed = args(&["send", "text", "hi", "--color", "FF8000", "--at", "10,90", "--size", "150"]).unwrap();
        assert_eq!((parsed.color, parsed.at, parsed.size), (Some(Color::Rgb([255, 128, 0])), Some((10, 90)), Some(150)));
        assert_eq!(parse_color("cycle"), Ok(Color::Cycle));
        assert_eq!(parse_at("0,100"), Ok((0, 100)));
        assert_eq!((parse_size("1"), parse_size("1000")), (Ok(1), Ok(1000)));
        for bad in ["fff", "ff80001", "gg0000", "+12345", "#ff800", "Cycle", ""] {
            assert!(parse_color(bad).is_err(), "{bad}");
        }
        for bad in ["50", "101,5", "5,101", "-1,5", "5,", "5,5,5", "a,b", ""] {
            assert!(parse_at(bad).is_err(), "{bad}");
        }
        for bad in ["0", "1001", "-100", "1.5", ""] {
            assert!(parse_size(bad).is_err(), "{bad}");
        }
        assert!(args(&["send", "text", "hi", "--color", "red"]).is_err(), "parse_args refuses a bad value");
    }

    #[test]
    fn a_text_notice_takes_a_style_and_its_titles() {
        let build = |text: &[&str]| build_notice(&args(text).unwrap(), &[], 1_000);
        let plain = build(&["send", "text", "New update out", "--title", "4D530064"]).unwrap();
        assert_eq!((plain.style, plain.titles, plain.not_after), (None, vec![0x4D53_0064], 1_600));
        let styled = build(&["send", "text", "hi", "--title", "4D530064", "--size", "200"]).unwrap();
        assert_eq!(styled.style, Some(TextStyle { size: 200, ..TextStyle::default() }), "defaults fill the rest");
        let targeted = build_notice(&args(&["send", "dvd"]).unwrap(), &[peer()], 1_000).unwrap();
        assert!(targeted.titles.is_empty());
    }

    #[test]
    fn magnetic_cli_and_live_commands_preserve_and_validate_strength() {
        for n in [0, 1, 100] {
            let parsed = args(&["send", "magnetic", "--title", "54540079", "--strength", &n.to_string()]).unwrap();
            assert_eq!(build_notice(&parsed, &[], 1000).unwrap().kind, Kind::MagneticTraffic { strength: n });
            let line = format!(r#"{{"cmd":"send","kind":"magnetic","strength":{n}}}"#);
            let LiveCommand::Send(live) = live_command(&line, &[0x54540079]).unwrap() else { panic!() };
            assert_eq!(build_notice(&live, &[], 1000).unwrap().kind, Kind::MagneticTraffic { strength: n });
        }
        for bad in ["-1", "101", "1.5", "NaN"] {
            assert!(args(&["send", "magnetic", "--strength", bad]).is_err());
        }
        let wrong = args(&["send", "dvd", "--title", "54540079", "--strength", "10"]).unwrap();
        assert!(build_notice(&wrong, &[], 1000).is_err());
        let default = args(&["send", "magnetic", "--title", "54540079"]).unwrap();
        assert_eq!(build_notice(&default, &[], 1000).unwrap().kind, Kind::MagneticTraffic { strength: 1 });
    }

    #[test]
    fn motion_and_spin_parse_and_only_text_takes_them() {
        let parsed = args(&["send", "text", "hi", "--motion", "bounce", "--spin", "3d", "--size", "1000"]).unwrap();
        assert_eq!((parsed.motion, parsed.spin, parsed.size), (Some(Motion::Bounce), Some(Spin::Spin3d), Some(1000)));
        let names = [("still", Motion::Still), ("bounce", Motion::Bounce), ("marquee", Motion::Marquee)];
        for (name, motion) in names {
            assert_eq!(parse_motion(name), Ok(motion));
        }
        let spins = [("none", Spin::None), ("flat", Spin::Flat), ("3d", Spin::Spin3d), ("seesaw", Spin::SeeSaw),
            ("wobble", Spin::Wobble), ("tumble", Spin::Tumble)];
        for (name, spin) in spins {
            assert_eq!(parse_spin(name), Ok(spin));
        }
        for (flag, bad) in [("--motion", "spin"), ("--motion", ""), ("--spin", "3D"), ("--spin", "roll")] {
            assert!(args(&["send", "text", "hi", flag, bad]).unwrap_err().starts_with(flag), "{flag} {bad}");
        }
        let build = |text: &[&str]| build_notice(&args(text).unwrap(), &[], 1_000);
        let spun = build(&["send", "text", "hi", "--title", "4D530064", "--spin", "tumble"]).unwrap();
        assert_eq!(spun.style, Some(TextStyle { spin: Spin::Tumble, ..TextStyle::default() }));
        for what in ["dvd", "clear"] {
            for flag in [["--motion", "marquee"], ["--spin", "flat"]] {
                let err = build(&["send", what, "--title", "4D530064", flag[0], flag[1]]).unwrap_err();
                assert!(err.contains("only to text"), "{what} {flag:?}: {err}");
            }
        }
        let line = r#"{"cmd":"send","kind":"text","text":"hi","motion":"marquee","spin":"wobble"}"#;
        let Ok(LiveCommand::Send(live)) = live_command(line, &[0x5454_0079]) else { panic!("{line}") };
        assert_eq!((live.motion, live.spin), (Some(Motion::Marquee), Some(Spin::Wobble)));
        for (bad, why) in [
            (r#"{"cmd":"send","kind":"text","text":"hi","motion":"zoom"}"#, "--motion"),
            (r#"{"cmd":"send","kind":"text","text":"hi","spin":"roll"}"#, "--spin"),
        ] {
            assert!(live_command(bad, &[0x5454_0079]).unwrap_err().contains(why), "{bad}");
        }
    }

    #[test]
    fn style_options_need_text_and_text_must_not_be_empty() {
        let build = |text: &[&str]| build_notice(&args(text).unwrap(), &[], 1_000);
        for what in ["dvd", "clear"] {
            for style in [["--color", "cycle"], ["--at", "5,5"], ["--size", "100"]] {
                let err = build(&["send", what, "--title", "4D530064", style[0], style[1]]).unwrap_err();
                assert!(err.contains("only to text"), "{what} {style:?}: {err}");
            }
        }
        for empty in ["", "   ", "\u{3000}"] {
            assert!(build(&["send", "text", empty, "--title", "4D530064"]).unwrap_err().contains("empty"), "{empty:?}");
        }
    }

    /// New York in 2026: UTC-5, and UTC-4 from 2026-03-08 07:00 UTC to 2026-11-01 06:00 UTC.
    fn new_york(utc: i64) -> i64 {
        if (1_772_953_200..1_793_512_800).contains(&utc) { -4 * 3600 } else { -5 * 3600 }
    }

    /// 2026-10-04 20:31 EDT is 2026-10-05 00:31 UTC.
    const BUILD_UNIX: u64 = 1_791_160_260;

    #[test]
    fn civil_dates_convert_to_unix_seconds_and_back() {
        assert_eq!(civil_to_unix(1970, 1, 1, 0, 0), Some(0));
        assert_eq!(civil_to_unix(2026, 1, 1, 0, 0), Some(notice::BUILD_EPOCH_UNIX as i64));
        assert_eq!(civil_to_unix(2026, 10, 5, 0, 31), Some(BUILD_UNIX as i64));
        assert_eq!(civil_to_unix(2028, 2, 29, 0, 0), Some(1_835_395_200), "a leap day");
        assert_eq!(civil_to_unix(2100, 3, 1, 0, 0), Some(4_107_542_400));
        for secs in [0, 59, 60, 86_399, 86_400, 951_782_400, 1_791_160_260, 4_107_542_400 - 60] {
            let (y, mo, d, h, mi) = unix_to_civil(secs);
            assert_eq!(civil_to_unix(y, mo, d, h, mi), Some(secs - secs % 60), "{secs}");
        }
        assert_eq!(unix_to_civil(1_791_160_260), (2026, 10, 5, 0, 31));
        for (y, mo, d, h, mi) in [(2027, 2, 29, 0, 0), (2100, 2, 29, 0, 0), (2026, 4, 31, 0, 0), (2026, 0, 1, 0, 0),
                                  (2026, 13, 1, 0, 0), (2026, 1, 0, 0, 0), (2026, 1, 1, 24, 0), (2026, 1, 1, 0, 60)] {
            assert_eq!(civil_to_unix(y, mo, d, h, mi), None, "{y}-{mo}-{d} {h}:{mi}");
        }
    }

    #[test]
    fn a_local_date_converts_through_the_zone_in_force_on_that_day() {
        let edt = |_: i64| -4 * 3600;
        assert_eq!(when("2026-10-04T20:31", &edt), Ok(BUILD_UNIX));
        assert_eq!(build_of("2026-10-04T20:31", &edt).as_deref(), Ok("08JSV"));
        assert_eq!(build_of("2026-10-04", &edt).as_deref(), Ok("08IUO"), "a bare date is local midnight");
        assert_eq!(build_of("2026-10-04T00:00", &edt).as_deref(), Ok("08IUO"));
        assert_eq!(build_of("2026-03-01T12:00", &new_york).as_deref(), Ok("01UCC"), "winter, UTC-5");
        assert_eq!(build_of("2026-07-01T12:00", &new_york).as_deref(), Ok("05LUO"), "summer, UTC-4");
        assert_eq!(build_of("2026-11-01T12:00", &new_york).as_deref(), Ok("09EKC"), "after the clocks go back");
        assert_eq!(build_of("2026-03-08T03:30", &new_york).as_deref(), Ok("021OI"), "just after they go forward");
        assert_eq!(build_of("2026-10-05T00:31", &|_: i64| 0).as_deref(), Ok("08JSV"), "UTC");
    }

    #[test]
    fn a_code_stands_for_itself_and_bad_builds_are_refused() {
        let edt = |_: i64| -4 * 3600;
        assert_eq!(build_of("08JSV", &edt).as_deref(), Ok("08JSV"));
        assert_eq!(build_of("08jsv", &edt).as_deref(), Ok("08JSV"));
        assert_eq!(when("08JSV", &edt), Ok(BUILD_UNIX));
        for bad in ["", "0.1.5", "v0.1.5", "08JS", "08JSVV", "2026-10-4", "26-10-04", "2026-10-04 20:31", "2026-10-04T20:31Z",
                    "2026-10-04T2031", "2026-10-04T20:31:00", "2026/10/04", "2026-02-30", "2026-13-01", "2026-10-04T24:00",
                    "2026-10-04T20:60", "-2026-10-04", "+026-10-04", "2026-1a-04", "٢٠٢٦-10-04"] {
            assert!(build_of(bad, &edt).is_err(), "{bad:?}");
        }
        assert!(build_of("2025-12-31T18:59", &edt).unwrap_err().contains("range"), "before the epoch");
        assert!(build_of("2141-01-01", &edt).unwrap_err().contains("range"), "past ZZZZZ");
        assert!(build_of("2026-01-01", &|_: i64| 0).is_ok(), "the epoch itself");
    }

    #[test]
    fn stamp_prints_the_code_for_a_date_and_the_date_for_a_code() {
        let edt = |_: i64| -4 * 3600;
        let line = "08JSV  2026-10-04 20:31 local  2026-10-05 00:31 UTC";
        assert_eq!(stamp_line(Some("08JSV"), 0, &edt).as_deref(), Ok(line));
        assert_eq!(stamp_line(Some("08jsv"), 0, &edt).as_deref(), Ok(line));
        assert_eq!(stamp_line(Some("2026-10-04T20:31"), 0, &edt).as_deref(), Ok(line));
        assert_eq!(stamp_line(None, BUILD_UNIX + 17, &edt).as_deref(), Ok(line), "no argument is now");
        assert_eq!(
            stamp_line(Some("2026-07-01T12:00"), 0, &new_york).as_deref(),
            Ok("05LUO  2026-07-01 12:00 local  2026-07-01 16:00 UTC")
        );
        assert!(stamp_line(Some("v0.1.5"), 0, &edt).is_err());
        assert!(stamp_line(None, 5, &edt).is_err(), "now before the epoch has no code");
    }

    #[test]
    fn the_system_zone_offset_is_a_whole_number_of_minutes_within_a_day() {
        for utc in [0, 1_767_225_600, 1_782_921_600, 1_791_160_260] {
            let offset = local_offset(utc);
            assert!(offset.abs() <= 14 * 3600 && offset % 60 == 0, "{utc}: {offset}");
        }
    }

    #[test]
    fn gamertags_resolve_through_friends_txt_and_ambiguity_is_an_error() {
        let dir = std::env::temp_dir().join(format!("rexnet-notice-test-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let (one, two, three) = (peer(), peer(), peer());
        std::fs::write(dir.join("friends.txt"), format!("{one} friend Racer\t\n{two} friend twin\t\n{three} in Twin\t\n")).unwrap();
        assert_eq!(resolve_target("racer", Some(&dir.join("friends.txt"))), Ok(one));
        assert_eq!(resolve_target(&two.to_base58(), None), Ok(two));
        assert!(resolve_target("twin", Some(&dir)).unwrap_err().contains("ambiguous"));
        assert!(resolve_target("nobody", Some(&dir)).unwrap_err().contains("Racer"), "lists the known names");
        assert!(resolve_target("racer", None).is_err());
        let _ = std::fs::remove_dir_all(dir);
    }

    #[test]
    fn only_the_compiled_key_may_sign() {
        assert!(check_key(&Keypair::generate_ed25519()).is_err());
    }

    #[test]
    fn keys_are_never_written_inside_a_git_repository() {
        assert!(inside_git_repo(&Path::new(env!("CARGO_MANIFEST_DIR")).join("notice.key")));
        assert!(!inside_git_repo(&std::env::temp_dir().join("notice.key")));
    }

    /// The GUI parses these (tools/rexnet_notice_gui/cli.py); change them together.
    #[test]
    fn roster_rows_and_the_final_line_keep_the_layout_the_gui_parses() {
        let id = peer();
        let column = format!("{:<54}", id.to_base58());
        assert_eq!(roster_row(0x5454_0079, &id, "Racer One"), format!("54540079   {column} Racer One"));
        assert_eq!(roster_row(0x5454_0079, &id, OLD_BUILD), format!("54540079   {column} (old build)"));
        assert_eq!(roster_summary(2, 1), "2 runner(s) answered, 1 on an old build");
    }

    #[test]
    fn live_commands_parse_to_the_cli_arguments_they_stand_for() {
        let titles = [0x5454_0079];
        assert_eq!(live_command(r#"{"cmd":"roster"}"#, &titles), Ok(LiveCommand::Roster));
        assert_eq!(live_command(r#"{"cmd":"stop"}"#, &titles), Ok(LiveCommand::Stop));
        assert_eq!(live_command(r#" {"cmd":"quit"} "#, &titles), Ok(LiveCommand::Quit));
        let line = r#"{"cmd":"send","kind":"text","text":"hi","color":"cycle","at":[10,90],"size":150,"for":"2h","below":"08jsv"}"#;
        let Ok(LiveCommand::Send(send)) = live_command(line, &titles) else { panic!("{line}") };
        let expected = Args {
            command: "send".into(),
            what: vec!["text".into(), "hi".into()],
            titles: titles.to_vec(),
            below: Some("08JSV".into()),
            lifetime: Some(Duration::from_secs(7200)),
            color: Some(Color::Cycle),
            at: Some((10, 90)),
            size: Some(150),
            ..Args::default()
        };
        assert_eq!(send, expected, "a broadcast goes to the session's titles");
        let notice = build_notice(&send, &[], 1_000).unwrap();
        assert_eq!((notice.titles, notice.not_after), (vec![0x5454_0079], 8_200));
        let target = peer();
        let line = format!(r#"{{"cmd":"send","kind":"dvd","to":["{target}"]}}"#);
        let Ok(LiveCommand::Send(send)) = live_command(&line, &titles) else { panic!("{line}") };
        assert_eq!((send.to, send.titles), (vec![target.to_base58()], vec![]), "a targeted send has no titles");
        for (bad, why) in [
            ("not json", "not a JSON command"),
            (r#"{"cmd":"dance"}"#, "unknown command"),
            (r#"{"kind":"dvd"}"#, "unknown command"),
            (r#"{"cmd":"send","kind":"text","text":"hi","colour":"ff0000"}"#, "unknown field"),
            (r#"{"cmd":"send","text":"hi"}"#, "missing field"),
            (r#"{"cmd":"send","kind":"text","text":"hi","color":"red"}"#, "--color"),
            (r#"{"cmd":"send","kind":"text","text":"hi","at":[101,5]}"#, "--at"),
            (r#"{"cmd":"send","kind":"text","text":"hi","size":1001}"#, "--size"),
            (r#"{"cmd":"send","kind":"dvd","for":"10"}"#, "expected a whole number"),
            (r#"{"cmd":"send","kind":"dvd","below":"v0.1.5"}"#, "--below"),
            (r#"{"cmd":"send","kind":"dvd","to":["Racer One"]}"#, "peer ids only in live mode"),
        ] {
            let err = live_command(bad, &titles).unwrap_err();
            assert!(err.contains(why), "{bad}: {err}");
        }
        // The notice checks are the CLI's too.
        let built = |line: &str| match live_command(line, &titles) {
            Ok(LiveCommand::Send(send)) => build_notice(&send, &[], 1_000),
            other => panic!("{line}: {other:?}"),
        };
        assert!(built(r#"{"cmd":"send","kind":"dvd","size":100}"#).unwrap_err().contains("only to text"));
        assert!(built(r#"{"cmd":"send","kind":"text","text":" "}"#).unwrap_err().contains("empty"));
        assert!(built(r#"{"cmd":"send","kind":"dvd","for":"8d"}"#).unwrap_err().contains("--for"));
    }

    #[test]
    fn engine_events_become_one_json_line_each() {
        let id = peer();
        let line = |event: Event| serde_json::from_str::<Value>(&event_json(&event).unwrap().to_string()).unwrap();
        let entry = |name: &str| Event::RosterEntry { peer: id, title_id: 0x5454_0079, display_name: name.into(), addr: None };
        assert_eq!(line(entry("Racer")), json!({"ev": "runner", "peer": id.to_base58(), "title": "54540079", "name": "Racer"}));
        assert_eq!(
            line(Event::RosterOldBuild { peer: id, title_id: 0x5454_0079 }),
            json!({"ev": "old_build", "peer": id.to_base58(), "title": "54540079"})
        );
        assert_eq!(line(Event::RosterGone { peer: id }), json!({"ev": "gone", "peer": id.to_base58()}));
        assert_eq!(line(Event::NoticeDelivered { peer: id }), json!({"ev": "delivered", "peer": id.to_base58()}));
        assert!(event_json(&Event::PeerDisconnected { peer: id }).is_none(), "not a panel event");
        assert!(!event_json(&entry("a\nb")).unwrap().to_string().contains('\n'), "one line whatever the name");
    }

    #[test]
    fn only_roster_rows_for_the_sessions_titles_are_kept() {
        let (id, titles) = (peer(), [0x5454_0079, 0x4D53_0064]);
        let entry = |title_id| Event::RosterEntry { peer: id, title_id, display_name: "x".into(), addr: None };
        assert!(for_session(&entry(0x4D53_0064), &titles));
        assert!(!for_session(&entry(0), &titles), "another panel answers as title 0");
        assert!(!for_session(&entry(0x1122_3344), &titles), "a runner of another game, reached through mDNS");
        assert!(for_session(&Event::RosterOldBuild { peer: id, title_id: 0x5454_0079 }, &titles), "carries the round's title");
        assert!(!for_session(&Event::RosterOldBuild { peer: id, title_id: 0 }, &titles));
        assert!(for_session(&Event::RosterGone { peer: id }, &titles), "not a row");
    }

    #[test]
    fn remembered_players_are_pruned_capped_and_saved_through_a_rename() {
        let dir = std::env::temp_dir().join(format!("rexnet-notice-cache-{}", std::process::id()));
        let path = dir.join("xrecomp").join("rexnet-notice-peers.json");
        let now = 2_000_000_000;
        let player = |n: u64, addr: Option<&str>| Player {
            peer: format!("peer{n}"),
            name: format!("racer{n}"),
            title: "54540079".into(),
            addr: addr.map(str::to_string),
            seen: now - n,
        };
        assert!(load_players(&path, now).is_empty(), "no file yet");
        let mut players = Vec::new();
        for n in 0..CACHE_MAX as u64 + 5 {
            remember(&mut players, player(n, Some("/ip4/1.2.3.4/tcp/1")), now);
        }
        assert_eq!(players.len(), CACHE_MAX, "capped");
        assert!(players.iter().all(|p| p.seen > now - CACHE_MAX as u64), "the newest kept");
        remember(&mut players, Player { name: "renamed".into(), ..player(3, Some("/ip4/5.6.7.8/tcp/2")) }, now);
        assert_eq!(players.len(), CACHE_MAX, "an answer replaces its peer's entry");
        save_players(&path, &players).unwrap();
        assert!(!path.with_extension("json.tmp").exists(), "renamed into place");
        assert_eq!(load_players(&path, now), players, "round trip");
        let stale = vec![player(1, Some("/ip4/1.2.3.4/tcp/1")), player(CACHE_DAYS * 86_400 + 1, Some("/ip4/1.2.3.4/tcp/1")), player(2, None)];
        save_players(&path, &stale).unwrap();
        assert_eq!(load_players(&path, now), vec![stale[0].clone()], "older than 14 days, or no address: dropped");
        std::fs::write(&path, "not json").unwrap();
        assert!(load_players(&path, now).is_empty(), "a damaged file is no players");
        let _ = std::fs::remove_dir_all(dir);
        assert_eq!(cache_path(Some(Path::new("x.json"))), PathBuf::from("x.json"), "--cache");
        assert!(cache_path(None).ends_with("xrecomp/rexnet-notice-peers.json"));
    }

    #[test]
    fn a_line_that_is_not_utf8_is_an_error_and_a_blank_one_is_skipped() {
        let titles = [0x5454_0079];
        assert!(read_command(b"{\"cmd\":\"send\",\"kind\":\"text\",\"text\":\"\xff\"}", &titles).unwrap().unwrap_err().contains("UTF-8"));
        assert!(read_command(b"  \r\n", &titles).is_none());
        assert_eq!(read_command(b"{\"cmd\":\"quit\"}\r\n", &titles), Some(Ok(LiveCommand::Quit)));
    }

    /// A sink that records each line, and reports the client gone once `open` is false.
    fn recorder(open: bool) -> (Sink, std::sync::Arc<std::sync::Mutex<Vec<Value>>>) {
        let lines = std::sync::Arc::new(std::sync::Mutex::new(Vec::new()));
        let kept = lines.clone();
        let sink: Sink = Box::new(move |line: &str| {
            kept.lock().unwrap().push(serde_json::from_str(line).unwrap());
            open
        });
        (sink, lines)
    }

    #[test]
    fn a_client_that_attaches_late_is_told_who_is_online_and_what_is_still_going_out() {
        let (me, a, b, c, t1, t2) = (peer(), peer(), peer(), peer(), peer(), peer());
        let mut session = Session::new(me);
        let entry = |peer, name: &str| Event::RosterEntry { peer, title_id: 0x5454_0079, display_name: name.into(), addr: None };
        assert!(session.event(&entry(a, "Racer")).is_some());
        assert!(session.event(&Event::RosterOldBuild { peer: b, title_id: 0x5454_0079 }).is_some());
        session.event(&entry(c, "Gone Soon"));
        assert_eq!(session.event(&Event::RosterGone { peer: c }), Some(json!({"ev": "gone", "peer": c.to_base58()})));
        assert_eq!(session.event(&Event::RosterGone { peer: peer() }), None, "a peer never listed leaves silently");
        assert!(session.event(&Event::PeerDisconnected { peer: a }).is_none(), "not a panel event");
        let now = 1_000;
        let signed = notice::sign(&Keypair::generate_ed25519(), &notice::Ask { schema: 1, panel_peer: vec![], issued_ms: 0 });
        let active = |seq, kind: &str, ends| Active { seq, kind: kind.into(), ends };
        session.broadcast = Some(active(5, "dvd", 1_600));
        session.pending.insert(t1, (signed.clone(), active(7, "text", 1_300)));
        session.pending.insert(t2, (signed.clone(), active(7, "text", 1_300)));
        session.pending.insert(peer(), (signed.clone(), active(3, "dvd", 900)));
        assert!(session.event(&Event::NoticeDelivered { peer: t2 }).is_some());
        let mut rows = vec![
            json!({"ev": "runner", "peer": a.to_base58(), "title": "54540079", "name": "Racer"}),
            json!({"ev": "old_build", "peer": b.to_base58(), "title": "54540079"}),
        ];
        rows.sort_by_key(|row| row["peer"].as_str().unwrap().to_string());
        let mut expected = vec![json!({"ev": "ready", "peer": me.to_base58()})];
        expected.extend(rows);
        expected.push(json!({"ev": "active", "seq": 5, "kind": "dvd", "ends": 1_600}));
        expected.push(json!({"ev": "active", "seq": 7, "kind": "text", "ends": 1_300, "pending": [t1.to_base58()]}));
        assert_eq!(session.snapshot(now), expected, "ended notices and delivered targets are left out");
        session.broadcast = Some(active(5, "dvd", now));
        assert_eq!(session.snapshot(now).len(), expected.len() - 1, "a broadcast that has ended is not active");
    }

    #[test]
    fn events_go_to_the_attached_client_and_one_that_has_gone_is_dropped() {
        let me = peer();
        let mut session = Session::new(me);
        session.emit(&json!({"ev": "error", "msg": "nobody listening"}));
        let (gone, seen) = recorder(false);
        session.attach(gone, 0);
        assert_eq!(*seen.lock().unwrap(), vec![json!({"ev": "ready", "peer": me.to_base58()})], "the snapshot first");
        session.emit(&json!({"ev": "error", "msg": "after it went"}));
        assert_eq!(seen.lock().unwrap().len(), 1, "a client that has gone is not written to again");
        let (next, lines) = recorder(true);
        session.attach(next, 0);
        session.emit(&json!({"ev": "sent", "seq": 1}));
        assert_eq!(*lines.lock().unwrap(), vec![json!({"ev": "ready", "peer": me.to_base58()}), json!({"ev": "sent", "seq": 1})]);
    }

    #[test]
    fn the_daemon_log_moves_to_old_once_it_passes_its_cap() {
        let dir = std::env::temp_dir().join(format!("rexnet-notice-log-{}", std::process::id()));
        let path = dir.join("xrecomp").join("rexnet-notice-daemon.log");
        let _ = std::fs::remove_dir_all(&dir);
        let mut log = LogFile::open(path.clone()).unwrap();
        for _ in 0..LOG_MAX / 1024 {
            log.write_all(&[b'x'; 1024]).unwrap();
        }
        assert!(!path.with_extension("log.old").exists(), "exactly at the cap");
        log.write_all(b"next\n").unwrap();
        assert_eq!(std::fs::read(&path).unwrap(), b"next\n");
        assert_eq!(std::fs::metadata(path.with_extension("log.old")).unwrap().len(), LOG_MAX);
        drop(log);
        let mut log = LogFile::open(path.clone()).unwrap();
        log.write_all(b"appended\n").unwrap();
        assert_eq!(std::fs::read(&path).unwrap(), b"next\nappended\n", "a restart appends");
        let _ = std::fs::remove_dir_all(dir);
    }

    #[test]
    fn a_daemon_log_that_cannot_be_moved_is_emptied_instead_of_growing() {
        let dir = std::env::temp_dir().join(format!("rexnet-notice-log-stuck-{}", std::process::id()));
        let path = dir.join("xrecomp").join("rexnet-notice-daemon.log");
        let _ = std::fs::remove_dir_all(&dir);
        // A directory where .log.old goes makes every move fail.
        std::fs::create_dir_all(path.with_extension("log.old")).unwrap();
        let mut log = LogFile::open(path.clone()).unwrap();
        log.write_all(&vec![b'x'; LOG_MAX as usize]).unwrap();
        log.write_all(b"next\n").unwrap();
        log.write_all(b"after\n").unwrap();
        let kept = std::fs::read(&path).unwrap();
        assert!(kept == b"next\nafter\n", "the log holds {} bytes", kept.len());
        let _ = std::fs::remove_dir_all(dir);
    }

    #[test]
    fn a_daemon_log_that_cannot_be_reopened_writes_nothing_until_it_can() {
        let dir = std::env::temp_dir().join(format!("rexnet-notice-log-closed-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        // A file where its directory goes, as after a move whose reopen failed.
        std::fs::write(dir.join("xrecomp"), b"").unwrap();
        let path = dir.join("xrecomp").join("rexnet-notice-daemon.log");
        let mut log = LogFile { path: path.clone(), file: None, size: 0 };
        assert!(log.write_all(b"lost\n").is_err());
        std::fs::remove_file(dir.join("xrecomp")).unwrap();
        log.write_all(b"kept\n").unwrap();
        assert_eq!(std::fs::read(&path).unwrap(), b"kept\n");
        let _ = std::fs::remove_dir_all(dir);
    }

    #[tokio::test]
    async fn roster_timers_fire_once_after_a_sleep_not_once_per_period_missed() {
        let (scans, refreshes) = roster_timers();
        for timer in [scans, refreshes] {
            assert_eq!(timer.missed_tick_behavior(), tokio::time::MissedTickBehavior::Skip);
        }
    }
}
